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
#include <gsl/span>

#include "typedefs.h"
#include "utils.hpp"
#include "buffers_t.h"

const std::string FNAME{"bufs"};
const std::string DBG_FNAME{"dbg_buff_"};

buffers_t::buffers_t(const idx_t npart)
    : mpi_bufs(npart), max_sbuf_size(0), mbuf_idx(0) {}

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
  const auto rbuf_size = mpi_bufs.rbuf_size(phase);
  const auto sbuf_size = mpi_bufs.sbuf_size(phase);

  if (max_sbuf_size < sbuf_size) {
    max_sbuf_size = sbuf_size;
  }

  // Allocate `sbuf_idcs` for this phase.
  mpi_bufs.sbuf_idcs.resize(mpi_bufs.sbuf_idcs.size() + sbuf_size);

  // Update `mbuf_idx`.
  mbuf_idx += rbuf_size;
}

void buffers_t::do_comp(int phase) {
  auto mcount = mcsr.mptr.begin[phase + 1] - //
                mcsr.mptr.begin[phase];
  auto cur_mbuf = mbuf.get_ptr(phase);
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
  /// @todo(vatai): Remove asserts and clean up.
  const auto scount = mpi_bufs.sbuf_size(phase);

  // fill_sbuf()
  const auto sbuf_idcs = mpi_bufs.sbuf_idcs.get_ptr(phase);
  for (size_t i = 0; i < scount; i++) {
    assert(0 <= sbuf_idcs[i]);
    assert(sbuf_idcs[i] < (int)mbuf.begin[phase]);
    // assert(mbuf[sbuf_idcs[i]] != 0); // mcol2mptr-debug
    sbuf[i] = mbuf[sbuf_idcs[i]];
  }

  // call_mpi()
  const auto offset = mpi_bufs.npart * phase;
  const auto sendcounts = mpi_bufs.sendcounts.data() + offset;
  const auto recvcounts = mpi_bufs.recvcounts.data() + offset;
  const auto sdispls = mpi_bufs.sdispls.data() + offset;
  const auto rdispls = mpi_bufs.rdispls.data() + offset;

  /// @todo(vatai): Convert `rbuf` to span.
  auto rbuf = mbuf.get_ptr(phase) + mcsr.mptr_size(phase);

  // ////// Debug //////
  os << "rbuf(before)(" << phase << "): ";
  for (int i = -1; i < (int)mpi_bufs.rbuf_size(phase); i++)
    os << rbuf[i] << ", ";
  os << std::endl;

  // assert(mpi_bufs.rbuf_size(phase) == 0 or mbuf.begin[phase] < mbuf.size());
  MPI_Alltoallv(sbuf.data(), sendcounts, sdispls, MPI_DOUBLE, //
                rbuf, recvcounts, rdispls, MPI_DOUBLE, MPI_COMM_WORLD);

  // ////// Debug //////
  os << "rbuf        (" << phase << "): ";
  for (int i = -1; i < (int)mpi_bufs.rbuf_size(phase); i++)
    os << rbuf[i] << ", ";
  os << std::endl;

  // do_init()
  /// @todo(vatai): Implement get_init_span.
  const auto begin = mpi_bufs.init_idcs.begin[phase];
  const auto length = mpi_bufs.init_idcs.begin[phase + 1] - begin;
  const auto init = gsl::make_span(mpi_bufs.init_idcs).subspan(begin, length);
  for (const auto pair : init) {
    // assert(mbuf[pair.first] != 0.0); // mcol2mptr-debug
    const auto tgt_idx = pair.second;
    if (tgt_idx >= (int)mbuf.size()) {
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
}

void buffers_t::exec() {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  auto const fname = DBG_FNAME + std::to_string(rank) + ".txt";
  std::ofstream file(fname);

  const auto nphases = mbuf.begin.size();
  assert(mcsr.mptr.begin.size() == nphases + 1);
  mbuf.resize(mbuf_idx, 0);
  sbuf.resize(max_sbuf_size);

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

  file.write((char*)&max_sbuf_size, sizeof(max_sbuf_size));
  file.write((char*)&mbuf_idx, sizeof(mbuf_idx));
  Utils::dump_vec(mbuf.begin, file);
  Utils::dump_vec(result_idx, file);
  mpi_bufs.dump_to_ofs(file);
  mcsr.dump_to_ofs(file);
}

void buffers_t::load(const int rank) {
  const auto fname = FNAME + std::to_string(rank) + ".bin";
  std::ifstream file(fname, std::ios::binary);

  file.read((char*)&max_sbuf_size, sizeof(max_sbuf_size));
  file.read((char*)&mbuf_idx, sizeof(mbuf_idx));
  Utils::load_vec(mbuf.begin, file);
  Utils::load_vec(result_idx, file);
  mpi_bufs.load_from_ifs(file);
  mcsr.load_from_ifs(file);
}

void buffers_t::dump_txt(const int rank) {
  const auto fname = FNAME + std::to_string(rank) + ".txt";
  std::ofstream file(fname);
  // mbuf_idx
  file << "max_sbuf_size: " << max_sbuf_size << std::endl;
  file << "mbuf_idx: " << mbuf_idx << std::endl;
  Utils::dump_txt("mbuf_begin", mbuf.begin, file);
  Utils::dump_txt("result_idx", result_idx, file);
  Utils::dump_txt("dbg_idx", dbg_idx, file);
  mpi_bufs.dump_to_txt(file);
  mcsr.dump_to_txt(file);
}
