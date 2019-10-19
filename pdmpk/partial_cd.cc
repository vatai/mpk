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

#include "partial_cd.h"

#include "metis.h"

void partial_cd::debug_print_levels(std::ostream &os)
{
  const int width = 4;
  for (int i = 0; i < csr.n; i++) {
    if (i % 10 == 0) os << std::endl;
    os << std::setw(width) << levels[i] << ", ";
  }
  os << std::endl;
}

void partial_cd::debug_print_partials(std::ostream &os)
{
  int max = 0;
  for (int i = 0; i < csr.n; i++) {
    const idx_t d = csr.ptr[i + 1] - csr.ptr[i];
    if (d > max)
      max = d;
  }
  for (int i = 0; i < csr.n; i++) {
    if (i % 10 == 0) os << std::endl;
    const idx_t d = csr.ptr[i + 1] - csr.ptr[i];
    for (int j = 0; j < max - d; j++)
      os << "_";
    for (int j = csr.ptr[i]; j < csr.ptr[i + 1]; j++)
      os << (partials[j] ? "*" : "O");
    os << " ";
  }
  os << std::endl;
}

void partial_cd::debug_print_partitions(std::ostream &os)
{
  for (int i = 0; i < csr.n; i++) {
    if (i % 10 == 0) os << std::endl;
    os << partitions[i] << ", ";
  }
  os << std::endl;
}

void partial_cd::debug_print_report(std::ostream &os, const int phase)
{
    std::cout << std::endl << "Phase: " << phase;
    debug_print_partitions(std::cout);
    debug_print_levels(std::cout);
    debug_print_partials(std::cout);
}

partial_cd::partial_cd(
    const char *fname,
    const idx_t npart,
    const level_t nlevels)
    : csr{fname},
      npart{npart},
      nlevels{nlevels},
      partitions(csr.n),
      levels(csr.n, 0),
      partials(csr.nnz, 0),
      weights(csr.nnz)
{
  init_vectors();
  init_communication();
  metis_partition();
  update_levels();
  update_weights();
  debug_print_report(std::cout, 0);

  for (int i = 0; i < 7; i++) {
    phase = i;
    phase_shift();
    metis_partition_with_levels();
    update_levels();
    update_weights();
    debug_print_report(std::cout, i + 1);
  }
}

void partial_cd::init_vectors()
{
  partials.resize(csr.nnz, false);

  // TODO(vatai): move this to buffers_t?
  bufs.resize(npart);
  const size_t mbs = csr.n * nlevels;
  for (auto &buffer : bufs) {
    buffer.record_phase();
    buffer.final_mpi_bufs.recvcounts.resize(mbs, 0);
    buffer.final_mpi_bufs.sendcounts.resize(mbs, 0);
    buffer.final_mpi_bufs.rdispls.resize(mbs, 0);
    buffer.final_mpi_bufs.sdispls.resize(mbs, 0);
  }
}

void partial_cd::init_communication()
{
  for (int idx = 0; idx < csr.n; idx++) {
    auto part = partitions[idx];
    set_store_part(idx, 0, part);
  }
}

void partial_cd::phase_shift()
{
  for (auto &buffer : bufs) {
    // buffers.fill_displs(); // scan recv, send count
    buffer.record_phase();
  }

  /// @todo(vatai): Calculate displacements.
  for (size_t i = 1; i < npart; i++) {
  }
  for (idx_t from = 0; from < npart; from++) {
    for (idx_t to = 0; to < npart; to++) {
      auto idx = comm_dict[{from, to}];
    }
  }
}

void partial_cd::update_levels()
{
  // `was_active` is true, if there was progress made at a level. If
  // no progress is made, the next level is processed.
  bool was_active = true;
  // `min_level` is important, see NOTE1 below.
  auto min_level = *std::min_element(begin(levels), end(levels));
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
      if (levels[idx] == lbelow and proc_vertex(idx, lbelow)) {
        was_active = true;
      }
    }
  }
}

bool partial_cd::proc_vertex(const idx_t idx, const level_t lbelow)
{
  bool retval = false;
  const auto p = partitions[idx];

  rec_vert(p);

  for (idx_t t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (can_add(idx, lbelow, t)) {
      proc_adjacent(idx, lbelow, t);
      retval = true;
    }
  }
  if (retval == true) {
    set_store_part(idx, lbelow + 1, p);
    inc_level(idx);
  }
  return retval;
}

