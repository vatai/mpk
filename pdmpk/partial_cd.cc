//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-09-17

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "buffers_t.h"
#include "partial_cd.h"

#include "metis.h"
#include "pdmpk_bufs_t.h"
#include "typedefs.h"

partial_cd::partial_cd(
    const char *fname,
    const idx_t npart,
    const level_t nlevels)
    : csr{fname},
      npart{npart},
      nlevels{nlevels},
      pdmpk_bufs(csr),
      bufs(npart, buffers_t(npart))
{
  phase = 0;
  for (auto &buffer : bufs)
    /// @todo(vatai): This is basically a call to `rec_mptr`, some
    /// refactoring might be needed.
    buffer.mcsr.mptr.push_back(0);
  phase_init();
  pdmpk_bufs.metis_partition(npart);
  init_communication();
  update_levels();

  for (int i = 0; i < 7; i++) {
    phase = i + 1;
    phase_init();
    pdmpk_bufs.metis_partition_with_levels(npart);
    update_levels();
  }
}

void partial_cd::phase_init()
{
  for (auto &buffer : bufs) {
    // @todo(vatai): (Maybe) refactor this into
    // `buffer_t::phase_init()` splitting and moving some stuff (like
    // the init and sbuf vectors) out to `buffers_t`.
    buffer.mpi_bufs.phase_init();
    buffer.mcsr.rec_mptr_begin();
    buffer.mbuf_begin.push_back(buffer.mbuf_idx);
  }
}

void partial_cd::init_communication()
{
  for (int idx = 0; idx < csr.n; idx++) {
    auto part = pdmpk_bufs.partitions[idx];
    rec_mbuf_idx({idx, 0}, part);
  }
}

void partial_cd::update_levels()
{
  // `was_active` is true, if there was progress made at a level. If
  // no progress is made, the next level is processed.
  bool was_active = true;
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
      if (pdmpk_bufs.levels[idx] == lbelow and proc_vertex(idx, lbelow)) {
        was_active = true;
      }
    }
  }
  pdmpk_bufs.update_weights();
  pdmpk_bufs.debug_print_report(std::cout, phase);
  phase_finalize();
}

bool partial_cd::proc_vertex(const idx_t idx, const level_t lbelow)
{
  bool retval = false;
  cur_part = pdmpk_bufs.partitions[idx];

  /// @todo(vatai): This init_idcs is a bit tricky, currently it
  /// should probably converted into a `std::map`.
  if (not pdmpk_bufs.partial_is_empty(idx)) {
    const auto src_part_idx = store_part.at({idx, lbelow + 1});
    const auto src_part = src_part_idx.first;
    const auto src_idx = src_part_idx.second;
    if (src_part != cur_part) {
      // Add to init_dict.
    } else {
      // Add to init_idcs.
      bufs[cur_part].mpi_bufs.init_idcs.push_back({src_idx, bufs[cur_part].mbuf_idx});
    }
  }

  for (idx_t t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (pdmpk_bufs.can_add(idx, lbelow, t)) {
      proc_adjacent(idx, lbelow, t);
      retval = true;
    }
  }
  if (retval == true) {
    rec_mbuf_idx({idx, lbelow + 1}, cur_part);
    pdmpk_bufs.inc_level(idx);
  }
  bufs[cur_part].mcsr.rec_mptr();
  return retval;
}

void partial_cd::proc_adjacent(const idx_t idx, const level_t lbelow, const idx_t t)
{
  pdmpk_bufs.partials[t] = true;

  const auto j = csr.col[t]; // Matrix column index.
  const auto src_part_idx = store_part.at({j, lbelow});
  const auto src_part = src_part_idx.first;
  const auto src_idx = src_part_idx.second;
  if (cur_part == src_part_idx.first) {
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

void partial_cd::phase_finalize()
{
  // Update each buffer (separately).
  for (idx_t src = 0; src < npart; src++)
    bufs[src].phase_finalize(phase);

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
}

void partial_cd::proc_comm_dict(const comm_dict_t::const_iterator &iter)
{
  auto src_mpi_buf = bufs[iter->first.first].mpi_bufs;
  auto tgt_buf = bufs[iter->first.second];
  const auto val = iter->second;

  // `sbuf_idcs` of the current phase.
  const auto offset =
      src_mpi_buf.sbuf_idcs_begin[phase] + src_send_base(iter->first);
  auto size = val.size();
  for (auto idx = 0; idx < size; idx++) {
    src_mpi_buf.sbuf_idcs.at(offset + idx) = val[idx].first;
    const auto mbuf_idx = tgt_buf.mbuf_begin[phase] +
                          tgt_recv_base(iter->first) +
                          idx;
    // `val[idx].second` holds the `mcol` index in the target
    // partition.
    tgt_buf.mcsr.mcol[val[idx].second] = mbuf_idx;
  }
}

void partial_cd::proc_init_dict(const init_dict_t::const_iterator &iter)
{

}

void partial_cd::rec_mbuf_idx(const idx_lvl_t idx_lvl, const idx_t part)
{
  store_part[idx_lvl] = {part, bufs[part].mbuf_idx};
  bufs[part].mbuf_idx++;
}

idx_t partial_cd::src_send_base(const sidx_tidx_t src_tgt) const
{
  return bufs[src_tgt.first].mpi_bufs.sdispls[phase * npart + src_tgt.second];
}

idx_t partial_cd::tgt_recv_base(const sidx_tidx_t src_tgt) const
{
  return bufs[src_tgt.second].mpi_bufs.rdispls[phase * npart + src_tgt.first];
}
