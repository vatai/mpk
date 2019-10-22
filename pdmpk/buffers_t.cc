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

void buffers_t::fill_sbuf_idcs(
    const idx_t src,
    const int phase,
    const std::map<src_tgt_t, std::vector<idx_t>> &comm_dict)
{
  // Fill sbuf_idcs[]
  const auto npart = mpi_bufs.npart;
  const auto nsize =
      mpi_bufs.sbuf_idcs.size() + mpi_bufs.sbuf_size(phase);
  mpi_bufs.sbuf_idcs.resize(nsize);
  std::vector<idx_t> scount(npart, 0);

  const auto offset = npart * phase;

  for (idx_t tgt = 0; tgt < npart; tgt++) {
    const auto iter = comm_dict.find({src, tgt});
    if (iter != end(comm_dict)) {
      const auto idx = mpi_bufs.sdispls[offset + tgt] + scount[tgt];
      const auto dest = begin(mpi_bufs.sbuf_idcs) + idx;
      const auto src_idx_vect = iter->second;
      std::copy(begin(src_idx_vect), end(src_idx_vect), dest);
      scount[tgt] += src_idx_vect.size();
    }
  }
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
