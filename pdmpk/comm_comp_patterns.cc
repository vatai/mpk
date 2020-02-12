// Author: Emil VATAI <emil.vatai@gmail.com>
// Author: Utsav SINGHAL <utsavsinghal5@gmail.com>
// Date: 2019-09-17

#include "metis.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "buffers.h"
#include "comm_comp_patterns.h"
#include "lapjv.h"
#include "pdmpk_buffers.h"
#include "typedefs.h"

CommCompPatterns::CommCompPatterns(const Args &args)
    : bufs(args.npart, Buffers(args)), //
      args{args},                      //
      csr{args.mtxname},               //
      pdmpk_bufs{args, csr},           //
      pdmpk_count{args, csr},          //
      phase{0} {
  pdmpk_bufs.MetisPartition();
  partition_history.push_back(pdmpk_bufs.partitions);
  // Distribute all the vertices to their initial partitions
  for (int idx = 0; idx < csr.n; idx++) {
    auto part = pdmpk_bufs.partitions[idx];
    FinalizeVertex({idx, 0}, part);
  }
  phase = 0;
  ProcPhase(phase);
  ProcAllPhasesCyclePartitions();
  // nphase + 1 since last phase didn't do any update of levels
  for (auto &buffer : bufs) {
    buffer.mcsr.mptr.rec_phase_begin();
    buffer.mcsr.NextMcolIdxToMptr();
    buffer.mpi_bufs.init_idcs.rec_phase_begin();
  }
  // fill `result_idx`
  for (int i = 0; i < csr.n; i++) {
    const auto &pair = store_part.at({i, args.nlevel});
    bufs[pair.first].results.SaveIndex(i);
    bufs[pair.first].results_mbuf_idx.push_back(pair.second);
  }
#ifndef NDEBUG
  DbgAsserts();
#endif
}

void CommCompPatterns::Stats() const {
  std::ofstream of(args.mtxname + "-" + std::to_string(args.npart) + "-" +
                   std::to_string(args.nlevel) + ".stats.txt");

  size_t sum = 0;
  size_t ssum = 0;
  for (const auto &buffer : bufs) {
    for (int i = 0; i < phase; i++) {
      sum += buffer.mpi_bufs.RbufSize(i);
      ssum += buffer.mpi_bufs.SbufSize(i);
    }
  }
  std::cout << "Rbuf sum: " << sum << ", "
            << "Sbuf sum: " << ssum << ", "
            << "Phase (num of): " << phase << std::endl;
  of << sum << " " << phase << std::endl;
}

void CommCompPatterns::ProcAllPhasesMinAboveHalf() {
  bool is_finished = pdmpk_bufs.IsFinished();
  while (not is_finished and not partition_history.empty()) {
    phase++;
    const auto min_level = pdmpk_bufs.MinLevel();
    const auto level_sum = pdmpk_bufs.ExactLevelSum();
    std::cout << "Phase: " << phase << ", "
              << "ExactLevelSum(): " << level_sum << std::endl;
    if (min_level < args.nlevel / 2) {
      std::cout << "First branch" << std::endl;
      pdmpk_bufs.MetisPartitionWithWeights();
      partition_history.push_back(pdmpk_bufs.partitions);
    } else {
      std::cout << "Second branch" << std::endl;
      pdmpk_bufs.partitions = partition_history.back();
      partition_history.pop_back();
    }
    OptimizePartitionLabels(min_level);
    ProcPhase(min_level);
    is_finished = pdmpk_bufs.IsFinished();
  }
  if (not is_finished) {
    std::cout << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__
              << ": Couldn't finish (probably got stuck)." << std::endl;
    exit(1);
  }
}

void CommCompPatterns::ProcAllPhasesCyclePartitions() {
  bool is_finished = pdmpk_bufs.IsFinished();
  while (not is_finished) {
    phase++;
    const auto min_level = pdmpk_bufs.MinLevel();
    const auto level_sum = pdmpk_bufs.ExactLevelSum();
    std::cout << "Phase: " << phase << ", "
              << "ExactLevelSum(): " << level_sum << std::endl;
    if (phase < args.cycle) {
      pdmpk_bufs.MetisPartitionWithWeights();
      partition_history.push_back(pdmpk_bufs.partitions);
    } else {
      pdmpk_bufs.partitions = partition_history[phase % args.cycle];
    }
    OptimizePartitionLabels(min_level);
    ProcPhase(min_level);
    is_finished = pdmpk_bufs.IsFinished();
  }
  if (not is_finished) {
    std::cout << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__
              << ": Couldn't finish (probably got stuck)." << std::endl;
    exit(1);
  }
}

