//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-20

#include "mcsr_t.h"

void mcsr_t::rec_mptr_begin()
{
  mptr_begin.push_back(mptr.size());
}

void mcsr_t::rec_mptr()
{
  mptr.push_back(mcol.size());
}
