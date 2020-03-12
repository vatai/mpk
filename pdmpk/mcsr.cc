// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2019-10-20

#include "mcsr.h"
#include "utils.hpp"
#include <fstream>

size_t Mcsr::MptrSize(const int &phase) const {
  return mptr.phase_begin[phase + 1] - mptr.phase_begin[phase];
}

void Mcsr::NextMcolIdxToMptr() { mptr.push_back(mcol.size()); }

void Mcsr::DumpToOFS(std::ofstream &ofs) {
  Utils::DumpVec(mptr, ofs);
  Utils::DumpVec(mptr.phase_begin, ofs);
  Utils::DumpVec(mcol, ofs);
  Utils::DumpVec(mval, ofs);
}

void Mcsr::LoadFromIFS(std::ifstream &ifs) {
  Utils::LoadVec(ifs, &mptr);
  Utils::LoadVec(ifs, &mptr.phase_begin);
  Utils::LoadVec(ifs, &mcol);
  Utils::LoadVec(ifs, &mval);
}

void Mcsr::DumpToTxt(std::ofstream &ofs) {
  Utils::DumpTxt("mptr", mptr, ofs);
  Utils::DumpTxt("mptr.phase_begin", mptr.phase_begin, ofs);
  Utils::DumpTxt("mcol", mcol, ofs);
  Utils::DumpTxt("mval", mval, ofs);
}
