//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-20

#include <fstream>
#include "utils.hpp"
#include "mcsr_t.h"

void mcsr_t::rec_mptr_begin()
{
  mptr_begin.push_back(mptr.size());
}

void mcsr_t::rec_mptr()
{
  mptr.push_back(mcol.size());
}

void mcsr_t::dump_to_ofs(std::ofstream &ofs) {
  dump_vec(mptr, ofs);
  dump_vec(mptr_begin, ofs);
  dump_vec(mcol, ofs);
  dump_vec(mval, ofs);
}

void mcsr_t::load_from_ifs(std::ifstream &ifs) {
  load_vec(mptr, ifs);
  load_vec(mptr_begin, ifs);
  load_vec(mcol, ifs);
  load_vec(mval, ifs);
}
