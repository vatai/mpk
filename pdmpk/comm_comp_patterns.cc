// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2019-09-17

#include "metis.h"
#include <algorithm>
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
    : bufs(npart, Buffers(npart)),                        //
      csr{fname},                                         //
      npart{npart},                                       //
      nlevels{nlevels},                                   //
      pdmpk_bufs{csr},                                    //
      phase{0} {
  pdmpk_bufs.MetisPartition(npart);
  // Distribute all the vertices to their initial partitions
  for (int idx = 0; idx < csr.n; idx++) {
    auto part = pdmpk_bufs.partitions[idx];
    FinalizeVertex({idx, 0}, part);
  }
  // Process phase = 0 and keep processing next higher phases
  // till the time last phase showed any update
  bool was_active_phase = ProcPhase();
  while (was_active_phase) {
    phase++;
    pdmpk_bufs.MetisPartitionWithWeights(npart);
    was_active_phase = ProcPhase();
  }
  // nphase + 1 since last phase didn't do any update of levels
  for (auto &buffer : bufs) {
    buffer.mcsr.mptr.rec_phase_begin();
    buffer.mcsr.NextMcolIdxToMptr();
    buffer.mpi_bufs.init_idcs.rec_phase_begin();
  }
  // fill `result_idx`
  for (int i = 0; i < csr.n; i++) {
    const auto &pair = store_part.at({i, nlevels});
    bufs[pair.first].results.vect_idx.push_back(i);
    bufs[pair.first].results_mbuf_idx.push_back(pair.second);
  }
  DbgAsserts();
}

bool CommCompPatterns::ProcPhase() {
  InitPhase();
  // `was_active` is true, if there was progress made at a level. If
  // no progress is made, the next level is processed.
  bool was_active_level = true;
  bool retval = false; // used by was_active_phase
  // `min_level` is important, see NOTE1 below.
  auto min_level = pdmpk_bufs.MinLevel();
  // lbelow + 1 = level: we calculate idx at level=lbelow + 1, from
  // vertices col[t] from level=lbelow.
  for (int lbelow = min_level; was_active_level and lbelow < nlevels;
       lbelow++) {
    was_active_level = false;
    // NOTE1: Starting from `min_level` ensures, we start from a level
    // where progress will be made, and set `was_active` to true
    // i.e. starting from level 0, might be a problem if all vertices
    // have level >0, because then, the first round would leave
    // `was_active` as false, and would terminate prematurely.
    for (int idx = 0; idx < csr.n; idx++) {
      if (pdmpk_bufs.levels[idx] == lbelow) {
        if (ProcVertex(idx, lbelow)) {
          was_active_level = true;
          retval = true;
        }
      }
    }
  }
  FinalizePhase();
  return retval;
}

void CommCompPatterns::InitPhase() {
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
      AddToBackPatch(idx, lbelow + 1);
    }
    pdmpk_bufs.IncLevel(idx);
    FinalizeVertex({idx, lbelow + 1}, cur_part);
  }
  return retval;
}

void CommCompPatterns::AddToBackPatch(const idx_t idx, const idx_t level) {
  const auto src_part_idx = store_part.at({idx, level});
  const auto src_part = src_part_idx.first;
  const auto src_idx = src_part_idx.second;
  const auto tgt_part = pdmpk_bufs.partitions[idx];
  const auto tgt_idx = bufs[tgt_part].mbuf_idx;
  if (src_part != tgt_part) {
    // Add to `init_dict`, process it with `ProcInitDict()`.
    SrcTgtType insert_data;
    insert_data.src_mbuf_idx = src_idx;
    insert_data.tgt_idx = tgt_idx;
    insert_data.type = kInitIdcs;
    back_patch[{src_part, tgt_part}].insert(insert_data);
  } else {
    // Add to `init_idcs`.
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
    const auto tgt_idx = bufs[cur_part].mcsr.mcol.size();
    SrcTgtType insert_data;
    insert_data.src_mbuf_idx = src_idx;
    insert_data.tgt_idx = tgt_idx;
    insert_data.type = kMcol;
    back_patch[{src_part, cur_part}].insert(insert_data);
    bufs[cur_part].mcsr.mcol.push_back(-1); // Push dummy value.
  }
}

