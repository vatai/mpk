//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-09-17

#include <algorithm>
#include <fstream>
#include <iostream>
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
  store_part.resize(crs.n * (nlevels + 1), -1);

  std::cout << std::endl << "Phase: 0";
  metis_partition();
  update_levels();
  update_weights();
  debug_print_partitions();
  debug_print_levels();

  std::cout << std::endl << "Phase: 1";
  metis_partition_with_levels();
  update_levels();
  update_weights();
  debug_print_partitions();
  debug_print_levels();

  std::cout << std::endl << "Phase: 2";
  metis_partition_with_levels();
  update_levels();
  update_weights();
  debug_print_partitions();
  debug_print_levels();

  std::cout << std::endl << "Phase: 3";
  metis_partition_with_levels();
  update_levels();
  update_weights();
  debug_print_partitions();
  debug_print_levels();

  std::cout << std::endl << "Phase: 4";
  metis_partition_with_levels();
  update_levels();
  update_weights();
  debug_print_partitions();
  debug_print_levels();

  std::cout << std::endl << "Phase: 5";
  metis_partition_with_levels();
  update_levels();
  update_weights();
  debug_print_partitions();
  debug_print_levels();

  std::cout << std::endl << "Phase: 6";
  metis_partition_with_levels();
  update_levels();
  update_weights();
  debug_print_partitions();
  debug_print_levels();

  std::cout << std::endl << "Phase: 7";
  metis_partition_with_levels();
  update_levels();
  update_weights();
  debug_print_partitions();
  debug_print_levels();

}

void partial_cd::debug_print_levels()
{
  for (int i = 0; i < crs.n; i++) {
    if (i % 10 == 0) std::cout << std::endl;
    std::cout << levels[i] << ", ";
  }
  std::cout << std::endl;
}

void partial_cd::debug_print_partitions()
{
  for (int i = 0; i < crs.n; i++) {
    if (i % 10 == 0) std::cout << std::endl;
    std::cout << partitions[i] << ", ";
  }
  std::cout << std::endl;
}

void partial_cd::update_levels()
{
  // `was_active` is set to true, if there was progress made. If no
  // progress is made, we should not proceed to the next level.
  int was_active = true;
  for (int lbelow = 0; was_active and lbelow < nlevels; lbelow++) {

    was_active = false;
    for (int idx = 0; idx < crs.n; idx++) {
      if (levels[idx] < lbelow + 1) {  // needs calculations
        // order is here important, because of lazy eval.
        was_active = proc_vertex(idx, lbelow) or was_active;
      }
    }
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

// Process vertex v[idx] at level `level`.
// - add it to (levels, partials)
// - update store_partition,
// - add it to buffers
bool partial_cd::proc_vertex(const idx_t idx, const level_t lbelow)
{
  bool retval = false;
  const idx_t cur_part = partitions[idx];
  for (idx_t t = crs.ptr[idx]; t < crs.ptr[idx + 1]; t++) {
    const idx_t j = crs.col[t];
    const bool needed = not partials[t];
    const bool same_part = cur_part == partitions[j];
    const bool computed = levels[j] >= lbelow - 1;
    if (needed and same_part and computed) {
      // Add neighbour `t` to `idx`
      partials[t] = true;
      // TODO(vatai): Add v[t, k] to the buffers.
      retval = true;
    }
  }
  if (retval == true) {
    store_part[(lbelow + 1) * crs.n + idx] = cur_part;
  }
  if (partial_is_full(idx)) {
    levels[idx]++;
    partial_reset(idx);
  }
  return retval;
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
  METIS_PartGraphKway(&n, &nconstr, ptr, col, NULL, NULL, weights.data(),
                      &npart, NULL, NULL, opt, &retval, partitions.data());
}
