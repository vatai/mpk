// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2019-09-17

#include "metis.h"
#include <cassert>
#include <iostream>
#include <iterator>

#include "buffers.h"
#include "comm_comp_patterns.h"
#include "pdmpk_buffers.h"
#include "typedefs.h"

CommCompPatterns::CommCompPatterns(const char *fname,     //
                                   const idx_t npart,     //
                                   const level_t nlevels) //
    : csr{fname},                                         //
      npart{npart},                                       //
      nlevels{nlevels},                                   //
      pdmpk_bufs(csr),                                    //
      bufs(npart, Buffers(npart)),                        //
      phase(0) {
  pdmpk_bufs.MetisPartition(npart);
  for (int idx = 0; idx < csr.n; idx++) {
    auto part = pdmpk_bufs.partitions[idx];
    FinalizeVertex({idx, 0}, part);
  }
  bool was_active = UpdateLevels();
  while (was_active) {
    phase++;
    pdmpk_bufs.MetisPartitionWithLevels(npart);
    was_active = UpdateLevels();
  }
  // nphase + 1
  for (auto &buffer : bufs) {
    buffer.mcsr.mptr.rec_begin();
    buffer.mcsr.NextMcolIdxToMptr();
    buffer.mpi_bufs.init_idcs.rec_begin();
  }
  // fill `result_idx`
  for (int i = 0; i < csr.n; i++) {
    const auto &pair = store_part.at({i, nlevels});
    bufs[pair.first].results.vect_idx.push_back(i);
    bufs[pair.first].results_mbuf_idx.push_back(pair.second);
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
  auto min_level = pdmpk_bufs.MinLevel();
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
      if (pdmpk_bufs.levels[idx] == lbelow) {
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
  const auto cur_part = pdmpk_bufs.partitions[idx];
  /// @todo(vatai): This init_idcs is a bit tricky, currently it
  /// should probably converted into a `std::map` or just sort it?
  bool send_partial = not pdmpk_bufs.PartialIsEmpty(idx);

  for (idx_t t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (pdmpk_bufs.CanAdd(idx, lbelow, t)) {
      if (retval == false)
        bufs[cur_part].mcsr.NextMcolIdxToMptr();
      ProcAdjacent(idx, lbelow, t);
      retval = true;
    }
  }
  if (retval == true) {
    if (send_partial) {
      AddToInit(idx, lbelow + 1);
    }
    pdmpk_bufs.IncLevel(idx);
    FinalizeVertex({idx, lbelow + 1}, cur_part);
  }
  return retval;
}

void CommCompPatterns::AddToInit(const idx_t idx, const idx_t level) {
  const auto src_part_idx = store_part.at({idx, level});
  const auto src_part = src_part_idx.first;
  const auto src_idx = src_part_idx.second;
  const auto tgt_part = pdmpk_bufs.partitions[idx];
  const auto tgt_idx = bufs[tgt_part].mbuf_idx;
  if (src_part != tgt_part) {
    // Add to `init_dict`, process it with `ProcInitDict()`.
    bufs[tgt_part].mpi_bufs.recvcounts[npart * phase + src_part]++;
    bufs[src_part].mpi_bufs.sendcounts[npart * phase + tgt_part]++;
    init_dict[{src_part, tgt_part}].push_back({src_idx, tgt_idx});
  } else {
    // Add to `initIdcs`.
    bufs[tgt_part].mpi_bufs.init_idcs.push_back({src_idx, tgt_idx});
  }
}

void CommCompPatterns::ProcAdjacent(const idx_t idx,      //
                                    const level_t lbelow, //
                                    const idx_t t) {
  pdmpk_bufs.partials[t] = true;

  const auto j = csr.col[t]; // Matrix column index.
  const auto cur_part = pdmpk_bufs.partitions[idx];
  const auto src_part_idx = store_part.at({j, lbelow});
  const auto src_part = src_part_idx.first;
  const auto src_idx = src_part_idx.second;
  bufs[cur_part].mcsr.mval.push_back(csr.val[t]);
  bufs[cur_part].dbg_idx.push_back(idx);
  if (cur_part == src_part) {
    // 199 // from store part
    bufs[cur_part].mcsr.mcol.push_back(src_idx);
  } else {
    // Record communication.
    bufs[cur_part].mpi_bufs.recvcounts[npart * phase + src_part]++;
    bufs[src_part].mpi_bufs.sendcounts[npart * phase + cur_part]++;
    const auto tgt_idx = bufs[cur_part].mcsr.mcol.size();
    comm_dict[{src_part, cur_part}].push_back({src_idx, tgt_idx});
    bufs[cur_part].mcsr.mcol.push_back(-1); // Push dummy value.
  }
}

void CommCompPatterns::FinalizeVertex(const idx_lvl_t idx_lvl,
                                      const idx_t part) {
  store_part[idx_lvl] = {part, bufs[part].mbuf_idx};
  bufs[part].mbuf_idx++;
}

void CommCompPatterns::PhaseFinalize() {
  // Update each buffer (separately).
  for (auto &buffer : bufs)
    buffer.PhaseFinalize(phase);

  // Update `mcol` and fill `sbuf_idcs` from `comm_dict`.
  for (CommDict::const_iterator iter = begin(comm_dict);
       iter != end(comm_dict); iter++)
    ProcCommDict(iter);

  // Fill `ibuf` and the remainder of `sbuf`.
  for (InitDict::const_iterator iter = begin(init_dict);
       iter != end(init_dict); iter++)
    ProcInitDict(iter);

  comm_dict.clear();
  init_dict.clear();

  pdmpk_bufs.UpdateWeights();
  pdmpk_bufs.DebugPrintReport(std::cout, phase);

  DbgMbufChecks();
}

void CommCompPatterns::ProcCommDict(const CommDict::const_iterator &iter) {
  auto &src_tgt = iter->first;
  auto &src_mpi_buf = bufs[src_tgt.first].mpi_bufs;
  auto &tgt_buf = bufs[src_tgt.second];
  const auto vec = iter->second;

  const auto src_send_baseidx = src_mpi_buf.sbuf_idcs.begin[phase] //
                                + SrcSendBase(src_tgt);
  const auto tgt_recv_baseidx = tgt_buf.mbuf.begin[phase]        //
                                + tgt_buf.mcsr.mptr.size()       //
                                - tgt_buf.mcsr.mptr.begin[phase] //
                                + TgtRecvBase(src_tgt);
  const auto size = vec.size();
  for (size_t idx = 0; idx < size; idx++) {
    const auto src_idx = vec[idx].first;
    const auto tgt_idx = vec[idx].second;
    src_mpi_buf.sbuf_idcs[src_send_baseidx + idx] = src_idx;
    tgt_buf.mcsr.mcol[tgt_idx] = tgt_recv_baseidx + idx;
  }
}

void CommCompPatterns::ProcInitDict(const InitDict::const_iterator &iter) {
  auto &src_mpi_buf = bufs[iter->first.first].mpi_bufs;
  auto &tgt_buf = bufs[iter->first.second];
  const auto &vec = iter->second;

  const auto comm_dict_size = comm_dict[iter->first].size();
  const auto src_send_baseidx = src_mpi_buf.sbuf_idcs.begin[phase] //
                                + SrcSendBase(iter->first) + comm_dict_size;
  const auto tgt_recv_baseidx = tgt_buf.mbuf.begin[phase]        //
                                + tgt_buf.mcsr.mptr.size()       //
                                - tgt_buf.mcsr.mptr.begin[phase] //
                                + TgtRecvBase(iter->first) + comm_dict_size;
  const auto size = vec.size();
  for (size_t idx = 0; idx < size; idx++) {
    const auto src_idx = tgt_recv_baseidx + idx;
    const auto tgt_idx = vec[idx].second;
    src_mpi_buf.sbuf_idcs[src_send_baseidx + idx] = vec[idx].first;
    assert((int)src_idx < tgt_buf.mbuf_idx);
    tgt_buf.mpi_bufs.init_idcs.push_back({src_idx, tgt_idx});
  }
}

idx_t CommCompPatterns::SrcSendBase(const sidx_tidx_t src_tgt) const {
  return bufs[src_tgt.first].mpi_bufs.sdispls[phase * npart + src_tgt.second];
}

idx_t CommCompPatterns::TgtRecvBase(const sidx_tidx_t src_tgt) const {
  return bufs[src_tgt.second].mpi_bufs.rdispls[phase * npart + src_tgt.first];
}

// ////// DEBUG //////

void CommCompPatterns::DbgAsserts() const {
  /// Check all vertices reach `nlevels`.
  for (auto level : pdmpk_bufs.levels) {
    assert(level == nlevels);
  }
  /// Assert mbuf + rbuf = mptr size (for each buffer and phase).
  for (auto b : bufs) {
    for (auto phase = 1; phase < this->phase; phase++) {
      auto mbd = b.mbuf.begin[phase] - b.mbuf.begin[phase - 1];
      auto mpd = b.mcsr.mptr.begin[phase] - b.mcsr.mptr.begin[phase - 1];
      auto rbs = b.mpi_bufs.RbufSize(phase - 1);
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
    auto mbuf_idx = buffer.mbuf_idx;
    // Check init_idcs.
    for (auto i = buffer.mpi_bufs.init_idcs.begin[phase];
         i < buffer.mpi_bufs.init_idcs.size(); i++) {
      auto pair = buffer.mpi_bufs.init_idcs[i];
      if (pair.first >= mbuf_idx) {
        std::cout << pair.first << ", " << mbuf_idx << std::endl;
      }
      assert(pair.first < mbuf_idx);
      assert(pair.second < mbuf_idx);
    }
    // Check sbuf_idcs.
    for (auto i = buffer.mpi_bufs.sbuf_idcs.begin[phase];
         i < buffer.mpi_bufs.sbuf_idcs.size(); i++) {
      auto value = buffer.mpi_bufs.sbuf_idcs[i];
      assert(value < mbuf_idx);
    }
    // Check mcol.
    for (auto value : buffer.mcsr.mcol) {
      assert(value < mbuf_idx);
    }
  }
}
