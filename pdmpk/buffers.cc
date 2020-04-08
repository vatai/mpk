// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2019-10-17

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <metis.h>
#include <mpi.h>
#include <string>
#include <vector>

#include "buffers.h"
#include "results.h"
#include "typedefs.h"
#include "utils.hpp"

const std::string kFname{"bufs"};

Buffers::Buffers(const Args &args)
    : mpi_bufs{args.npart}, //
      max_sbuf_size{0},     //
      mbuf_idx{0},          //
      results(args),        //
      args{args} {}

void Buffers::PhaseInit() {
  mpi_bufs.ResizeMpiBufs();
  mpi_bufs.sbuf_idcs.rec_phase_begin();
  mpi_bufs.init_idcs.rec_phase_begin();

  mcsr.mptr.rec_phase_begin();
  mbuf.phase_begin.push_back(mbuf_idx);
}

void Buffers::PhaseFinalize(const int &phase) {
  // Fill displacement buffers from count buffers.
  mpi_bufs.FillDispls();
  const auto sbuf_size = mpi_bufs.SbufSize(phase);

  if (max_sbuf_size < sbuf_size) {
    max_sbuf_size = sbuf_size;
  }

  // Allocate `sbuf_idcs` for this phase.
  mpi_bufs.sbuf_idcs.resize(mpi_bufs.sbuf_idcs.size() + sbuf_size);

  // Update `mbuf_idx`.
  mbuf_idx += mpi_bufs.RbufSize(phase);
}

void Buffers::DoComp(const int &phase) {
  auto mcount = mcsr.mptr.phase_begin[phase + 1] - //
                mcsr.mptr.phase_begin[phase];
  auto cur_mbuf = mbuf.get_ptr(phase);
  auto cur_mptr = mcsr.mptr.get_ptr(phase);

  auto init_idx = mpi_bufs.init_idcs.phase_begin[phase];
  for (size_t mi = 0; mi < mcount; mi++) {
    double tmp = 0.0;
    const auto &pair = mpi_bufs.init_idcs[init_idx];
    if (mpi_bufs.init_idcs.size() > 0 and
        init_idx < mpi_bufs.init_idcs.phase_begin[phase + 1] and
        mbuf.phase_begin[phase] + mi == (size_t)pair.second) {
      tmp = mbuf[pair.first];
      init_idx++;
    };
    for (auto mj = cur_mptr[mi]; mj < cur_mptr[mi + 1]; mj++) {
      tmp += mcsr.mval[mj] * mbuf[mcsr.mcol[mj]];
    }
    cur_mbuf[mi] = tmp;
  }
}

void Buffers::DoComm(const int &phase) {
  const auto scount = mpi_bufs.SbufSize(phase);

  // fill_sbuf()
  const auto sbuf_idcs = mpi_bufs.sbuf_idcs.get_ptr(phase);
  for (size_t i = 0; i < scount; i++) {
    sbuf[i] = mbuf[sbuf_idcs[i]];
  }

  // call mpi()
  const auto offset = mpi_bufs.npart * phase;
  const auto sendcounts = mpi_bufs.sendcounts.data() + offset;
  const auto recvcounts = mpi_bufs.recvcounts.data() + offset;
  const auto sdispls = mpi_bufs.sdispls.data() + offset;
  const auto rdispls = mpi_bufs.rdispls.data() + offset;

  auto rbuf = mbuf.get_ptr(phase) + mcsr.MptrSize(phase);
  MPI_Alltoallv(sbuf.data(), sendcounts, sdispls, MPI_DOUBLE, //
                rbuf, recvcounts, rdispls, MPI_DOUBLE, MPI_COMM_WORLD);
}

void Buffers::SendHome() {
  const auto size = home_idcs.size();
  for (size_t i = 0; i < size; i++) {
    mbuf[i] = mbuf[home_idcs[i]];
  }
}

