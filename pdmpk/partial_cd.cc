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
  phase_init();
  init_communication();
  pdmpk_bufs.metis_partition(npart);
  update_levels();
  pdmpk_bufs.update_weights();
  pdmpk_bufs.debug_print_report(std::cout, 0);
  phase_finalize();

  for (int i = 0; i < 7; i++) {
    /// @todo(vatai): Create the comm_dict.
    phase = i + 1;
    phase_init();
    pdmpk_bufs.metis_partition_with_levels(npart);
    update_levels();
    pdmpk_bufs.update_weights();
    pdmpk_bufs.debug_print_report(std::cout, i + 1);
    phase_finalize();
  }
}

void partial_cd::phase_init()
{
  const size_t size = npart * (phase + 1);
  for (auto &buffer : bufs) {
    buffer.mpi_bufs.resize(size);
    buffer.mcsr.rec_mptr_begin();
  }
}

void partial_cd::init_communication()
{
  for (int idx = 0; idx < csr.n; idx++) {
    auto part = pdmpk_bufs.partitions[idx];
    set_store_part(idx, 0, part);
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
}

bool partial_cd::proc_vertex(const idx_t idx, const level_t lbelow)
{
  bool retval = false;
  cur_part = pdmpk_bufs.partitions[idx];

  bufs[cur_part].mcsr.mptr.push_back(0);

  for (idx_t t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (pdmpk_bufs.can_add(idx, lbelow, t)) {
      proc_adjacent(idx, lbelow, t);
      retval = true;
    }
  }
  if (retval == true) {
    set_store_part(idx, lbelow + 1, cur_part);
    pdmpk_bufs.inc_level(idx);
  }
  return retval;
}

void partial_cd::proc_adjacent(const idx_t idx, const level_t lbelow, const idx_t t)
{
  pdmpk_bufs.partials[t] = true;

  const auto j = csr.col[t]; // Matrix column index.
  const auto src_part_idx = get_store_part(j, lbelow);
  const auto src_part = src_part_idx.first;
  if (cur_part == src_part_idx.first) {
    bufs[cur_part].mcsr.mcol_push_back(src_part_idx.second);
  } else {
    bufs[cur_part].mcsr.mcol_push_back(-1); // Push dummy value.
    // Record communication.
    bufs[cur_part].mpi_bufs.recvcounts[npart * phase + src_part]++;
    bufs[src_part].mpi_bufs.sendcounts[npart * phase + cur_part]++;
    comm_dict[{src_part, cur_part}].push_back(src_part);
  }
}

void partial_cd::phase_finalize()
{
  for (idx_t src = 0; src < npart; src++) {
    /// Fill displacement buffers from count buffers.
    auto &mpi_bufs = bufs[src].mpi_bufs;
    mpi_bufs.fill_displs(phase);

    const auto rbuf_size = mpi_bufs.rbuf_size(phase);
    bufs[src].mbuf_idx += rbuf_size; // Account for the inserted `recvbuf` in `mbuf`.

    const auto sbuf_size = mpi_bufs.sbuf_idcs.size() + mpi_bufs.sbuf_size(phase);
    mpi_bufs.sbuf_idcs.resize(sbuf_size);

    bufs[src].fill_sbuf_idcs(src, phase, comm_dict);
  }
  comm_dict.clear();
}

void partial_cd::mbuf_insert_rbuf()
{
}

void partial_cd::fill_sbuf_idcs(const idx_t src, buffers_t& buffer)
{
}

std::pair<idx_t, idx_t> partial_cd::get_store_part(
    const idx_t idx,
    const level_t level)
{
  const auto iter = store_part.find({idx, level});
  assert(iter != end(store_part));
  return iter->second;
}

void partial_cd::set_store_part(const idx_t idx, const level_t level, const idx_t part)
{
  // auto& pair_mbuf = bufs[part].pair_mbuf;
  store_part[{idx, level}] = {part, bufs[part].mbuf_idx++};
  // pair_mbuf.push_back({idx, level});
}

idx_t partial_cd::src_send_base(const sidx_tidx_t src_tgt) const
{
  return bufs[src_tgt.first].mpi_bufs.sdispls[src_tgt.second];
}

idx_t partial_cd::tgt_recv_base(const sidx_tidx_t src_tgt) const
{
  return bufs[src_tgt.second].mpi_bufs.rdispls[src_tgt.first];
}
