#include <iostream>

#include "buffers_t_new.h"

buffers_t::buffers_t(const idx_t _npart)
    : npart {_npart}
{
  std::cout << "basement" << std::endl;
}