void CommCompPatterns::OptimizePartitionLabels(const size_t &min_level) {
  pdmpk_count.partitions = pdmpk_bufs.partitions;
  pdmpk_count.partials = pdmpk_bufs.partials;
  pdmpk_count.levels = pdmpk_bufs.levels;
  bool was_active_level = true;
  for (int lbelow = min_level; was_active_level and lbelow < args.nlevel;
       lbelow++) {
    was_active_level = false;
    for (int idx = 0; idx < csr.n; idx++) {
      if (pdmpk_count.levels[idx] == lbelow) {
        if (OptimizeVertex(idx, lbelow)) {
          was_active_level = true;
        }
      }
    }
  }
  FindLabelPermutation();
  comm_table.clear();
}

bool CommCompPatterns::OptimizeVertex(const idx_t &idx, const level_t &lbelow) {
  bool retval = false;
  const auto tgt_part = pdmpk_count.partitions[idx];
  bool send_partial = not pdmpk_count.PartialIsEmpty(idx);

  for (idx_t t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (pdmpk_count.CanAdd(idx, lbelow, t)) {
      pdmpk_count.partials[t] = true;
      const auto j = csr.col[t];
      const auto &iter = store_part.find({j, lbelow});
      if (iter != store_part.end()) {
        const auto &src_part_idx = iter->second;
        const auto &src_part = src_part_idx.first;
        comm_table[{src_part, tgt_part}].insert(src_part_idx.second);
        retval = true;
      }
    }
  }
  if (retval == true) {
    if (send_partial) {
      const auto &src_part_idx = store_part.at({idx, lbelow + 1});
      const auto &src_part = src_part_idx.first;
      comm_table[{src_part, tgt_part}].insert(src_part_idx.second);
    }
    pdmpk_count.IncLevel(idx);
  }
  return retval;
}

void CommCompPatterns::FindLabelPermutation() {
  std::vector<int> comm_sums;
  comm_sums.reserve(args.npart * args.npart);
  for (auto s = 0; s < args.npart; s++) {
    for (auto t = 0; t < args.npart; t++) {
      comm_sums.push_back(comm_table[{s, t}].size());
    }
  }
  const auto min_sum = *std::min(std::begin(comm_sums), std::end(comm_sums));
  for (auto &elem : comm_sums) {
    elem = elem - min_sum;
  }

  std::vector<int> permutation(args.npart);
  // The following permutations break ```$ ./pdmpk_prep mp4p4 4 4```
  // std::vector<std::vector<int>> pperms{
  //     {3, 0, 2, 1}, {1, 2, 3, 0}, {1, 0, 3, 2}, {0, 2, 3, 1},
  //     {1, 2, 3, 0}, {2, 3, 1, 0}, {3, 2, 1, 0}, {1, 2, 3, 0},
  // };
  // permutation = pperms[phase - 1];

  lapjv(comm_sums.data(), (int)args.npart, permutation.data());

  for (auto &part : pdmpk_bufs.partitions) {
    assert(0 <= part and part < args.npart);
    part = permutation[part];
    assert(0 <= part and part < args.npart);
  }
}

void CommCompPatterns::ProcPhase(const size_t &min_level) {
  InitPhase();
  // `was_active_level` is true, if there was progress made at a
  // level. If no progress is made, the next level is processed.
  bool was_active_level = true;
  // lbelow + 1 = level: we calculate idx at level=lbelow + 1, from
  // vertices col[t] from level=lbelow.
  for (int lbelow = min_level; was_active_level and lbelow < args.nlevel;
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
        }
      }
    }
  }
  FinalizePhase(min_level);
}

void CommCompPatterns::InitPhase() {
  for (auto &buffer : bufs) {
    buffer.PhaseInit();
  }
}

bool CommCompPatterns::ProcVertex(const idx_t &idx, const level_t &lbelow) {
  bool retval = false;
  const auto cur_part = pdmpk_bufs.partitions[idx];
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

void CommCompPatterns::AddToInit(const idx_t &idx, const idx_t &level) {
  const auto src_part_idx = store_part.at({idx, level});
  const auto src_part = src_part_idx.first;
  const auto src_idx = src_part_idx.second;
  const auto tgt_part = pdmpk_bufs.partitions[idx];
  const auto tgt_idx = bufs[tgt_part].mbuf_idx;
  if (src_part != tgt_part) {
    // Add to `init_dict`, process it with `ProcInitDict()`.
    comm_dict[{src_part, tgt_part}][{src_idx, kInitIdcs}].insert(tgt_idx);
  } else {
    // Add to `init_idcs`.
    bufs[tgt_part].mpi_bufs.init_idcs.push_back({src_idx, tgt_idx});
  }
}

void CommCompPatterns::ProcAdjacent(const idx_t &idx,      //
                                    const level_t &lbelow, //
                                    const idx_t &t) {
  /// @todo(utsav): Remove debug code
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
    const idx_t tgt_idx = bufs[cur_part].mcsr.mcol.size();
    comm_dict[{src_part, cur_part}][{src_idx, kMcol}].insert(tgt_idx);
    bufs[cur_part].mcsr.mcol.push_back(-1); // Push dummy value.
  }
}