void Buffers::Exec() {
  const auto nphases = mbuf.phase_begin.size();
  // mcsr.mptr has one more "phase_begin"s because there is one added
  // in the Epilogue() to make processing the same.
  assert(mcsr.mptr.phase_begin.size() == nphases + 1);

  DoComp(0);
  for (size_t phase = 1; phase < nphases; phase++) {
    DoComm(phase);
    DoComp(phase);
  }
  SendHome();
}

void Buffers::LoadInput() {
  // Load vector!
  for (size_t i = 0; i < mbuf.phase_begin[0]; i++)
    mbuf[i] = 1.0;
}

void Buffers::Dump(const int &rank) const {
  std::ofstream file(args.Filename("buf.bin", rank), std::ios::binary);
  file.write((char *)&max_sbuf_size, sizeof(max_sbuf_size));
  file.write((char *)&mbuf_idx, sizeof(mbuf_idx));
  Utils::DumpVec(mbuf.phase_begin, file);
  Utils::DumpVec(home_idcs, file);
  Utils::DumpVec(results.mbuf_idcs, file);
  results.Dump(rank);
  mpi_bufs.DumpToOFS(file);
  mcsr.DumpToOFS(file);
}

void Buffers::Load(const int &rank) {
  std::ifstream file(args.Filename("buf.bin", rank), std::ios::binary);
  file.read((char *)&max_sbuf_size, sizeof(max_sbuf_size));
  sbuf.resize(max_sbuf_size);

  file.read((char *)&mbuf_idx, sizeof(mbuf_idx));
  mbuf.resize(mbuf_idx, 0);

  Utils::LoadVec(file, &mbuf.phase_begin);
  Utils::LoadVec(file, &home_idcs);
  Utils::LoadVec(file, &results.mbuf_idcs);
  results.Load(rank);
  mpi_bufs.LoadFromIFS(file);
  mcsr.LoadFromIFS(file);
}

void Buffers::DumpTxt(const int &rank) const {
  std::ofstream file(args.Filename("buf.txt", rank));
  // mbuf_idx
  file << "max_sbuf_size: " << max_sbuf_size << std::endl;
  file << "mbuf_idx: " << mbuf_idx << std::endl;
  Utils::DumpTxt("mbuf.phase_begin", mbuf.phase_begin, file);
  Utils::DumpTxt("home_idcs", home_idcs, file);
  Utils::DumpTxt("result.mbuf_idcs", results.mbuf_idcs, file);
  results.DumpTxt(rank);
  Utils::DumpTxt("dbg_idx", dbg_idx, file);
  mpi_bufs.DumpToTxt(file);
  mcsr.DumpToTxt(file);
}

void Buffers::DumpMbufTxt(const int &rank) const {
  std::ofstream file(args.Filename("mbuf.txt", rank));

  Utils::DumpTxt("mbuf.phase_begin", mbuf.phase_begin, file);
  Utils::DumpTxt("mbuf", mbuf, file);

  mpi_bufs.DumpToTxt(file);
}

void Buffers::DbgCheck() const {
  // mpi_bufs !!!!!
  // mpi_bufs.init_idcs; // !!!
  // mpi_bufs.sbuf_idcs; // !!!
  // mpi_bufs.npart;
  // mpi_bufs.rdispls;
  // mpi_bufs.recvcounts;
  // mpi_bufs.sdispls;
  // mpi_bufs.sendcounts;
  // max_sbuf_size

  // mcsr !!!!!
  // mcsr.mcol; // !!!
  // mcsr.mptr; // !!!
  // mbuf_idx
  // results_mbuf_idx
  // results

  // dbg_idx??

  // name
  // mbuf
  // sbuf
}

void Buffers::CleanUp(const int &rank) const {
  if (not args.keepfiles) {
    std::remove(args.Filename("buf.bin", rank).c_str());
    std::remove(args.Filename("buf.txt", rank).c_str());
    std::remove(args.Filename("mbuf.txt", rank).c_str());
  }
}
