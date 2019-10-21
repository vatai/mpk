//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-19

#include "mpi_bufs_t.h"

mpi_bufs_t::mpi_bufs_t(const idx_t npart) : npart{npart} {}

void mpi_bufs_t::fill_dipls(int phase)
{
  size_t offset = npart * phase;
  auto scount = sendcounts.data() + offset;
  auto rcount = recvcounts.data() + offset;
  auto sdispl = sdispls.data() + offset;
  auto rdispl = rdispls.data() + offset;
  sdispl[0] = 0;
  rdispl[0] = 0;
  for (idx_t i = 1; i < npart; i++) {
    sdispl[i] = sdispl[i - 1] + scount[i - 1];
    rdispl[i] = rdispl[i - 1] + rcount[i - 1];
  }
}

int mpi_bufs_t::rbuf_size(int phase)
{
  size_t offset = npart * phase;
  auto rcount = recvcounts.data() + offset;
  auto rdispl = rdispls.data() + offset;
  return rcount[npart - 1] + rdispl[npart - 1];
}

int mpi_bufs_t::sbuf_size(int phase)
{
  size_t offset = npart * phase;
  auto scount = sendcounts.data() + offset;
  auto sdispl = sdispls.data() + offset;
  return scount[npart - 1] + sdispl[npart - 1];
}

void mpi_bufs_t::resize(size_t size)
{
  recvcounts.resize(size);
  sendcounts.resize(size);
  rdispls.resize(size);
  sdispls.resize(size);
}
