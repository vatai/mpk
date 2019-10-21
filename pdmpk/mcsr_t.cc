//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-20

#include "mcsr_t.h"

void mcsr_t::rec_phase()
{
  mptr_begin.push_back(mptr.size());
}

void mcsr_t::mcol_push_back(const idx_t idx)
{
  auto &last_mptr_element = *(end(mptr) - 1);
  last_mptr_element++;
  mcol.push_back(idx);
}
