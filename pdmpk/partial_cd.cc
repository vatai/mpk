//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-09-17

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "partial_cd.h"

#include "metis.h"

partial_cd::partial_cd(const char *_fname, const int _rank, const idx_t _npart,
                       const level_t _nlevels)
    : crs{_fname}, rank{_rank}, npart{_npart}, nlevels{_nlevels}
{
  partitions.resize(crs.n);
  levels.resize(crs.n, 0);
  weights.resize(crs.nnz);
  partials.resize(crs.nnz, false);
  bufs.resize(npart);

  bufs[rank].record_phase();
  std::cout << std::endl << "Phase: 0";
  metis_partition();
  update_levels();
  update_weights();
  debug_print_partitions(std::cout);
  debug_print_levels(std::cout);
  debug_print_partials(std::cout);

  for (int i = 0; i < 7; i++) {
    std::cout << std::endl << "Phase: " << i + 1;
    bufs[rank].record_phase();
    metis_partition_with_levels();
    update_levels();
    update_weights();
    debug_print_partitions(std::cout);
    debug_print_levels(std::cout);
    debug_print_partials(std::cout);
  }
}

void partial_cd::debug_print_levels(std::ostream &os)
{
  const int width = 4;
  for (int i = 0; i < crs.n; i++) {
    if (i % 10 == 0) os << std::endl;
    os << std::setw(width) << levels[i] << ", ";
  }
  os << std::endl;
}

void partial_cd::debug_print_partials(std::ostream &os)
{
  int max = 0;
  for (int i = 0; i < crs.n; i++) {
    const idx_t d = crs.ptr[i + 1] - crs.ptr[i];
    if (d > max)
      max = d;
  }
  for (int i = 0; i < crs.n; i++) {
    if (i % 10 == 0) os << std::endl;
    const idx_t d = crs.ptr[i + 1] - crs.ptr[i];
    for (int j = 0; j < max - d; j++)
      os << "_";
    for (int j = crs.ptr[i]; j < crs.ptr[i + 1]; j++)
      os << (partials[j] ? "*" : "O");
    os << " ";
  }
  os << std::endl;
}

void partial_cd::debug_print_partitions(std::ostream &os)
{
  for (int i = 0; i < crs.n; i++) {
    if (i % 10 == 0) os << std::endl;
    os << partitions[i] << ", ";
  }
  os << std::endl;
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
    for (int idx = 0; idx < crs.n; idx++) {
      if (levels[idx] == lbelow) {  // needs calculations
        // Pay attention: short-circuiting can cause problems.
        const auto tmp = proc_vertex(idx, lbelow);
        was_active = tmp or was_active;
      }
    }
  }
}

// Process vertex v[idx] at level `lbelow + 1`.
// - add it to (levels, partials)
// - update store_part,
// - add it to buffers
bool partial_cd::proc_vertex(const idx_t idx, const level_t lbelow)
{
  bool retval = false;

  buffers_t *bufptr = bufs.data() + partitions[idx];
  bufptr->pair_mbuf.push_back({idx, lbelow + 1});
  bufptr->mptr.push_back(0);

  for (idx_t t = crs.ptr[idx]; t < crs.ptr[idx + 1]; t++) {
    if (can_add(idx, lbelow, t)) {
      retval = proc_adjacent(idx, lbelow, t);
    }
  }
  if (retval == true) {
    update_data(idx, lbelow + 1);
  }
  return retval;
}

void partial_cd::update_data(const idx_t idx, const level_t level)
{
  store_part[{idx, level}] = partitions[idx];
  if (partial_is_full(idx)) {
    levels[idx]++;
    partial_reset(idx);
  }
}

bool partial_cd::proc_adjacent(const idx_t idx, const level_t lbelow, const idx_t t)
{
  buffers_t *bufptr = bufs.data() + partitions[idx];
  const auto j = crs.col[t];
  if (can_add(idx, lbelow, t)) {
    partials[t] = true;          // Add neighbour `t` to `idx`
    *(bufptr->mptr.end() - 1)++; // increment last element in mptr
    const auto loc = std::find(begin(bufptr->pair_mbuf), end(bufptr->pair_mbuf),
                               std::make_pair(j, lbelow));
    const auto buf_idx = loc - begin(bufptr->pair_mbuf);
    bufptr->mcol.push_back(buf_idx);
    // std::cout << "iter diff: " << buf_idx << std::endl;

    if (store_part.find({j, lbelow}) != end(store_part)) {
      if (store_part[{j, lbelow}] != partitions[idx]) {
      }
    }
    return true;
  }
  return false;
}

bool partial_cd::can_add(const idx_t idx, const level_t lbelow, const idx_t t)
{
  const idx_t j = crs.col[t];
  const bool needed = not partials[t];
  const bool same_part = partitions[idx] == partitions[j];
  const bool computed = levels[j] >= lbelow;
  return needed and same_part and computed;
}

bool partial_cd::partial_is_full(const idx_t idx)
{
  for (int t = crs.ptr[idx]; t < crs.ptr[idx + 1]; t++) {
    if (not partials[t])
      return false;
  }
  return true;
}

void partial_cd::partial_reset(const idx_t idx)
{
  for (int t = crs.ptr[idx]; t < crs.ptr[idx + 1]; t++) {
    partials[t] = false;
  }
}

void partial_cd::update_weights()
{
  level_t min = *std::min_element(begin(levels), end(levels));

  for (int i = 0; i < crs.n; i++) {
    int li = levels[i];
    for (int j = crs.ptr[i]; j < crs.ptr[i + 1]; j++) {
      int lj = levels[crs.col[j]];
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
  idx_t n = crs.n;
  idx_t *ptr = (idx_t *)crs.ptr.data();
  idx_t *col = (idx_t *)crs.col.data();
  idx_t retval, nconstr = 1;
  METIS_PartGraphKway(&n, &nconstr, ptr, col, NULL,
                      NULL, NULL, &npart, NULL, NULL, NULL, &retval,
                      partitions.data());
}

void partial_cd::metis_partition_with_levels()
{
  idx_t npart = this->npart;
  idx_t n = crs.n;
  idx_t *ptr = (idx_t *)crs.ptr.data();
  idx_t *col = (idx_t *)crs.col.data();
  idx_t retval, nconstr = 1;
  idx_t opt[METIS_NOPTIONS];
  METIS_SetDefaultOptions(opt);
  opt[METIS_OPTION_UFACTOR] = 1000;
  opt[METIS_OPTION_CONTIG] = 0;
  METIS_PartGraphKway(&n, &nconstr, ptr, col, NULL, NULL, weights.data(),
                      &npart, NULL, NULL, opt, &retval, partitions.data());
}
