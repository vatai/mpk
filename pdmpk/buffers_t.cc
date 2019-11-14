//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-17

#include <cassert>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <mpi.h>
#include <string>
#include <vector>

#include "utils.hpp"
#include "buffers_t.h"

const std::string FNAME{"bufs"};
const std::string DBG_FNAME{"dbg_buff_"};

buffers_t::buffers_t(const idx_t npart) : mpi_bufs(npart), mbuf_idx(0) {}

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
  const auto mbuf_begin_idx = mbuf_begin[phase];
  for (size_t t = mcol_begin; t < mcol_end; t++) {
    if ((size_t)mcsr.mcol[t] < mbuf_begin_idx and mcsr.mcol[t] != -1)
      mcsr.mcol[t] += rbuf_size;
  }
}

void buffers_t::do_comp(int phase) {
  // assert(phase < mcsr.mptr_begin.size());
  // assert(phase + 1 < mcsr.mptr_begin.size());

  auto mcount = mcsr.mptr_begin[phase + 1] - mcsr.mptr_begin[phase];
  //?? auto mptr = mcsr.mptr.data() + mcsr.mptr_begin[phase];
  //?? Following two lines are from the older `mpk2` code
  //??   long *mcol = bufs->mcol_buf + bufs->mcol_offsets[phase];
  //??   double *mval = bufs->mval_buf + bufs->mcol_offsets[phase];
  auto cur_mbuf = mbuf.data() + mbuf_begin[phase];
  for (auto mi = 0; mi < mcount; mi++) {
    double tmp = cur_mbuf[mi];
    for (auto mj = mcsr.mptr[mi]; mj < mcsr.mptr[mi + 1]; mj++) {
      tmp += mcsr.mval[mj] * mbuf[mcsr.mcol[mj]];
    }
    cur_mbuf[mi] += tmp;
  }
}

void buffers_t::do_comm(int phase, std::ofstream &os) {
  // fill_sbuf()
  const auto scount = mpi_bufs.sbuf_size(phase);
  const auto sbuf_idcs = mpi_bufs.sbuf_idcs.data() + mpi_bufs.sbuf_idcs_begin[phase];

  /// @todo(vatai): Have a single sbuf of size max(scount[phase]) and
  /// reuse it every time!
  double *sbuf = new double[scount];

  for (auto i = 0; i < scount; i++) {
    sbuf[i] = mbuf[sbuf_idcs[i]];
  }

  // call_mpi()
  const auto offset = mpi_bufs.npart * phase;
  const auto sendcounts = mpi_bufs.sendcounts.data() + offset;
  const auto recvcounts = mpi_bufs.recvcounts.data() + offset;
  const auto sdispls = mpi_bufs.sdispls.data() + offset;
  const auto rdispls = mpi_bufs.rdispls.data() + offset;

  // rbuf used by MPI - don't delete
  double *rbuf = mbuf.data() + mbuf_begin[phase];// - mpi_bufs.rbuf_size(phase);

  if (mpi_bufs.rbuf_size(phase) > 0 and mbuf_begin[phase] >= mbuf.size()) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    std::cout << "(@" << rank << ")"
              << "mbuf_begin[phase]: " << mbuf_begin[phase] << ", "
              << "mbuf.size(): " << mbuf.size() << std::endl;
  }

  // if (not (mpi_bufs.rbuf_size(phase) > 0 and mbuf_begin[phase] < mbuf.size())) {
  //   std::cout << mpi_bufs.rbuf_size(phase) << ", "
  //             << mbuf_begin[phase] << ", "
  //             << mbuf.size() << std::endl;
  // }
  os << "rbuf(before)(" << phase << "): ";
  for (int i = 0; i < mpi_bufs.rbuf_size(phase); i++)
    os << rbuf[i] << ", ";
  os << std::endl;

  assert(mpi_bufs.rbuf_size(phase) == 0 or mbuf_begin[phase] < mbuf.size());
  MPI_Alltoallv(sbuf, sendcounts, sdispls, MPI_DOUBLE, //
                rbuf, recvcounts, rdispls, MPI_DOUBLE, MPI_COMM_WORLD);
  os << "rbuf (" << phase << "): ";
  for (int i = 0; i < mpi_bufs.rbuf_size(phase); i++)
    os << rbuf[i] << ", ";
  os << std::endl;

  // do_init()
  const auto begin = mpi_bufs.init_idcs_begin[phase];
  const auto end = mpi_bufs.init_idcs_begin[phase + 1];
  for (auto i = begin; i < end; i++) {
    const auto pair = mpi_bufs.init_idcs[i];
    mbuf[pair.second] = mbuf[pair.first];
  }

  delete[] sbuf;
}

void buffers_t::exec() {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  auto const fname = DBG_FNAME + std::to_string(rank) + ".txt";
  std::ofstream file(fname);

  const auto nphases = mbuf_begin.size();
  mbuf.resize(mbuf_idx, 0);

  std::cout << "exec(" << rank << ")" << std::endl;

  for (size_t i = 0; i < mbuf_begin[0]; i++)
    mbuf[i] = 1.0;

  do_comp(0);

  for (size_t phase = 1; phase < nphases; phase++) {
    do_comm(phase, file);
    do_comp(phase);
  }
  // assert(mcsr.mptr_begin.size() == nphases + 1);
}

void buffers_t::dump(const int rank) {
  const auto fname = FNAME + std::to_string(rank) + ".bin";
  std::ofstream file(fname, std::ios::binary);

  file.write((char*)&mbuf_idx, sizeof(mbuf_idx));
  Utils::dump_vec(mbuf_begin, file);
  mpi_bufs.dump_to_ofs(file);
  mcsr.dump_to_ofs(file);
}

void buffers_t::load(const int rank) {
  const auto fname = FNAME + std::to_string(rank) + ".bin";
  std::ifstream file(fname, std::ios::binary);

  file.read((char*)&mbuf_idx , sizeof(mbuf_idx));
  Utils::load_vec(mbuf_begin, file);
  mpi_bufs.load_from_ifs(file);
  mcsr.load_from_ifs(file);
}

void buffers_t::dump_txt(const int rank) {
  const auto fname = FNAME + std::to_string(rank) + ".txt";
  std::ofstream file(fname);
  // mbuf_idx
  file << "mbuf_idx: " << mbuf_idx << std::endl;
  Utils::dump_txt("mbuf_begin", mbuf_begin, file);
  mpi_bufs.dump_to_txt(file);
  mcsr.dump_to_txt(file);
}
