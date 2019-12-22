/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-12-01

#pragma once

#include <metis.h>
#include <string>
#include <vector>
class Results {
public:
  Results(const std::string &name);
  void FillVal(const std::vector<idx_t> &idx, const std::vector<double> &mbuf);
  void FillResults(std::vector<double> *results);
  void Dump(const int &rank);
  void Load(const int &rank);
  void DumpTxt(const int &rank);
  void SaveIndex(const int &idx);

private:
  std::string name;
  std::vector<idx_t> vect_idx;
  std::vector<double> val;
};
