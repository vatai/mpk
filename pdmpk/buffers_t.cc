#include <fstream>
#include <sstream>

#include "buffers_t.h"

void buffers_t::dump()
{
  double xvar = 3.14;
  std::stringstream fname;
  fname << "fname";
  std::ofstream file(fname.str(), std::ios::binary);
}
