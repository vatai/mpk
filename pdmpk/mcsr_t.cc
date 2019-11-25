//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-20

#include <fstream>
#include "utils.hpp"
#include "mcsr_t.h"

size_t mcsr_t::mptr_size(const int phase) const {
  return mptr.begin[phase + 1] - mptr.begin[phase];
}

void mcsr_t::next_mcol_idx_to_mptr() { mptr.push_back(mcol.size()); }

void mcsr_t::dump_to_ofs(std::ofstream &ofs) {
  Utils::dump_vec(mptr, ofs);
  Utils::dump_vec(mptr.begin, ofs);
  Utils::dump_vec(mcol, ofs);
  Utils::dump_vec(mval, ofs);
}

void mcsr_t::load_from_ifs(std::ifstream &ifs) {
  Utils::load_vec(mptr, ifs);
  Utils::load_vec(mptr.begin, ifs);
  Utils::load_vec(mcol, ifs);
  Utils::load_vec(mval, ifs);
}

void mcsr_t::dump_to_txt(std::ofstream &ofs) {
  Utils::dump_txt("mptr", mptr, ofs);
  Utils::dump_txt("mptr.begin", mptr.begin, ofs);
  Utils::dump_txt("mcol", mcol, ofs);
  Utils::dump_txt("mval", mval, ofs);
}
