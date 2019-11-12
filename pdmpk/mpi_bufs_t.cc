//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-19

#include <fstream>
#include "utils.hpp"
#include "mpi_bufs_t.h"

mpi_bufs_t::mpi_bufs_t(const idx_t npart) : npart{npart} {}

void mpi_bufs_t::fill_displs(int phase)
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

int mpi_bufs_t::sbuf_size(int phase) const
{
  size_t idx = npart * phase + npart - 1;
  return sendcounts[idx] + sdispls[idx];
}

int mpi_bufs_t::rbuf_size(int phase) const
{
  size_t idx = npart * phase + npart - 1;
  return recvcounts[idx] + rdispls[idx];
}

void mpi_bufs_t::phase_init()
{
  const auto size = recvcounts.size() + npart;
  recvcounts.resize(size);
  sendcounts.resize(size);
  rdispls.resize(size);
  sdispls.resize(size);

  sbuf_idcs_begin.push_back(sbuf_idcs.size());
  init_idcs_begin.push_back(init_idcs.size());
}

void mpi_bufs_t::dump_to_ofs(std::ofstream &ofs) {
  dump_vec(sendcounts, ofs);
  dump_vec(recvcounts, ofs);
  dump_vec(sdispls, ofs);
  dump_vec(rdispls, ofs);
  dump_vec(sbuf_idcs, ofs);
  dump_vec(sbuf_idcs_begin, ofs);
  dump_vec(init_idcs, ofs);
  dump_vec(init_idcs_begin, ofs);
}

void mpi_bufs_t::load_from_ifs(std::ifstream &ifs) {
  load_vec(sendcounts, ifs);
  load_vec(recvcounts, ifs);
  load_vec(sdispls, ifs);
  load_vec(rdispls, ifs);
  load_vec(sbuf_idcs, ifs);
  load_vec(sbuf_idcs_begin, ifs);
  load_vec(init_idcs, ifs);
  load_vec(init_idcs_begin, ifs);
}
