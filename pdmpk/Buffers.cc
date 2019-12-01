//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-17

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <metis.h>
#include <mpi.h>

#include "typedefs.h"
#include "utils.hpp"
#include "Buffers.h"

const std::string FNAME{"bufs"};
const std::string DBG_FNAME{"dbg_buff_"};

Buffers::Buffers(const idx_t npart)
    : mpiBufs(npart), maxSbufSize(0), mbufIdx(0) {}

void Buffers::PhaseInit() {
  mpiBufs.alloc_mpi_bufs();
  mpiBufs.sbuf_idcs.rec_begin();
  mpiBufs.init_idcs.rec_begin();

  mcsr.mptr.rec_begin();
  mbuf.begin.push_back(mbufIdx);
}

void Buffers::PhaseFinalize(const int phase) {
  // Fill displacement buffers from count buffers.
  mpiBufs.fill_displs(phase);
  const auto rbufSize = mpiBufs.rbuf_size(phase);
  const auto sbufSize = mpiBufs.sbuf_size(phase);

  if (maxSbufSize < sbufSize) {
    maxSbufSize = sbufSize;
  }

  // Allocate `sbufIdcs` for this phase.
  mpiBufs.sbuf_idcs.resize(mpiBufs.sbuf_idcs.size() + sbufSize);

  // Update `mbufIdx`.
  mbufIdx += rbufSize;
}

void Buffers::DoComp(int phase) {
  auto mcount = mcsr.mptr.begin[phase + 1] - //
                mcsr.mptr.begin[phase];
  auto curMbuf = mbuf.get_ptr(phase);
  auto curMptr = mcsr.mptr.get_ptr(phase);
  for (size_t mi = 0; mi < mcount; mi++) {
    double tmp = curMbuf[mi];
    for (auto mj = curMptr[mi]; mj < curMptr[mi + 1]; mj++) {
      tmp += mcsr.mval[mj] * mbuf[mcsr.mcol[mj]];
    }
    // ////// DEBUG //////
    // int rank;
    // MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // cur_mbuf[mi] = -phase * 1000 - rank * 100 - 42;
    curMbuf[mi] = tmp;
  }
}

void Buffers::DoComm(int phase, std::ofstream &os) {
  /// @todo(vatai): Remove asserts and clean up.
  const auto scount = mpiBufs.sbuf_size(phase);

  // fillSbuf()
  const auto sbufIdcs = mpiBufs.sbuf_idcs.get_ptr(phase);
  for (size_t i = 0; i < scount; i++) {
    assert(0 <= sbufIdcs[i]);
    assert(sbufIdcs[i] < (int)mbuf.begin[phase]);
    sbuf[i] = mbuf[sbufIdcs[i]];
  }

  // call mpi()
  const auto offset = mpiBufs.npart * phase;
  const auto sendcounts = mpiBufs.sendcounts.data() + offset;
  const auto recvcounts = mpiBufs.recvcounts.data() + offset;
  const auto sdispls = mpiBufs.sdispls.data() + offset;
  const auto rdispls = mpiBufs.rdispls.data() + offset;

  /// @todo(vatai): Convert `rbuf` to span.
  auto rbuf = mbuf.get_ptr(phase) + mcsr.mptr_size(phase);

  // ////// Debug //////
  os << "rbuf(before)(" << phase << "): ";
  for (int i = -1; i < (int)mpiBufs.rbuf_size(phase); i++)
    os << rbuf[i] << ", ";
  os << std::endl;

  assert(mpiBufs.rbuf_size(phase) == 0 or mbuf.begin[phase] < mbuf.size());
  MPI_Alltoallv(sbuf.data(), sendcounts, sdispls, MPI_DOUBLE, //
                rbuf, recvcounts, rdispls, MPI_DOUBLE, MPI_COMM_WORLD);

  // ////// Debug //////
  os << "rbuf        (" << phase << "): ";
  for (int i = -1; i < (int)mpiBufs.rbuf_size(phase); i++)
    os << rbuf[i] << ", ";
  os << std::endl;

  // DoInit()
  /// @todo(vatai): Implement get_init_span.
  const auto begin = mpiBufs.init_idcs.begin[phase];
  const auto end = mpiBufs.init_idcs.begin[phase + 1];
  for (auto i = begin; i < end; i++) {
    const auto &pair = mpiBufs.init_idcs[i];
    const auto tgt_idx = pair.second;
    if (tgt_idx >= (int)mbuf.size()) {
      int rank;
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);
      std::cout << phase << "@"
                << rank << ": "
                << tgt_idx << ", "
                << mbuf.size() << std::endl;
    }
    assert(tgt_idx < (int)mbuf.size()); // crash!
    assert(mbuf[tgt_idx] == 0.0); // crash!
    mbuf[tgt_idx] = mbuf[pair.first];
  }
}

void Buffers::Exec() {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  auto const fname = DBG_FNAME + std::to_string(rank) + ".txt";
  std::ofstream file(fname);

  const auto nphases = mbuf.begin.size();
  assert(mcsr.mptr.begin.size() == nphases + 1);
  mbuf.resize(mbufIdx, 0);
  sbuf.resize(maxSbufSize);

  // Load vector!
  for (size_t i = 0; i < mbuf.begin[0]; i++)
    mbuf[i] = 1.0;

  DoComp(0);
  for (size_t phase = 1; phase < nphases; phase++) {
    DoComm(phase, file);
    DoComp(phase);
  }

  results.FillVal(results_mbuf_idx, mbuf);
  std::cout << "exec(" << rank << ")" << std::endl;
}

void Buffers::Dump(const int rank) {
  std::ofstream file(FNAME + std::to_string(rank) + ".bin", std::ios::binary);
  file.write((char *)&maxSbufSize, sizeof(maxSbufSize));
  file.write((char *)&mbufIdx, sizeof(mbufIdx));
  Utils::dump_vec(mbuf.begin, file);
  Utils::dump_vec(results_mbuf_idx, file);
  Utils::dump_vec(results.vectIdx, file);
  mpiBufs.dump_to_ofs(file);
  mcsr.dump_to_ofs(file);
}

void Buffers::Load(const int rank) {
  std::ifstream file(FNAME + std::to_string(rank) + ".bin", std::ios::binary);
  file.read((char *)&maxSbufSize, sizeof(maxSbufSize));
  file.read((char *)&mbufIdx, sizeof(mbufIdx));
  Utils::load_vec(mbuf.begin, file);
  Utils::load_vec(results_mbuf_idx, file);
  Utils::load_vec(results.vectIdx, file);
  mpiBufs.load_from_ifs(file);
  mcsr.load_from_ifs(file);
}

void Buffers::DumpTxt(const int rank) {
  std::ofstream file(FNAME + std::to_string(rank) + ".txt");
  // mbuf_idx
  file << "max_sbuf_size: " << maxSbufSize << std::endl;
  file << "mbuf_idx: " << mbufIdx << std::endl;
  Utils::dump_txt("mbuf_begin", mbuf.begin, file);
  Utils::dump_txt("result_mbuf_idx", results_mbuf_idx, file);
  Utils::dump_txt("result_vect_idx", results.vectIdx, file);
  Utils::dump_txt("dbg_idx", dbg_idx, file);
  mpiBufs.dump_to_txt(file);
  mcsr.dump_to_txt(file);
}

void Buffers::DumpMbufTxt(const int rank) {
  std::ofstream file("dresult" + std::to_string(rank) + ".txt");
  Utils::dump_txt("mbuf", mbuf, file);
}
