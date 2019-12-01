//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-09-17

#include <cassert>
#include <iostream>
#include <iterator>

#include "Buffers.h"
#include "CommCompPatterns.h"

#include "metis.h"
#include "PDMPKBuffers.h"
#include "typedefs.h"

CommCompPatterns::CommCompPatterns(const char *fname,     //
                                   const idx_t npart,     //
                                   const level_t nlevels) //
    : csr{fname},                                         //
      npart{npart},                                       //
      nlevels{nlevels},                                   //
      pdmpkBufs(csr),                                     //
      bufs(npart, Buffers(npart)), phase(0) {
  pdmpkBufs.MetisPartition(npart);
  for (int idx = 0; idx < csr.n; idx++) {
    auto part = pdmpkBufs.partitions[idx];
    FinalizeVertex({idx, 0}, part);
  }
  bool was_active = UpdateLevels();
  while (was_active) {
    phase++;
    pdmpkBufs.MetisPartitionWithLevels(npart);
    was_active = UpdateLevels();
  }
  // nphase + 1
  for (auto &buffer : bufs) {
    buffer.mcsr.mptr.rec_begin();
    buffer.mcsr.NextMcolIdxToMptr();
    buffer.mpiBufs.initIdcs.rec_begin();
  }
  // fill `result_idx`
  for (int i = 0; i < csr.n; i++) {
    const auto &pair = storePart.at({i, nlevels});
    bufs[pair.first].results.vectIdx.push_back(i);
    bufs[pair.first].resultsMbufIdx.push_back(pair.second);
  }
  DbgAsserts();
}

bool CommCompPatterns::UpdateLevels() {
  PhaseInit();
  // `was_active` is true, if there was progress made at a level. If
  // no progress is made, the next level is processed.
  bool was_active = true;
  bool retval = false;
  // `min_level` is important, see NOTE1 below.
  auto min_level = pdmpkBufs.MinLevel();
  // lbelow + 1 = level: we calculate idx at level=lbelow + 1, from
  // vertices col[t] from level=lbelow.
  for (int lbelow = min_level; was_active and lbelow < nlevels; lbelow++) {
    was_active = false;
    // NOTE1: Starting from `min_level` ensures, we start from a level
    // where progress will be made, and set `was_active` to true
    // i.e. starting from level 0, might be a problem if all vertices
    // have level >0, because then, the first round would leave
    // `was_active` as false, and would terminate prematurely.
    for (int idx = 0; idx < csr.n; idx++) {
      if (pdmpkBufs.levels[idx] == lbelow) {
        if (ProcVertex(idx, lbelow)) {
          was_active = true;
          retval = true;
        }
      }
    }
  }
  PhaseFinalize();
  return retval;
}

void CommCompPatterns::PhaseInit() {
  for (auto &buffer : bufs) {
    buffer.PhaseInit();
  }
}

bool CommCompPatterns::ProcVertex(const idx_t idx, const level_t lbelow) {
  bool retval = false;
  const auto cur_part = pdmpkBufs.partitions[idx];
  /// @todo(vatai): This init_idcs is a bit tricky, currently it
  /// should probably converted into a `std::map` or just sort it?
  bool was_dirty = not pdmpkBufs.PartialIsEmpty(idx);

  for (idx_t t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (pdmpkBufs.CanAdd(idx, lbelow, t)) {
      if (retval == false)
        bufs[cur_part].mcsr.NextMcolIdxToMptr();
      ProcAdjacent(idx, lbelow, t);
      retval = true;
    }
  }
  if (retval == true) {
    if (was_dirty) {
      AddToInit(idx, lbelow + 1);
    }
    pdmpkBufs.IncLevel(idx);
    FinalizeVertex({idx, lbelow + 1}, cur_part);
  }
  return retval;
}

