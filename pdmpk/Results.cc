#include "Results.h"
#include "utils.hpp"

const std::string FNAME{"fresults"};

void Results::FillVal(const std::vector<idx_t> &idx,
                       const std::vector<double> &mbuf) {
  for (const auto i : idx) {
    val.push_back(mbuf[i]);
  }
}

void Results::Dump(const int rank) {
  std::ofstream file(FNAME + std::to_string(rank) + ".bin",
                     std::ios_base::binary);
  Utils::dump_vec(vect_idx, file);
  Utils::dump_vec(val, file);
}

void Results::Load(const int rank) {
  const auto fname = FNAME + std::to_string(rank) + ".bin";
}

void Results::DumpTxt(const int rank) {
  std::ofstream file(FNAME + std::to_string(rank) + ".txt");
  Utils::dump_txt("result_val", val, file);
}
