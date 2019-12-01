/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-09-17

#pragma once

#include <fstream>
#include <vector>

#include <metis.h>

class CSR {
public:
  CSR(const char *fname);
  std::vector<double> SpMV(const std::vector<double> &vec) const;
  void MPK(const int nlevels, std::vector<double> &vec) const;
  idx_t n;
  idx_t nnz;
  std::vector<idx_t> ptr;
  std::vector<idx_t> col;
  std::vector<double> val;

private:
  void MtxCheckBanner(std::ifstream &file);
  void MtxFillSize(std::ifstream &file);
  void MtxFillVectors(std::ifstream &file);
};
