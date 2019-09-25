#include <fstream>
#include <sstream>

#include "buffers_t.h"

const std::string FNAME{"bufs"};

void buffers_t::record_phase()
{
  offset_sbuf.push_back(sbuf.size());
  offset_mptr.push_back(mptr.size());
  offset_mcol.push_back(mcol.size());
  offset_mval.push_back(mval.size());
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