void CommCompPatterns::AddToInit(const idx_t idx, const idx_t level) {
  const auto srcPartIdx = storePart.at({idx, level});
  const auto srcPart = srcPartIdx.first;
  const auto srcIdx = srcPartIdx.second;
  const auto tgtPart = pdmpkBufs.partitions[idx];
  const auto tgtIdx = bufs[tgtPart].mbufIdx;
  if (srcPart != tgtPart) {
    // Add to `init_dict`, process it with `proc_init_dict()`.
    bufs[tgtPart].mpiBufs.recvcounts[npart * phase + srcPart]++;
    bufs[srcPart].mpiBufs.sendcounts[npart * phase + tgtPart]++;
    initDict[{srcPart, tgtPart}].push_back({srcIdx, tgtIdx});
  } else {
    // Add to `init_idcs`.
    bufs[tgtPart].mpiBufs.initIdcs.push_back({srcIdx, tgtIdx});
  }
}

void CommCompPatterns::ProcAdjacent(const idx_t idx,      //
                               const level_t lbelow, //
                               const idx_t t) {
  pdmpkBufs.partials[t] = true;

  const auto j = csr.col[t]; // Matrix column index.
  const auto curPart = pdmpkBufs.partitions[idx];
  const auto srcPartIdx = storePart.at({j, lbelow});
  const auto srcPart = srcPartIdx.first;
  const auto srcIdx = srcPartIdx.second;
  bufs[curPart].mcsr.mval.push_back(csr.val[t]);
  bufs[curPart].dbgIdx.push_back(idx);
  if (curPart == srcPart) {
    // 199 // from store part
    bufs[curPart].mcsr.mcol.push_back(srcIdx);
  } else {
    // Record communication.
    bufs[curPart].mpiBufs.recvcounts[npart * phase + srcPart]++;
    bufs[srcPart].mpiBufs.sendcounts[npart * phase + curPart]++;
    const auto tgtIdx = bufs[curPart].mcsr.mcol.size();
    commDict[{srcPart, curPart}].push_back({srcIdx, tgtIdx});
    bufs[curPart].mcsr.mcol.push_back(-1); // Push dummy value.
  }
}

void CommCompPatterns::FinalizeVertex(const idx_lvl_t idx_lvl, const idx_t part) {
  storePart[idx_lvl] = {part, bufs[part].mbufIdx};
  bufs[part].mbufIdx++;
}

void CommCompPatterns::PhaseFinalize() {
  // Update each buffer (separately).
  for (auto &buffer : bufs)
    buffer.PhaseFinalize(phase);

  // Update `mcol` and fill `sbuf_idcs` from `comm_dict`.
  for (comm_dict_t::const_iterator iter = begin(commDict);
       iter != end(commDict); iter++)
    ProcCommDict(iter);

  // Fill `ibuf` and the remainder of `sbuf`.
  for (init_dict_t::const_iterator iter = begin(initDict);
       iter != end(initDict); iter++)
    ProcInitDict(iter);

  commDict.clear();
  initDict.clear();

  pdmpkBufs.UpdateWeights();
  pdmpkBufs.DebugPrintReport(std::cout, phase);

  DbgMbufChecks();
}

void CommCompPatterns::ProcCommDict(const comm_dict_t::const_iterator &iter) {
  auto &src_tgt = iter->first;
  auto &src_mpi_buf = bufs[src_tgt.first].mpiBufs;
  auto &tgt_buf = bufs[src_tgt.second];
  const auto vec = iter->second;

  const auto src_send_baseidx = src_mpi_buf.sbufIdcs.begin[phase] //
                                + SrcSendBase(src_tgt);
  const auto tgt_recv_baseidx = tgt_buf.mbuf.begin[phase]  //
                                + tgt_buf.mcsr.mptr.size() //
                                - tgt_buf.mcsr.mptr.begin[phase] //
                                + TgtRecvBase(src_tgt);
  const auto size = vec.size();
  for (size_t idx = 0; idx < size; idx++) {
    const auto src_idx = vec[idx].first;
    const auto tgt_idx = vec[idx].second;
    src_mpi_buf.sbufIdcs[src_send_baseidx + idx] = src_idx;
    // 142
    tgt_buf.mcsr.mcol[tgt_idx] = tgt_recv_baseidx + idx;
  }
}

