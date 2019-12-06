// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2019-10-17

#include <cassert>
#include <fstream>
#include <iostream>
#include <metis.h>
#include <mpi.h>
#include <string>
#include <vector>

#include "buffers.h"
#include "typedefs.h"
#include "utils.hpp"

const std::string FNAME{"bufs"};
const std::string DBG_FNAME{"dbg_buff_"};

Buffers::Buffers(const idx_t npart)
    : mpi_bufs{npart}, max_sbuf_size{0}, mbuf_idx{0} {}

void Buffers::PhaseInit() {
  mpi_bufs.AllocMpiBufs();
  mpi_bufs.sbuf_idcs.rec_begin();
  mpi_bufs.init_idcs.rec_begin();

  mcsr.mptr.rec_begin();
  mbuf.begin.push_back(mbuf_idx);
}

void Buffers::PhaseFinalize(const int phase) {
  // Fill displacement buffers from count buffers.
  mpi_bufs.FillDispls(phase);
  const auto rbuf_size = mpi_bufs.RbufSize(phase);
  const auto sbuf_size = mpi_bufs.SbufSize(phase);

  if (max_sbuf_size < sbuf_size) {
    max_sbuf_size = sbuf_size;
  }

  // Allocate `sbuf_idcs` for this phase.
  mpi_bufs.sbuf_idcs.resize(mpi_bufs.sbuf_idcs.size() + sbuf_size);

  // Update `mbuf_idx`.
  mbuf_idx += rbuf_size;
}

void Buffers::DoComp(int phase) {
  auto mcount = mcsr.mptr.begin[phase + 1] - //
                mcsr.mptr.begin[phase];
  auto cur_mbuf = mbuf.get_ptr(phase);
  auto cur_mptr = mcsr.mptr.get_ptr(phase);
  for (size_t mi = 0; mi < mcount; mi++) {
    double tmp = cur_mbuf[mi];
    for (auto mj = cur_mptr[mi]; mj < cur_mptr[mi + 1]; mj++) {
      tmp += mcsr.mval[mj] * mbuf[mcsr.mcol[mj]];
    }
    cur_mbuf[mi] = tmp;
  }
}

void Buffers::DoComm(int phase, std::ofstream &os) {
  /// @todo(vatai): Remove asserts and clean up.
  const auto scount = mpi_bufs.SbufSize(phase);

  // fill_sbuf()
  const auto sbuf_idcs = mpi_bufs.sbuf_idcs.get_ptr(phase);
  for (size_t i = 0; i < scount; i++) {
    assert(0 <= sbuf_idcs[i]);
    assert(sbuf_idcs[i] < (int)mbuf.begin[phase]);
    sbuf[i] = mbuf[sbuf_idcs[i]];
  }

  // call mpi()
  const auto offset = mpi_bufs.npart * phase;
  const auto sendcounts = mpi_bufs.sendcounts.data() + offset;
  const auto recvcounts = mpi_bufs.recvcounts.data() + offset;
  const auto sdispls = mpi_bufs.sdispls.data() + offset;
  const auto rdispls = mpi_bufs.rdispls.data() + offset;

  /// @todo(vatai): Convert `rbuf` to span.
  auto rbuf = mbuf.get_ptr(phase) + mcsr.MptrSize(phase);

  // ////// Debug //////
  os << "rbuf(before)(" << phase << "): ";
  for (int i = -1; i < (int)mpi_bufs.RbufSize(phase); i++)
    os << rbuf[i] << ", ";
  os << std::endl;

  assert(mpi_bufs.RbufSize(phase) == 0 or mbuf.begin[phase] < mbuf.size());
  MPI_Alltoallv(sbuf.data(), sendcounts, sdispls, MPI_DOUBLE, //
                rbuf, recvcounts, rdispls, MPI_DOUBLE, MPI_COMM_WORLD);

  // ////// Debug //////
  os << "rbuf        (" << phase << "): ";
  for (int i = -1; i < (int)mpi_bufs.RbufSize(phase); i++)
    os << rbuf[i] << ", ";
  os << std::endl;

  // DoInit()
  /// @todo(vatai): Implement get_init_span.
  const auto begin = mpi_bufs.init_idcs.begin[phase];
  const auto end = mpi_bufs.init_idcs.begin[phase + 1];
  for (auto i = begin; i < end; i++) {
    const auto &pair = mpi_bufs.init_idcs[i];
    const auto tgtIdx = pair.second;
    assert(tgtIdx < (int)mbuf.size()); // crash!
    assert(mbuf[tgtIdx] == 0.0);       // crash!
    mbuf[tgtIdx] = mbuf[pair.first];
  }
}

void Buffers::Exec(const int rank) {
  auto const fname = DBG_FNAME + std::to_string(rank) + ".txt";
  std::ofstream file(fname);

  const auto nphases = mbuf.begin.size();
  assert(mcsr.mptr.begin.size() == nphases + 1);
  mbuf.resize(mbuf_idx, 0);
  sbuf.resize(max_sbuf_size);

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
  file.write((char *)&max_sbuf_size, sizeof(max_sbuf_size));
  file.write((char *)&mbuf_idx, sizeof(mbuf_idx));
  Utils::DumpVec(mbuf.begin, file);
  Utils::DumpVec(results_mbuf_idx, file);
  Utils::DumpVec(results.vect_idx, file);
  mpi_bufs.DumpToOFS(file);
  mcsr.DumpToOFS(file);
}

void Buffers::Load(const int rank) {
  std::ifstream file(FNAME + std::to_string(rank) + ".bin", std::ios::binary);
  file.read((char *)&max_sbuf_size, sizeof(max_sbuf_size));
  file.read((char *)&mbuf_idx, sizeof(mbuf_idx));
  Utils::LoadVec(mbuf.begin, file);
  Utils::LoadVec(results_mbuf_idx, file);
  Utils::LoadVec(results.vect_idx, file);
  mpi_bufs.LoadFromIFS(file);
  mcsr.LoadFromIFS(file);
}

void Buffers::DumpTxt(const int rank) {
  std::ofstream file(FNAME + std::to_string(rank) + ".txt");
  // mbuf_idx
  file << "max_sbuf_size: " << max_sbuf_size << std::endl;
  file << "mbuf_idx: " << mbuf_idx << std::endl;
  Utils::DumpTxt("mbuf_begin", mbuf.begin, file);
  Utils::DumpTxt("result_mbuf_idx", results_mbuf_idx, file);
  Utils::DumpTxt("result_vect_idx", results.vect_idx, file);
  Utils::DumpTxt("dbg_idx", dbg_idx, file);
  mpi_bufs.DumpToTxt(file);
  mcsr.DumpToTxt(file);
}

void Buffers::DumpMbufTxt(const int rank) {
  std::ofstream file("dresult" + std::to_string(rank) + ".txt");
  Utils::DumpTxt("mbuf", mbuf, file);
}
