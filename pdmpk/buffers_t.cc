//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-17

#include <fstream>
#include <sstream>

#include "buffers_t.h"

const std::string FNAME{"bufs"};

buffers_t::buffers_t(const idx_t npart)
    : mbuf_idx(0),
      mpi_bufs(npart)
{}

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
