// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2019-12-01

#include "results.h"
#include "utils.hpp"
#include <fstream>
#include <ios>
#include <string>

const std::string kFname{"fresults"};

Results::Results(const Args &args) : args{args} {}

void Results::FillVal(const std::vector<idx_t> &idx,
                      const std::vector<double> &mbuf) {
  val.clear();
  for (const auto i : idx) {
    val.push_back(mbuf[i]);
  }
}

void Results::FillResults(std::vector<double> *results) {
  const size_t size = val.size();
  for (size_t i = 0; i < size; i++) {
    auto idx = vect_idx[i];
    (*results)[idx] = val[i];
  }
}

void Results::Dump(const int &rank) {
  std::ofstream file(Filename(rank, "bin"), std::ios_base::binary);
  Utils::DumpVec(vect_idx, file);
  Utils::DumpVec(val, file);
}

void Results::Load(const int &rank) {
  std::ifstream file(Filename(rank, "bin"), std::ios_base::binary);
  Utils::LoadVec(file, &vect_idx);
  Utils::LoadVec(file, &val);
}

void Results::DumpTxt(const int &rank) {
  std::ofstream file(Filename(rank, "txt"));
  Utils::DumpTxt("result_val", val, file);
}

void Results::SaveIndex(const int &idx) { vect_idx.push_back(idx); }

std::string Results::Filename(const int &rank, const std::string &ext) const {
  return args.mtxname + "-" + kFname + "-" + std::to_string(args.npart) + "-" +
         std::to_string(args.nlevels) + "-" + std::to_string(rank) + "." + ext;
}
