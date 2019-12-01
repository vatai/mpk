#pragma once

#include <metis.h>
#include <vector>
class Results {
 public:
  std::vector<idx_t> vectIdx;
  std::vector<double> val;
  void FillVal(const std::vector<idx_t> &idx, const std::vector<double> &mbuf);
  void Dump(const int rank);
  void Load(const int rank);
  void DumpTxt(const int rank);
};
