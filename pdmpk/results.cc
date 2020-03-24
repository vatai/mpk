// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2019-12-01

#include <cstdio>
#include <fstream>
#include <ios>
#include <string>

#include "results.h"
#include "utils.hpp"

const std::string kFname{"fresults"};

Results::Results(const Args &args) : args{args} {}

void Results::FillVal(const std::vector<double> &mbuf) {
  values.clear();
  for (const auto mbuf_idx : mbuf_idcs) {
    values.push_back(mbuf[mbuf_idx]);
  }
}

void Results::FillResults(std::vector<double> *results) {
  const size_t size = values.size();
  for (size_t i = 0; i < size; i++) {
    auto idx = original_idcs[i];
    (*results)[idx] = values[i];
  }
}

void Results::SaveIndex(const idx_t &idx, const idx_t &mbuf_idx) {
  original_idcs.push_back(idx);
  mbuf_idcs.push_back(mbuf_idx);
}

void Results::Dump(const int &rank) const {
  std::ofstream file(args.Filename("results.bin", rank), std::ios_base::binary);
  Utils::DumpVec(original_idcs, file);
  Utils::DumpVec(values, file);
}

void Results::Load(const int &rank) {
  std::ifstream file(args.Filename("results.bin", rank), std::ios_base::binary);
  Utils::LoadVec(file, &original_idcs);
  Utils::LoadVec(file, &values);
}

void Results::DumpTxt(const int &rank) const {
  std::ofstream file(args.Filename("results.txt", rank));
  Utils::DumpTxt("orig_i", original_idcs, file);
  Utils::DumpTxt("values", values, file);
}

void Results::CleanUp(const int &rank) const {
  if (not args.keepfiles) {
    std::remove(args.Filename("results.bin", rank).c_str());
    std::remove(args.Filename("results.txt", rank).c_str());
  }
}
