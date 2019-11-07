//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-17

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <mpi.h>

#include "buffers_t.h"

const std::string FNAME{"bufs"};

buffers_t::buffers_t(const idx_t npart) : mbuf_idx(0), mpi_bufs(npart) {}

void buffers_t::phase_finalize(const int phase) {
  const auto rbuf_size = mpi_bufs.rbuf_size(phase);
  /// Fill displacement buffers from count buffers.
  mpi_bufs.fill_displs(phase);

  // Allocate `sbuf_idcs` for this phase.
  mpi_bufs.sbuf_idcs.resize(mpi_bufs.sbuf_idcs.size() +
                            mpi_bufs.sbuf_size(phase));
  // Update `mbuf_idx`.
  mbuf_idx += rbuf_size;

  // Update `mcol`.
  const auto mptr_begin = mcsr.mptr_begin[phase];
  const auto mcol_begin = mcsr.mptr[mptr_begin];
  const auto mcol_end = mcsr.mcol.size();
  const auto mbuf_begin_idx = this->mbuf_begin[phase];
  for (idx_t t = mcol_begin; t < mcol_end; t++) {
    if (mcsr.mcol[t] < mbuf_begin_idx and mcsr.mcol[t] != -1)
      mcsr.mcol[t] += rbuf_size;
  }
}

void buffers_t::do_comp(int phase, std::vector<double> &mbuf) {
  // assert(phase < mcsr.mptr_begin.size());
  // assert(phase + 1 < mcsr.mptr_begin.size());

  auto mcount = mcsr.mptr_begin[phase + 1] - mcsr.mptr_begin[phase];
  auto mptr = mcsr.mptr.data() + mcsr.mptr_begin[phase];
  //?? Following two lines are from the older `mpk2` code
  //??   long *mcol = bufs->mcol_buf + bufs->mcol_offsets[phase];
  //??   double *mval = bufs->mval_buf + bufs->mcol_offsets[phase];
  auto cur_mbuf = mbuf.data() + mbuf_begin[phase];
  for (auto mi = 0; mi < mcount; mi++) {
    double tmp = 0.0;
    for (auto mj = mcsr.mptr[mi]; mj < mcsr.mptr[mi + 1]; mj++) {
      tmp += mcsr.mval[mj] * mbuf[mcsr.mcol[mj]];
    }
    cur_mbuf[mi] += tmp;
  }
}

void buffers_t::do_comm(int phase, std::vector<double> &mbuf) {
  // fill_sbuf()
  // call_mpi()
  const auto scount = mpi_bufs.sbuf_size(phase);
  const auto sbuf_idcs = mpi_bufs.sbuf_idcs.data() + mpi_bufs.sbuf_idcs_begin[phase];
  const auto sbuf = std::vector<double>(scount).data();
  const auto rbuf = mbuf.data() + mbuf_begin[phase] - mpi_bufs.rbuf_size(phase);

  /// @todo(vatai): Have a single sbuf of size max(scount[phase]) and
  /// reuse it every time!

  // Copy data to send buffers.
  for (auto i = 0; i < scount; i++)
    sbuf[i] = mbuf[sbuf_idcs[i]];

  const auto offset = mpi_bufs.npart * phase;
  const auto sendcounts = mpi_bufs.sendcounts.data() + offset;
  const auto recvcounts = mpi_bufs.recvcounts.data() + offset;
  const auto sdispls = mpi_bufs.sdispls.data() + offset;
  const auto rdispls = mpi_bufs.rdispls.data() + offset;
  MPI_Alltoallv(sbuf, sendcounts, sdispls, MPI_DOUBLE, //
                rbuf, recvcounts, rdispls, MPI_DOUBLE, MPI_COMM_WORLD);

  // do_init()
}

void buffers_t::exec() {
  const auto nphases = mbuf_begin.size();
  std::vector<double> mbuf(mbuf_idx, 0);

  for (auto i = 0; i < mbuf_begin[0]; i++)
    mbuf[i] = 1.0;

  do_comp(0, mbuf);

  for (auto phase = 1; phase < nphases; phase++) {
    do_comm(phase, mbuf);
    do_comp(phase, mbuf);
  }
  // assert(mcsr.mptr_begin.size() == nphases + 1);
}

void buffers_t::dump(const int rank) {
  std::stringstream fname;
  fname << FNAME << rank;
  std::ofstream file(fname.str(), std::ios::binary);
  double xvar = 3.14;
  file << xvar;
}

void buffers_t::load(const int rank) {
  std::stringstream fname;
  fname << FNAME << rank;
  std::ifstream file(fname.str(), std::ios::binary);
  double xvar = 3.14;
  file >> xvar;
}
