//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-19

#include <fstream>
#include "utils.hpp"
#include "mpi_buffers.h"

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
  Utils::dump_vec(sendcounts, ofs);
  Utils::dump_vec(sdispls, ofs);
  Utils::dump_vec(recvcounts, ofs);
  Utils::dump_vec(rdispls, ofs);
  Utils::dump_vec(sbuf_idcs, ofs);
  Utils::dump_vec(sbuf_idcs.begin, ofs);
  Utils::dump_vec(init_idcs, ofs);
  Utils::dump_vec(init_idcs.begin, ofs);
}

void MPIBuffers::LoadFromIFS(std::ifstream &ifs) {
  Utils::load_vec(sendcounts, ifs);
  Utils::load_vec(sdispls, ifs);
  Utils::load_vec(recvcounts, ifs);
  Utils::load_vec(rdispls, ifs);
  Utils::load_vec(sbuf_idcs, ifs);
  Utils::load_vec(sbuf_idcs.begin, ifs);
  Utils::load_vec(init_idcs, ifs);
  Utils::load_vec(init_idcs.begin, ifs);
}

void MPIBuffers::DumpToTxt(std::ofstream &ofs) {
  Utils::dump_txt("sendcounts", sendcounts, ofs);
  Utils::dump_txt("sdispls   ", sdispls, ofs);
  Utils::dump_txt("recvcounts", recvcounts, ofs);
  Utils::dump_txt("rdispls   ", rdispls, ofs);
  Utils::dump_txt("sbuf_idcs", sbuf_idcs, ofs);
  Utils::dump_txt("sbuf_idcs.begin", sbuf_idcs.begin, ofs);
  Utils::dump_txt("init_idcs", init_idcs, ofs);
  Utils::dump_txt("init_idcs.begin", init_idcs.begin, ofs);
}
