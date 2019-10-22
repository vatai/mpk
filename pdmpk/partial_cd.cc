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
    /// @todo(vatai): Rename `rec_phase()` to `rec_mptr_begin()`.
    buffer.mcsr.rec_phase();
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

void partial_cd::phase_finalize()
{
  for (idx_t src = 0; src < npart; src++) {
    auto &buffer = bufs[src];
    buffer.mpi_bufs.fill_dipls(phase);
    buffer.mbuf_idx += buffer.mpi_bufs.rbuf_size(phase);
    // buffer.fun(src, phase, comm_dict);
    fill_sbuf_idcs(src, buffer);
  }
  comm_dict.clear();
}

void partial_cd::fill_sbuf_idcs(const idx_t src, buffers_t& buffer)
{
  // Fill sbuf_idcs[]
  const auto nsize =
      buffer.mpi_bufs.sbuf_idcs.size() + buffer.mpi_bufs.sbuf_size(phase);
  buffer.mpi_bufs.sbuf_idcs.resize(nsize);
  std::vector<idx_t> scount(npart, 0);

  const auto offset = npart * phase;
  auto mpi_bufs = buffer.mpi_bufs;

  for (idx_t tgt = 0; tgt < npart; tgt++) {
    const auto iter = comm_dict.find({src, tgt});
    if (iter != end(comm_dict)) {
      const auto idx = mpi_bufs.sdispls[offset + tgt] + scount[tgt];
      const auto dest = begin(mpi_bufs.sbuf_idcs) + idx;
      const auto src_idx_vect = iter->second;
      std::copy(begin(src_idx_vect), end(src_idx_vect), dest);
      scount[tgt] += src_idx_vect.size();
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
  const auto j = csr.col[t];
  /// @todo(vatai): `can_add` should be checked once, right? (see
  /// `proc_vertex`, it has it too).
  const auto pair = get_store_part(j, lbelow);
  pdmpk_bufs.partials[t] = true;
  bufs[cur_part].mcsr.mcol_push_back(pair.second);
  rec_comm(cur_part, pair);
}

void partial_cd::rec_comm(const idx_t tgt, const part_sidx_t &pair)
{
  const auto& src = pair.first;
  const auto& src_idx = pair.second;
  const auto base = npart * phase;
  if (tgt != src) {
    /// @todo(vatai): record sending {j, lbelow}, from adj_part to cur_part
    bufs[tgt].mpi_bufs.recvcounts[base + src]++;
    bufs[src].mpi_bufs.sendcounts[base + tgt]++;
    const src_tgt_t pair = {src, tgt};
    // auto iter = comm_dict.find(pair);
    // if (iter == end(comm_dict))
    //   comm_dict[pair];
    comm_dict[pair].push_back(src_idx);
  }
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
