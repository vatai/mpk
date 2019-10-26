//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-21

#include <algorithm>
#include <iomanip>
#include "pdmpk_bufs_t.h"

pdmpk_bufs_t::pdmpk_bufs_t(const csr_t &csr) :
    csr{csr},
    partials(csr.nnz, false),
    partitions(csr.n),
    levels(csr.n, 0),
    weights(csr.nnz)
{}

level_t pdmpk_bufs_t::min_level()
{
  return *std::min_element(begin(levels), end(levels));
}

bool pdmpk_bufs_t::can_add(const idx_t idx, const level_t lbelow, const idx_t t)
{
  const idx_t j = csr.col[t];
  const bool needed = not partials[t];
  const bool same_part = partitions[idx] == partitions[j];
  const bool computed = levels[j] >= lbelow;
  return needed and same_part and computed;
}

void pdmpk_bufs_t::inc_level(const idx_t idx)
{
  if (partial_is_full(idx)) {
    levels[idx]++;
    partial_reset(idx);
  }
}

void pdmpk_bufs_t::update_weights()
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

bool pdmpk_bufs_t::partial_is_full(const idx_t idx) const
{
  for (int t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (not partials[t])
      return false;
  }
  return true;
}

bool pdmpk_bufs_t::partial_is_empty(const idx_t idx) const
{
  for (int t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (partials[t])
      return false;
  }
  return true;
}

void pdmpk_bufs_t::partial_reset(const idx_t idx)
{
  for (int t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    partials[t] = false;
  }
}

void pdmpk_bufs_t::metis_partition(idx_t npart)
{
  idx_t n = csr.n;
  idx_t *ptr = (idx_t *)csr.ptr.data();
  idx_t *col = (idx_t *)csr.col.data();
  idx_t retval, nconstr = 1;
  METIS_PartGraphKway(&n, &nconstr, ptr, col, NULL,
                      NULL, NULL, &npart, NULL, NULL, NULL, &retval,
                      partitions.data());
}

void pdmpk_bufs_t::metis_partition_with_levels(idx_t npart)
{
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

void pdmpk_bufs_t::debug_print_levels(std::ostream &os)
{
  const int width = 4;
  for (int i = 0; i < csr.n; i++) {
    if (i % 10 == 0) os << std::endl;
    os << std::setw(width) << levels[i] << ", ";
  }
  os << std::endl;
}

void pdmpk_bufs_t::debug_print_partials(std::ostream &os)
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

void pdmpk_bufs_t::debug_print_partitions(std::ostream &os)
{
  for (int i = 0; i < csr.n; i++) {
    if (i % 10 == 0) os << std::endl;
    os << partitions[i] << ", ";
  }
  os << std::endl;
}

void pdmpk_bufs_t::debug_print_report(std::ostream &os, const int phase)
{
    std::cout << std::endl << "Phase: " << phase;
    debug_print_partitions(std::cout);
    debug_print_levels(std::cout);
    debug_print_partials(std::cout);
}
