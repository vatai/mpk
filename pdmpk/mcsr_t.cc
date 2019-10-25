//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-20

#include "mcsr_t.h"

void mcsr_t::rec_mptr_begin()
{
  mptr_begin.push_back(mptr.size());
}

void mcsr_t::mptr_push_back()
{
  mptr.push_back(mcol.size());
}

void mcsr_t::mcol_push_back(const idx_t idx)
{
  auto &last_mptr_element = *(end(mptr) - 1);
  last_mptr_element++;
  mcol.push_back(idx);
}
