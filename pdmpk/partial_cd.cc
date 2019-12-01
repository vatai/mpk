//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-09-17

#include <cassert>
#include <iostream>
#include <iterator>

#include "Buffers.h"
#include "partial_cd.h"

#include "metis.h"
#include "pdmpk_bufs_t.h"
#include "typedefs.h"

partial_cd::partial_cd(const char *fname,     //
                       const idx_t npart,     //
                       const level_t nlevels) //
    : csr{fname},                             //
      npart{npart},                           //
      nlevels{nlevels},                       //
      pdmpk_bufs(csr),                        //
      bufs(npart, Buffers(npart)),
      phase(0) {
  pdmpk_bufs.metis_partition(npart);
  for (int idx = 0; idx < csr.n; idx++) {
    auto part = pdmpk_bufs.partitions[idx];
    finalize_vertex({idx, 0}, part);
  }
  bool was_active = update_levels();
  while (was_active) {
    phase++;
    pdmpk_bufs.metis_partition_with_levels(npart);
    was_active = update_levels();
  }
  // nphase + 1
  for (auto &buffer : bufs) {
    buffer.mcsr.mptr.rec_begin();
    buffer.mcsr.NextMcolIdxToMptr();
    buffer.mpiBufs.init_idcs.rec_begin();
  }
  // fill `result_idx`
  for (int i = 0; i < csr.n; i++) {
    const auto &pair = store_part.at({i, nlevels});
    bufs[pair.first].results.vectIdx.push_back(i);
    bufs[pair.first].results_mbuf_idx.push_back(pair.second);
  }
  dbg_asserts();
}

bool partial_cd::update_levels() {
  phase_init();
  // `was_active` is true, if there was progress made at a level. If
  // no progress is made, the next level is processed.
  bool was_active = true;
  bool retval = false;
  // `min_level` is important, see NOTE1 below.
  auto min_level = pdmpk_bufs.min_level();
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
        if (proc_vertex(idx, lbelow)) {
          was_active = true;
          retval = true;
        }
      }
    }
  }
  phase_finalize();
  return retval;
}

void partial_cd::phase_init() {
  for (auto &buffer : bufs) {
    buffer.PhaseInit();
  }
}

bool partial_cd::proc_vertex(const idx_t idx, const level_t lbelow) {
  bool retval = false;
  const auto cur_part = pdmpk_bufs.partitions[idx];
  /// @todo(vatai): This init_idcs is a bit tricky, currently it
  /// should probably converted into a `std::map` or just sort it?
  bool was_dirty = not pdmpk_bufs.partial_is_empty(idx);

  for (idx_t t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (pdmpk_bufs.can_add(idx, lbelow, t)) {
      if (retval == false)
        bufs[cur_part].mcsr.NextMcolIdxToMptr();
      proc_adjacent(idx, lbelow, t);
      retval = true;
    }
  }
  if (retval == true) {
    if (was_dirty) {
      add_to_init(idx, lbelow + 1);
    }
    pdmpk_bufs.inc_level(idx);
    finalize_vertex({idx, lbelow + 1}, cur_part);
  }
  return retval;
}

void partial_cd::add_to_init(const idx_t idx, const idx_t level) {
  const auto src_part_idx = store_part.at({idx, level});
  const auto src_part = src_part_idx.first;
  const auto src_idx = src_part_idx.second;
  const auto tgt_part = pdmpk_bufs.partitions[idx];
  const auto tgt_idx = bufs[tgt_part].mbufIdx;
  if (src_part != tgt_part) {
    // Add to `init_dict`, process it with `proc_init_dict()`.
    bufs[tgt_part].mpiBufs.recvcounts[npart * phase + src_part]++;
    bufs[src_part].mpiBufs.sendcounts[npart * phase + tgt_part]++;
    init_dict[{src_part, tgt_part}].push_back({src_idx, tgt_idx});
  } else {
    // Add to `init_idcs`.
    bufs[tgt_part].mpiBufs.init_idcs.push_back({src_idx, tgt_idx});
  }
}

void partial_cd::proc_adjacent(const idx_t idx,      //
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
    bufs[cur_part].mpiBufs.recvcounts[npart * phase + src_part]++;
    bufs[src_part].mpiBufs.sendcounts[npart * phase + cur_part]++;
    const auto tgt_idx = bufs[cur_part].mcsr.mcol.size();
    comm_dict[{src_part, cur_part}].push_back({src_idx, tgt_idx});
    bufs[cur_part].mcsr.mcol.push_back(-1); // Push dummy value.
  }
}

void partial_cd::finalize_vertex(const idx_lvl_t idx_lvl, const idx_t part) {
  store_part[idx_lvl] = {part, bufs[part].mbufIdx};
  bufs[part].mbufIdx++;
}

