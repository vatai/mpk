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

void buffers_t::phase_init() {
  mpi_bufs.alloc_mpi_bufs();
  mpi_bufs.sbuf_idcs.rec_begin();
  mpi_bufs.init_idcs.rec_begin();

  mcsr.mptr.rec_begin();
  mbuf.begin.push_back(mbuf_idx);
}

void buffers_t::phase_finalize(const int phase) {
  // Fill displacement buffers from count buffers.
  mpi_bufs.fill_displs(phase);

  // Allocate `sbuf_idcs` for this phase.
  mpi_bufs.sbuf_idcs.resize(mpi_bufs.sbuf_idcs.size() +
                            mpi_bufs.sbuf_size(phase));

  const auto rbuf_size = mpi_bufs.rbuf_size(phase);
  // Update `mbuf_idx`.
  mbuf_idx += rbuf_size;

  // Update `mcol`.
  const auto mptr_begin = mcsr.mptr.begin[phase];
  /// @todo(vatai): Investigate why can `mcsr.mptr.begin[phase]` be
  /// (greater or) equal to `mcsr.mptr.size()` which caused the
  /// `out-of-range` exception below.
  if (mptr_begin < mcsr.mptr.size()) {
    const auto mcol_begin = mcsr.mptr[mptr_begin]; // out-of-range
    const auto mcol_end = mcsr.mcol.size();
    const auto mbuf_begin_idx = mbuf.begin[phase];
    for (size_t t = mcol_begin; t < mcol_end; t++) {
      /// @todo(vatai): Make clear how this is supposed to work.
      if ((size_t)mcsr.mcol[t] < mbuf_begin_idx and mcsr.mcol[t] != -1) {
        mcsr.mcol[t] += rbuf_size;
      }
    }
  }
}

void buffers_t::do_comp(int phase) {
  auto mcount = mcsr.mptr.begin[phase + 1] - //
                mcsr.mptr.begin[phase];
  auto cur_mbuf = mbuf.get_ptr(phase) + mpi_bufs.rbuf_size(phase);
  auto cur_mptr = mcsr.mptr.get_ptr(phase);
  for (size_t mi = 0; mi < mcount; mi++) {
    double tmp = cur_mbuf[mi];
    for (auto mj = cur_mptr[mi]; mj < cur_mptr[mi + 1]; mj++) {
      tmp += mcsr.mval[mj] * mbuf[mcsr.mcol[mj]];
    }
    cur_mbuf[mi] += tmp;
  }
}

void buffers_t::do_comm(int phase, std::ofstream &os) {
  /// @todo(vatai): Have a single sbuf of size max(scount[phase]) and
  /// reuse it every time!
  const auto scount = mpi_bufs.sbuf_size(phase);
  double *sbuf = new double[scount];

  // fill_sbuf()
  const auto sbuf_idcs = mpi_bufs.sbuf_idcs.get_ptr(phase);
  for (size_t i = 0; i < scount; i++) {
    assert(0 <= sbuf_idcs[i]);
    assert(sbuf_idcs[i] < (int)mbuf.begin[phase]);
    assert(mbuf[sbuf_idcs[i]] != 0);
    sbuf[i] = mbuf[sbuf_idcs[i]];
  }

  // call_mpi()
  const auto offset = mpi_bufs.npart * phase;
  const auto sendcounts = mpi_bufs.sendcounts.data() + offset;
  const auto recvcounts = mpi_bufs.recvcounts.data() + offset;
  const auto sdispls = mpi_bufs.sdispls.data() + offset;
  const auto rdispls = mpi_bufs.rdispls.data() + offset;

  double *rbuf = mbuf.get_ptr(phase);

  os << "rbuf(before)(" << phase << "): ";
  for (int i = -1; i < (int)mpi_bufs.rbuf_size(phase); i++)
    os << rbuf[i] << ", ";
  os << std::endl;

  // assert(mpi_bufs.rbuf_size(phase) == 0 or mbuf.begin[phase] < mbuf.size());
  MPI_Alltoallv(sbuf, sendcounts, sdispls, MPI_DOUBLE, //
                rbuf, recvcounts, rdispls, MPI_DOUBLE, MPI_COMM_WORLD);

  os << "rbuf        (" << phase << "): ";
  for (int i = -1; i < (int)mpi_bufs.rbuf_size(phase); i++)
    os << rbuf[i] << ", ";
  os << std::endl;

  // do_init()
  const auto begin = mpi_bufs.init_idcs.begin[phase];
  const auto end = mpi_bufs.init_idcs.begin[phase + 1];
  for (auto i = begin; i < end; i++) {
    const auto pair = mpi_bufs.init_idcs[i];
    /// @todo(vatai): Don't forget about adjusting init_idcv[i].second
    /// += rbuf_size; somewhere...
    assert(mbuf[pair.first] != 0.0);
    const auto tgt_idx = pair.second + mpi_bufs.rbuf_size(phase) - 1;
    if (tgt_idx >= mbuf.size()) {
      int rank;
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);
      std::cout << phase << "@"
                << rank << ": "
                << tgt_idx << ", "
                << mbuf.size() << std::endl;
    }
    // assert(tgt_idx < mbuf.size()); // crash!
    // assert(mbuf[tgt_idx] == 0.0); // crash!
    mbuf[tgt_idx] = mbuf[pair.first];
  }

  delete[] sbuf;
}

void buffers_t::exec() {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  auto const fname = DBG_FNAME + std::to_string(rank) + ".txt";
  std::ofstream file(fname);

  const auto nphases = mbuf.begin.size();
  assert(mcsr.mptr.begin.size() == nphases + 1);
  mbuf.resize(mbuf_idx, 0);

  // Load vector!
  for (size_t i = 0; i < mbuf.begin[0]; i++)
    mbuf[i] = 1.0;

  do_comp(0);
  for (size_t phase = 1; phase < nphases; phase++) {
    do_comm(phase, file);
    do_comp(phase);
  }

  std::cout << "exec(" << rank << ")" << std::endl;
}

void buffers_t::dump(const int rank) {
  const auto fname = FNAME + std::to_string(rank) + ".bin";
  std::ofstream file(fname, std::ios::binary);

  file.write((char*)&mbuf_idx, sizeof(mbuf_idx));
  Utils::dump_vec(mbuf.begin, file);
  mpi_bufs.dump_to_ofs(file);
  mcsr.dump_to_ofs(file);
}

void buffers_t::load(const int rank) {
  const auto fname = FNAME + std::to_string(rank) + ".bin";
  std::ifstream file(fname, std::ios::binary);

  file.read((char*)&mbuf_idx , sizeof(mbuf_idx));
  Utils::load_vec(mbuf.begin, file);
  mpi_bufs.load_from_ifs(file);
  mcsr.load_from_ifs(file);
}

void buffers_t::dump_txt(const int rank) {
  const auto fname = FNAME + std::to_string(rank) + ".txt";
  std::ofstream file(fname);
  // mbuf_idx
  file << "mbuf_idx: " << mbuf_idx << std::endl;
  Utils::dump_txt("mbuf_begin", mbuf.begin, file);
  mpi_bufs.dump_to_txt(file);
  mcsr.dump_to_txt(file);
}
