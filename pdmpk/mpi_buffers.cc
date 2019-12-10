//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-19

#include <fstream>

#include "mpi_buffers.h"
#include "utils.hpp"

MPIBuffers::MPIBuffers(const idx_t npart) : npart{npart} {}

void MPIBuffers::FillDispls(int phase) {
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

size_t MPIBuffers::SbufSize(int phase) const {
  size_t idx = npart * phase + npart - 1;
  return sendcounts[idx] + sdispls[idx];
}

size_t MPIBuffers::RbufSize(int phase) const {
  size_t idx = npart * phase + npart - 1;
  return recvcounts[idx] + rdispls[idx];
}

void MPIBuffers::AllocMpiBufs() {
  const auto size = recvcounts.size() + npart;
  recvcounts.resize(size);
  sendcounts.resize(size);
  rdispls.resize(size);
  sdispls.resize(size);
}

void MPIBuffers::DumpToOFS(std::ofstream &ofs) {
  Utils::DumpVec(sendcounts, ofs);
  Utils::DumpVec(sdispls, ofs);
  Utils::DumpVec(recvcounts, ofs);
  Utils::DumpVec(rdispls, ofs);
  Utils::DumpVec(sbuf_idcs, ofs);
  Utils::DumpVec(sbuf_idcs.phase_begin, ofs);
  Utils::DumpVec(init_idcs, ofs);
  Utils::DumpVec(init_idcs.phase_begin, ofs);
}

void MPIBuffers::LoadFromIFS(std::ifstream &ifs) {
  Utils::LoadVec(sendcounts, ifs);
  Utils::LoadVec(sdispls, ifs);
  Utils::LoadVec(recvcounts, ifs);
  Utils::LoadVec(rdispls, ifs);
  Utils::LoadVec(sbuf_idcs, ifs);
  Utils::LoadVec(sbuf_idcs.phase_begin, ifs);
  Utils::LoadVec(init_idcs, ifs);
  Utils::LoadVec(init_idcs.phase_begin, ifs);
}

void MPIBuffers::DumpToTxt(std::ofstream &ofs) {
  Utils::DumpTxt("sendcounts", sendcounts, ofs);
  Utils::DumpTxt("sdispls   ", sdispls, ofs);
  Utils::DumpTxt("recvcounts", recvcounts, ofs);
  Utils::DumpTxt("rdispls   ", rdispls, ofs);
  Utils::DumpTxt("sbuf_idcs", sbuf_idcs, ofs);
  Utils::DumpTxt("sbuf_idcs.begin", sbuf_idcs.phase_begin, ofs);
  Utils::DumpTxt("init_idcs", init_idcs, ofs);
  Utils::DumpTxt("init_idcs.begin", init_idcs.phase_begin, ofs);
}
