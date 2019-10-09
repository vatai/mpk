#include <fstream>
#include <sstream>

#include "buffers_t.h"

const std::string FNAME{"bufs"};

void buffers_t::record_phase()
{
  sbuf_offset.push_back(sbuf.size());
  mptr_offset.push_back(mptr.size());
  mcol_offset.push_back(mcol.size());
}

void buffers_t::dump(const int rank)
{
  std::stringstream fname;
  fname << FNAME << rank;
  std::ofstream file(fname.str(), std::ios::binary);
  double xvar = 3.14;
  file << xvar;
}

void buffers_t::load(const int rank)
{
  std::stringstream fname;
  fname << FNAME << rank;
  std::ifstream file(fname.str(), std::ios::binary);
  double xvar = 3.14;
  file >> xvar;
}