void partial_cd::proc_adjacent(const idx_t idx, const level_t lbelow, const idx_t t)
{
  const auto cur_part = partitions[idx];
  const auto j = csr.col[t];
  if (can_add(idx, lbelow, t)) {
    const auto pair = get_store_part(j, lbelow);
    rec_adj(idx, t, pair.second);
    rec_comm(cur_part, pair);
  }
}

void partial_cd::rec_vert(const idx_t part)
{
  auto& buf = bufs[part];
  // buf.mptr.push_back(0);
  // if (buf.mptr_count.size() == phase) {
  //   buf.mptr_count.push_back(1);
  // } else {
  //   buf.mptr_count[phase]++;
  // }
}

void partial_cd::rec_adj(
    const idx_t idx,
    const idx_t adj_tidx,
    const idx_t adj_buf_idx)
{
  partials[adj_tidx] = true;

  const auto cur_part = partitions[idx];
  auto &buf = bufs[cur_part];
  auto &last_mptr_element = *(end(buf.mptr) - 1);
  // last_mptr_element++;
  buf.mcol.push_back(adj_buf_idx);
}

void partial_cd::rec_comm(const idx_t to, const std::pair<idx_t, idx_t> &pair)
{
  const auto& from = pair.first;
  const auto& buf_idx = pair.second;
  if (to != from) {
    /// @todo(vatai): record sending {j, lbelow}, from adj_part to cur_part
    bufs[to].final_mpi_bufs.recvcounts[csr.n * phase + from]++;
    bufs[from].final_mpi_bufs.sendcounts[csr.n * phase + to]++;
    comm_dict[{from, to}] = buf_idx;
  }
}

bool partial_cd::can_add(const idx_t idx, const level_t lbelow, const idx_t t)
{
  const idx_t j = csr.col[t];
  const bool needed = not partials[t];
  const bool same_part = partitions[idx] == partitions[j];
  const bool computed = levels[j] >= lbelow;
  return needed and same_part and computed;
}

void partial_cd::inc_level(const idx_t idx)
{
  if (partial_is_full(idx)) {
    levels[idx]++;
    partial_reset(idx);
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

bool partial_cd::partial_is_full(const idx_t idx)
{
  for (int t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (not partials[t])
      return false;
  }
  return true;
}

void partial_cd::partial_reset(const idx_t idx)
{
  for (int t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    partials[t] = false;
  }
}

void partial_cd::update_weights()
{
  level_t min = *std::min_element(begin(levels), end(levels));

  for (int i = 0; i < csr.n; i++) {
    int li = levels[i];
    for (int j = csr.ptr[i]; j < csr.ptr[i + 1]; j++) {
      int lj = levels[csr.col[j]];
      double w = li + lj - 2 * min;
      w = 1e+6 / (w + 1);
      if (w < 1.0)
        weights[j] = 1;
      else
        weights[j] = w;
    }
  }
}

void partial_cd::metis_partition()
{
  idx_t npart = this->npart;
  idx_t n = csr.n;
  idx_t *ptr = (idx_t *)csr.ptr.data();
  idx_t *col = (idx_t *)csr.col.data();
  idx_t retval, nconstr = 1;
  METIS_PartGraphKway(&n, &nconstr, ptr, col, NULL,
                      NULL, NULL, &npart, NULL, NULL, NULL, &retval,
                      partitions.data());
}

void partial_cd::metis_partition_with_levels()
{
  idx_t npart = this->npart;
  idx_t n = csr.n;
  idx_t *ptr = (idx_t *)csr.ptr.data();
  idx_t *col = (idx_t *)csr.col.data();
  idx_t retval, nconstr = 1;
  idx_t opt[METIS_NOPTIONS];
  METIS_SetDefaultOptions(opt);
  opt[METIS_OPTION_UFACTOR] = 1000;
  opt[METIS_OPTION_CONTIG] = 0;
  METIS_PartGraphKway(&n, &nconstr, ptr, col, NULL, NULL, weights.data(),
                      &npart, NULL, NULL, opt, &retval, partitions.data());
}