void CommCompPatterns::FinalizeVertex(const idx_lvl_t idx_lvl,
                                      const idx_t part) {
  store_part[idx_lvl] = {part, bufs[part].mbuf_idx};
  bufs[part].mbuf_idx++;
}

void CommCompPatterns::FinalizePhase() {
  // Fill sendcount and recvcount.
  for (const auto &iter : back_patch)
    UpdateMPICountBuffers(iter.first, iter.second.size());

  for (auto &buffer : bufs)
    buffer.PhaseFinalize(phase);

  // Update `mcol` and fill `sbuf_idcs` from `back_patch`.
  // (In case of type = kMcol)
  // Fill `ibuf` and the remainder of `sbuf`.
  // (In case of type = kInitIdcs)
  for (BackPatch::const_iterator iter = begin(back_patch);
       iter != end(back_patch); iter++)
    ProcBackPatch(iter);

  back_patch.clear();

  pdmpk_bufs.UpdateWeights();
  pdmpk_bufs.DebugPrintReport(std::cout, phase);

  // Sort `init_idcs`.
  for (auto &buffer : bufs) {
  auto &init_idcs = buffer.mpi_bufs.init_idcs;
  std::sort(std::begin(init_idcs) + init_idcs.phase_begin[phase],
            std::end(init_idcs),
            [](const sidx_tidx_t &a, const sidx_tidx_t &b) { return a.second < b.second; });
  }
  DbgMbufChecks();
}

void CommCompPatterns::UpdateMPICountBuffers(const src_tgt_t &src_tgt_part,
                                             const size_t size) {
  const auto src = src_tgt_part.first;
  const auto tgt = src_tgt_part.second;
  bufs[tgt].mpi_bufs.recvcounts[phase * npart + src] += size;
  bufs[src].mpi_bufs.sendcounts[phase * npart + tgt] += size;
}

void CommCompPatterns::ProcBackPatch(const BackPatch::const_iterator &iter) {
  auto &src_tgt = iter->first;
  auto &src_mpi_buf = bufs[src_tgt.first].mpi_bufs;
  auto &tgt_buf = bufs[src_tgt.second];
  const auto &src_tgt_index_set = iter->second;

  const auto src_send_baseidx = src_mpi_buf.sbuf_idcs.phase_begin[phase] //
                                + SrcSendBase(src_tgt);
  const auto tgt_recv_baseidx = tgt_buf.mbuf.phase_begin[phase]        //
                                + tgt_buf.mcsr.mptr.size()             //
                                - tgt_buf.mcsr.mptr.phase_begin[phase] //
                                + TgtRecvBase(src_tgt);
  size_t idx = 0;
  for (const auto &src_tgt_idx : src_tgt_index_set) {
    src_mpi_buf.sbuf_idcs[src_send_baseidx + idx] = src_tgt_idx.src_mbuf_idx;
    if (src_tgt_idx.type==kMcol) {
      const auto tgt_idx = src_tgt_idx.tgt_idx;
      tgt_buf.mcsr.mcol[tgt_idx] = tgt_recv_baseidx + idx;
    } else {
      const auto src_idx = tgt_recv_baseidx + idx;
      const auto tgt_idx = src_tgt_idx.tgt_idx;
      tgt_buf.mpi_bufs.init_idcs.push_back({src_idx, tgt_idx});
    }
    idx++;
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
      auto mbd = b.mbuf.phase_begin[phase] - b.mbuf.phase_begin[phase - 1];
      auto mpd = b.mcsr.mptr.phase_begin[phase] //
                 - b.mcsr.mptr.phase_begin[phase - 1];
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
    for (auto i = buffer.mpi_bufs.init_idcs.phase_begin[phase];
         i < buffer.mpi_bufs.init_idcs.size(); i++) {
      auto pair = buffer.mpi_bufs.init_idcs[i];
      if (pair.first >= mbuf_idx) {
        std::cout << pair.first << ", " << mbuf_idx << std::endl;
      }
      assert(pair.first < mbuf_idx);
      assert(pair.second < mbuf_idx);
    }
    // Check sbuf_idcs.
    for (auto i = buffer.mpi_bufs.sbuf_idcs.phase_begin[phase];
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
