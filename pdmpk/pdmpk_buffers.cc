// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2019-10-21

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <metis.h>

#include "pdmpk_buffers.h"
#include "typedefs.h"

#define SMALL_N 10

PDMPKBuffers::PDMPKBuffers(const Args &args, const CSR &csr)
    : partials(csr.nnz, false), //
      partitions(csr.n),        //
      levels(csr.n, 0),         //
      weights(csr.nnz),         //
      csr{csr},                 //
      args{args} {}

level_t PDMPKBuffers::MinLevel() const {
  return *std::min_element(begin(levels), end(levels));
}

size_t PDMPKBuffers::ExactLevel(idx_t idx) const {
  size_t lvl = levels[idx] * (csr.ptr[idx + 1] - csr.ptr[idx]);
  for (idx_t t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (partials[t])
      lvl++;
  }
  return lvl;
}

size_t PDMPKBuffers::ExactLevelSum() const {
  size_t sum = 0;
  for (idx_t i = 0; i < csr.n; i++) {
    sum += ExactLevel(i);
  }
  return sum;
}

bool PDMPKBuffers::IsFinished() const {
  for (const auto &level : levels) {
    assert(level <= args.nlevel);
    if (level != args.nlevel)
      return false;
  }
  return true;
}

bool PDMPKBuffers::CanAdd(const idx_t &idx,      //
                          const level_t &lbelow, //
                          const idx_t &t) const {
  const idx_t j = csr.col[t];
  const bool needed = not partials[t];
  const bool same_part = partitions[idx] == partitions[j];
  const bool computed = levels[j] >= lbelow;
  return needed and same_part and computed;
}

void PDMPKBuffers::IncLevel(const idx_t &idx) {
  if (PartialIsFull(idx)) {
    levels[idx]++;
    PartialReset(idx);
  }
}

void PDMPKBuffers::UpdateWeights(const level_t &min) {
  for (int i = 0; i < csr.n; i++) {
    const double vi = double(levels[i] - min) + PartialCompleted(i);
    for (int t = csr.ptr[i]; t < csr.ptr[i + 1]; t++) {
      const auto j = csr.col[t];
      const double vj = double(levels[j] - min) + PartialCompleted(j);
      const double denom = vi * vi + vj * vj + 1.0;
      const double w = 1.0e+4 / denom;
      if (w < 1.0)
        weights[t] = 1;
      else
        weights[t] = w;
    }
  }
}

bool PDMPKBuffers::PartialIsFull(const idx_t &idx) const {
  for (int t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (not partials[t])
      return false;
  }
  return true;
}

bool PDMPKBuffers::PartialIsEmpty(const idx_t &idx) const {
  for (int t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (partials[t])
      return false;
  }
  return true;
}

void PDMPKBuffers::PartialReset(const idx_t &idx) {
  for (int t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    partials[t] = false;
  }
}

double PDMPKBuffers::PartialCompleted(const idx_t &idx) const {
  const double all = csr.ptr[idx + 1] - csr.ptr[idx];
  int count = 0;
  for (int t = csr.ptr[idx]; t < csr.ptr[idx + 1]; t++) {
    if (partials[t])
      count++;
  }
  return double(count) / all;
}

void PDMPKBuffers::MetisPartition() {
  idx_t n = csr.n;
  idx_t np = args.npart;
  idx_t *ptr = (idx_t *)csr.ptr.data();
  idx_t *col = (idx_t *)csr.col.data();
  idx_t retval, nconstr = 1;
  METIS_PartGraphKway(&n, &nconstr, ptr, col, NULL, NULL, NULL, &np, NULL, NULL,
                      NULL, &retval, partitions.data());
}

void PDMPKBuffers::MetisPartitionWithWeights() {
  idx_t n = csr.n;
  idx_t np = args.npart;
  idx_t *ptr = (idx_t *)csr.ptr.data();
  idx_t *col = (idx_t *)csr.col.data();
  idx_t retval, nconstr = 1;
  METIS_PartGraphKway(&n, &nconstr, ptr, col, NULL, NULL, weights.data(), &np,
                      NULL, NULL, const_cast<idx_t *>(args.opt), &retval,
                      partitions.data());
}

void PDMPKBuffers::DebugPrintLevels(std::ostream &os) {
  const int width = 4;
  for (int i = 0; i < csr.n; i++) {
    if (i % SMALL_N == 0)
      os << std::endl;
    os << std::setw(width) << levels[i] << ", ";
  }
  os << std::endl;
}

void PDMPKBuffers::DebugPrintPartials(std::ostream &os) {
  int max = 0;
  for (int i = 0; i < csr.n; i++) {
    const idx_t d = csr.ptr[i + 1] - csr.ptr[i];
    if (d > max)
      max = d;
  }
  for (int i = 0; i < csr.n; i++) {
    if (i % SMALL_N == 0)
      os << std::endl;
    const idx_t d = csr.ptr[i + 1] - csr.ptr[i];
    for (int j = 0; j < max - d; j++)
      os << "_";
    for (int j = csr.ptr[i]; j < csr.ptr[i + 1]; j++)
      os << (partials[j] ? "*" : "O");
    os << " ";
  }
  os << std::endl;
}

void PDMPKBuffers::DebugPrintPartitions(std::ostream &os) {
  for (int i = 0; i < csr.n; i++) {
    if (i % SMALL_N == 0)
      os << std::endl;
    os << partitions[i] << ", ";
  }
  os << std::endl;
}

void PDMPKBuffers::DebugPrintReport(std::ostream &os, const int &phase) {
  os << std::endl << "Phase: " << phase;
  DebugPrintPartitions(std::cout);
  DebugPrintLevels(std::cout);
  DebugPrintPartials(std::cout);
}