void partial_cd::phase_finalize() {
  // Update each buffer (separately).
  for (auto &buffer : bufs)
    buffer.PhaseFinalize(phase);

  // Update `mcol` and fill `sbuf_idcs` from `comm_dict`.
  for (comm_dict_t::const_iterator iter = begin(comm_dict);
       iter != end(comm_dict); iter++)
    proc_comm_dict(iter);

  // Fill `ibuf` and the remainder of `sbuf`.
  for (init_dict_t::const_iterator iter = begin(init_dict);
       iter != end(init_dict); iter++)
    proc_init_dict(iter);

  comm_dict.clear();
  init_dict.clear();

  pdmpk_bufs.update_weights();
  pdmpk_bufs.debug_print_report(std::cout, phase);

  dbg_mbuf_checks();
}

void partial_cd::proc_comm_dict(const comm_dict_t::const_iterator &iter) {
  auto &src_tgt = iter->first;
  auto &src_mpi_buf = bufs[src_tgt.first].mpiBufs;
  auto &tgt_buf = bufs[src_tgt.second];
  const auto vec = iter->second;

  const auto src_send_baseidx = src_mpi_buf.sbuf_idcs.begin[phase] //
                                + src_send_base(src_tgt);
  const auto tgt_recv_baseidx = tgt_buf.mbuf.begin[phase]  //
                                + tgt_buf.mcsr.mptr.size() //
                                - tgt_buf.mcsr.mptr.begin[phase] //
                                + tgt_recv_base(src_tgt);
  const auto size = vec.size();
  for (size_t idx = 0; idx < size; idx++) {
    const auto src_idx = vec[idx].first;
    const auto tgt_idx = vec[idx].second;
    src_mpi_buf.sbuf_idcs[src_send_baseidx + idx] = src_idx;
    // 142
    tgt_buf.mcsr.mcol[tgt_idx] = tgt_recv_baseidx + idx;
  }
}

void partial_cd::proc_init_dict(const init_dict_t::const_iterator &iter) {
  auto &src_mpi_buf = bufs[iter->first.first].mpiBufs;
  auto &tgt_buf = bufs[iter->first.second];
  const auto &vec = iter->second;

  const auto comm_dict_size = comm_dict[iter->first].size();
  const auto src_send_baseidx = src_mpi_buf.sbuf_idcs.begin[phase] //
                                + src_send_base(iter->first) + comm_dict_size;
  const auto tgt_recv_baseidx = tgt_buf.mbuf.begin[phase]  //
                                + tgt_buf.mcsr.mptr.size() //
                                - tgt_buf.mcsr.mptr.begin[phase] //
                                + tgt_recv_base(iter->first) + comm_dict_size;
  const auto size = vec.size();
  for (size_t idx = 0; idx < size; idx++) {
    const auto src_idx = tgt_recv_baseidx + idx;
    const auto tgt_idx = vec[idx].second;
    src_mpi_buf.sbuf_idcs[src_send_baseidx + idx] = vec[idx].first;
    // DEBUG //
    assert((int)src_idx < tgt_buf.mbufIdx);
    tgt_buf.mpiBufs.init_idcs.push_back({src_idx, tgt_idx});
  }
}

idx_t partial_cd::src_send_base(const sidx_tidx_t src_tgt) const {
  return bufs[src_tgt.first].mpiBufs.sdispls[phase * npart + src_tgt.second];
}

idx_t partial_cd::tgt_recv_base(const sidx_tidx_t src_tgt) const {
  return bufs[src_tgt.second].mpiBufs.rdispls[phase * npart + src_tgt.first];
}

// ////// DEBUG //////

void partial_cd::dbg_asserts() const {
  /// Check all vertices reach `nlevels`.
  for (auto level : pdmpk_bufs.levels) {
    assert(level == nlevels);
  }
  /// Assert mbuf + rbuf = mptr size (for each buffer and phase).
  for (auto b : bufs) {
    for (auto phase = 1; phase < this->phase; phase++) {
      auto mbd = b.mbuf.begin[phase] - b.mbuf.begin[phase - 1];
      auto mpd = b.mcsr.mptr.begin[phase] - b.mcsr.mptr.begin[phase - 1];
      auto rbs = b.mpiBufs.rbuf_size(phase - 1);
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

void partial_cd::dbg_mbuf_checks() {
  // Check nothing goes over mbuf_idx.
  for (auto buffer : bufs) {
    auto mbuf_idx = buffer.mbufIdx;
    // Check init_idcs.
    for (auto i = buffer.mpiBufs.init_idcs.begin[phase];
         i < buffer.mpiBufs.init_idcs.size(); i++) {
      auto pair = buffer.mpiBufs.init_idcs[i];
      if (pair.first >= mbuf_idx) {
        std::cout << pair.first << ", "
                  << mbuf_idx << std::endl;
      }
      assert(pair.first < mbuf_idx);
      assert(pair.second < mbuf_idx);
    }
    // Check sbuf_idcs.
    for (auto i = buffer.mpiBufs.sbuf_idcs.begin[phase];
         i < buffer.mpiBufs.sbuf_idcs.size(); i++) {
      auto value = buffer.mpiBufs.sbuf_idcs[i];
      assert(value < mbuf_idx);
    }
    // Check mcol.
    for (auto value : buffer.mcsr.mcol) {
      assert(value < mbuf_idx);
    }
  }
}
