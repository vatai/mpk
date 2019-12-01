// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2019-10-20

#include "mcsr.h"
#include "utils.hpp"
#include <fstream>

size_t MCSR::MptrSize(const int phase) const {
  return mptr.begin[phase + 1] - mptr.begin[phase];
}

void MCSR::NextMcolIdxToMptr() { mptr.push_back(mcol.size()); }

void MCSR::DumpToOFS(std::ofstream &ofs) {
  Utils::dump_vec(mptr, ofs);
  Utils::dump_vec(mptr.begin, ofs);
  Utils::dump_vec(mcol, ofs);
  Utils::dump_vec(mval, ofs);
}

void MCSR::LoadFromIFS(std::ifstream &ifs) {
  Utils::load_vec(mptr, ifs);
  Utils::load_vec(mptr.begin, ifs);
  Utils::load_vec(mcol, ifs);
  Utils::load_vec(mval, ifs);
}

void MCSR::DumpToTxt(std::ofstream &ofs) {
  Utils::dump_txt("mptr", mptr, ofs);
  Utils::dump_txt("mptr.begin", mptr.begin, ofs);
  Utils::dump_txt("mcol", mcol, ofs);
  Utils::dump_txt("mval", mval, ofs);
}
