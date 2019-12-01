//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-21

#include <algorithm>
#include <iomanip>
#include "PDMPKBuffers.h"

#define SMALL_N 4

PDMPKBuffers::PDMPKBuffers(const CSR &csr) :
    partials(csr.nnz, false),
    partitions(csr.n),
    levels(csr.n, 0),
    weights(csr.nnz),
    csr{csr}
{}

level_t PDMPKBuffers::MinLevel()
{
  return *std::min_element(begin(levels), end(levels));
}

bool PDMPKBuffers::CanAdd(const idx_t idx, const level_t lbelow, const idx_t t)
{
  const idx_t j = csr.col[t];
  const bool needed = not partials[t];
  const bool same_part = partitions[idx] == partitions[j];
  const bool computed = levels[j] >= lbelow;
  return needed and same_part and computed;
}

void PDMPKBuffers::IncLevel(const idx_t idx)
{
  if (PartialIsFull(idx)) {
    levels[idx]++;
    PartialReset(idx);
  }
}

void PDMPKBuffers::UpdateWeights()
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

bool PDMPKBuffers::PartialIsFull(const idx_t idx) const
{
  for (int t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (not partials[t])
      return false;
  }
  return true;
}

bool PDMPKBuffers::PartialIsEmpty(const idx_t idx) const
{
  for (int t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (partials[t])
      return false;
  }
  return true;
}

void PDMPKBuffers::PartialReset(const idx_t idx)
{
  for (int t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    partials[t] = false;
  }
}

void PDMPKBuffers::MetisPartition(idx_t npart)
{
  idx_t n = csr.n;
  idx_t *ptr = (idx_t *)csr.ptr.data();
  idx_t *col = (idx_t *)csr.col.data();
  idx_t retval, nconstr = 1;
  METIS_PartGraphKway(&n, &nconstr, ptr, col, NULL,
                      NULL, NULL, &npart, NULL, NULL, NULL, &retval,
                      partitions.data());
}

void PDMPKBuffers::MetisPartitionWithLevels(idx_t npart)
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

void PDMPKBuffers::DebugPrintLevels(std::ostream &os)
{
  const int width = 4;
  for (int i = 0; i < csr.n; i++) {
    if (i % SMALL_N == 0) os << std::endl;
    os << std::setw(width) << levels[i] << ", ";
  }
  os << std::endl;
}

void PDMPKBuffers::DebugPrintPartials(std::ostream &os)
{
  int max = 0;
  for (int i = 0; i < csr.n; i++) {
    const idx_t d = csr.ptr[i + 1] - csr.ptr[i];
    if (d > max)
      max = d;
  }
  for (int i = 0; i < csr.n; i++) {
    if (i % SMALL_N == 0) os << std::endl;
    const idx_t d = csr.ptr[i + 1] - csr.ptr[i];
    for (int j = 0; j < max - d; j++)
      os << "_";
    for (int j = csr.ptr[i]; j < csr.ptr[i + 1]; j++)
      os << (partials[j] ? "*" : "O");
    os << " ";
  }
  os << std::endl;
}

void PDMPKBuffers::DebugPrintPartitions(std::ostream &os)
{
  for (int i = 0; i < csr.n; i++) {
    if (i % SMALL_N == 0) os << std::endl;
    os << partitions[i] << ", ";
  }
  os << std::endl;
}

void PDMPKBuffers::DebugPrintReport(std::ostream &os, const int phase)
{
    os << std::endl << "Phase: " << phase;
    DebugPrintPartitions(std::cout);
    DebugPrintLevels(std::cout);
    DebugPrintPartials(std::cout);
}