void CommCompPatterns::FinalizeVertex(const idx_lvl_t &idx_lvl,
                                      const idx_t &part) {
  store_part[idx_lvl] = {part, bufs[part].mbuf_idx};
  bufs[part].mbuf_idx++;
}

void CommCompPatterns::FinalizePhase(const level_t &min_level) {
  // Fill sendcount and recvcount.
  for (const auto &iter : comm_dict) // CHECK
    UpdateMPICountBuffers(iter.first, iter.second.size());

  for (auto &buffer : bufs)
    buffer.PhaseFinalize(phase);

  // Update `mcol` and fill `sbuf_idcs` from `comm_dict`.
  // (In case of type = kMcol)
  // Fill `ibuf` and the remainder of `sbuf`.
  // (In case of type = kInitIdcs)
  for (CommDict::const_iterator iter = begin(comm_dict); iter != end(comm_dict);
       iter++)
    ProcCommDict(iter);

  comm_dict.clear();

  pdmpk_bufs.UpdateWeights(min_level);
  // pdmpk_bufs.DebugPrintReport(std::cout, phase);

  // Sort `init_idcs`.
  for (auto &buffer : bufs) {
    auto &init_idcs = buffer.mpi_bufs.init_idcs;
    std::sort(std::begin(init_idcs) + init_idcs.phase_begin[phase],
              std::end(init_idcs),
              [](const sidx_tidx_t &a, const sidx_tidx_t &b) {
                return a.second < b.second;
              });
  }
#ifndef NDEBUG
  DbgMbufChecks();
#endif
}

void CommCompPatterns::UpdateMPICountBuffers(const src_tgt_t &src_tgt_part,
                                             const size_t &size) {
  const auto src = src_tgt_part.first;
  const auto tgt = src_tgt_part.second;
  bufs[tgt].mpi_bufs.recvcounts[phase * args.npart + src] += size;
  bufs[src].mpi_bufs.sendcounts[phase * args.npart + tgt] += size;
}

void CommCompPatterns::ProcCommDict(const CommDict::const_iterator &iter) {
  auto &src_tgt = iter->first;
  auto &src_mpi_buf = bufs[src_tgt.first].mpi_bufs;
  auto &tgt_buf = bufs[src_tgt.second];
  const auto &src_type_mapto_tgt_index_set = iter->second;

  const auto src_send_baseidx = src_mpi_buf.sbuf_idcs.phase_begin[phase] //
                                + SrcSendBase(src_tgt);
  const auto tgt_recv_baseidx = tgt_buf.mbuf.phase_begin[phase]        //
                                + tgt_buf.mcsr.mptr.size()             //
                                - tgt_buf.mcsr.mptr.phase_begin[phase] //
                                + TgtRecvBase(src_tgt);
  size_t idx = 0;
  for (const auto &map_iter : src_type_mapto_tgt_index_set) {
    const auto src_idx = tgt_recv_baseidx + idx;
    for (const auto &tgt_idx : map_iter.second) {
      switch (map_iter.first.type) {
      case kMcol:
        tgt_buf.mcsr.mcol[tgt_idx] = tgt_recv_baseidx + idx;
        break;
      case kInitIdcs:
        tgt_buf.mpi_bufs.init_idcs.push_back({src_idx, tgt_idx});
        break;
      }
    }

    src_mpi_buf.sbuf_idcs[src_send_baseidx + idx] = map_iter.first.src_mbuf_idx;
    idx++;
  }
}

idx_t CommCompPatterns::SrcSendBase(const sidx_tidx_t &src_tgt) const {
  return bufs[src_tgt.first]
      .mpi_bufs.sdispls[phase * args.npart + src_tgt.second];
}

idx_t CommCompPatterns::TgtRecvBase(const sidx_tidx_t &src_tgt) const {
  return bufs[src_tgt.second]
      .mpi_bufs.rdispls[phase * args.npart + src_tgt.first];
}

#ifndef NDEBUG

void CommCompPatterns::DbgAsserts() const {
  /// Check all vertices reach `nlevels`.
  for (auto level : pdmpk_bufs.levels) {
    assert(level == args.nlevel);
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

#endif