void CommCompPatterns::ProcInitDict(const init_dict_t::const_iterator &iter) {
  auto &src_mpi_buf = bufs[iter->first.first].mpiBufs;
  auto &tgt_buf = bufs[iter->first.second];
  const auto &vec = iter->second;

  const auto comm_dict_size = commDict[iter->first].size();
  const auto src_send_baseidx = src_mpi_buf.sbufIdcs.begin[phase] //
                                + SrcSendBase(iter->first) + comm_dict_size;
  const auto tgt_recv_baseidx = tgt_buf.mbuf.begin[phase]  //
                                + tgt_buf.mcsr.mptr.size() //
                                - tgt_buf.mcsr.mptr.begin[phase] //
                                + TgtRecvBase(iter->first) + comm_dict_size;
  const auto size = vec.size();
  for (size_t idx = 0; idx < size; idx++) {
    const auto src_idx = tgt_recv_baseidx + idx;
    const auto tgt_idx = vec[idx].second;
    src_mpi_buf.sbufIdcs[src_send_baseidx + idx] = vec[idx].first;
    // DEBUG //
    assert((int)src_idx < tgt_buf.mbufIdx);
    tgt_buf.mpiBufs.initIdcs.push_back({src_idx, tgt_idx});
  }
}

idx_t CommCompPatterns::SrcSendBase(const sidx_tidx_t src_tgt) const {
  return bufs[src_tgt.first].mpiBufs.sdispls[phase * npart + src_tgt.second];
}

idx_t CommCompPatterns::TgtRecvBase(const sidx_tidx_t src_tgt) const {
  return bufs[src_tgt.second].mpiBufs.rdispls[phase * npart + src_tgt.first];
}

// ////// DEBUG //////

void CommCompPatterns::DbgAsserts() const {
  /// Check all vertices reach `nlevels`.
  for (auto level : pdmpkBufs.levels) {
    assert(level == nlevels);
  }
  /// Assert mbuf + rbuf = mptr size (for each buffer and phase).
  for (auto b : bufs) {
    for (auto phase = 1; phase < this->phase; phase++) {
      auto mbd = b.mbuf.begin[phase] - b.mbuf.begin[phase - 1];
      auto mpd = b.mcsr.mptr.begin[phase] - b.mcsr.mptr.begin[phase - 1];
      auto rbs = b.mpiBufs.RbufSize(phase - 1);
      if (mbd != mpd + rbs) {
        std::cout << "phase: " << phase << ", "
                  << "mbd: " << mbd << ", "
                  << "mpd: " << mpd << ", "
                  << "rbs: " << rbs << std::endl;
      }
      assert(mbd == mpd + rbs);
    }
  }
}

void CommCompPatterns::DbgMbufChecks() {
  // Check nothing goes over mbufIdx.
  for (auto buffer : bufs) {
    auto mbufIdx = buffer.mbufIdx;
    // Check init_idcs.
    for (auto i = buffer.mpiBufs.initIdcs.begin[phase];
         i < buffer.mpiBufs.initIdcs.size(); i++) {
      auto pair = buffer.mpiBufs.initIdcs[i];
      if (pair.first >= mbufIdx) {
        std::cout << pair.first << ", "
                  << mbufIdx << std::endl;
      }
      assert(pair.first < mbufIdx);
      assert(pair.second < mbufIdx);
    }
    // Check sbuf_idcs.
    for (auto i = buffer.mpiBufs.sbufIdcs.begin[phase];
         i < buffer.mpiBufs.sbufIdcs.size(); i++) {
      auto value = buffer.mpiBufs.sbufIdcs[i];
      assert(value < mbufIdx);
    }
    // Check mcol.
    for (auto value : buffer.mcsr.mcol) {
      assert(value < mbufIdx);
    }
  }
}
