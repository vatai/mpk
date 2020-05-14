//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-19

#include <algorithm>
#include <fstream>

#include "mpi_buffers.h"
#include "utils.hpp"

MpiBuffers::MpiBuffers(const idx_t &npart) : npart{npart} {}

void MpiBuffers::PhaseInit() {
  ResizeMpiBufs();
  sbuf_idcs.rec_phase_begin();
  init_idcs.rec_phase_begin();
}

void MpiBuffers::PhaseFinalize() {
  const size_t offset = sendcounts.size() - npart;
  const auto scount = sendcounts.data() + offset;
  const auto rcount = recvcounts.data() + offset;
  const auto sdispl = sdispls.data() + offset;
  const auto rdispl = rdispls.data() + offset;
  sdispl[0] = 0;
  rdispl[0] = 0;
  for (idx_t i = 1; i < npart; i++) {
    sdispl[i] = sdispl[i - 1] + scount[i - 1];
    rdispl[i] = rdispl[i - 1] + rcount[i - 1];
  }
}

void MpiBuffers::SortInitIdcs() {
  std::sort(std::begin(init_idcs) + init_idcs.phase_begin.back(),
            std::end(init_idcs),
            [](const sidx_tidx_t &a, const sidx_tidx_t &b) {
              return a.second < b.second;
            });
}

size_t MpiBuffers::SbufSize(const int &phase) const {
  size_t idx = npart * phase + npart - 1;
  return sendcounts[idx] + sdispls[idx];
}

size_t MpiBuffers::RbufSize(const int &phase) const {
  size_t idx = npart * phase + npart - 1;
  return recvcounts[idx] + rdispls[idx];
}

void MpiBuffers::ResizeMpiBufs() {
  const auto size = recvcounts.size() + npart;
  recvcounts.resize(size);
  sendcounts.resize(size);
  rdispls.resize(size);
  sdispls.resize(size);
}

void MpiBuffers::DumpToOFS(std::ofstream &ofs) const {
  Utils::DumpVec(sendcounts, ofs);
  Utils::DumpVec(sdispls, ofs);
  Utils::DumpVec(recvcounts, ofs);
  Utils::DumpVec(rdispls, ofs);
  Utils::DumpVec(sbuf_idcs, ofs);
  Utils::DumpVec(sbuf_idcs.phase_begin, ofs);
  Utils::DumpVec(init_idcs, ofs);
  Utils::DumpVec(init_idcs.phase_begin, ofs);
}

void MpiBuffers::LoadFromIFS(std::ifstream &ifs) {
  Utils::LoadVec(ifs, &sendcounts);
  Utils::LoadVec(ifs, &sdispls);
  Utils::LoadVec(ifs, &recvcounts);
  Utils::LoadVec(ifs, &rdispls);
  Utils::LoadVec(ifs, &sbuf_idcs);
  Utils::LoadVec(ifs, &sbuf_idcs.phase_begin);
  Utils::LoadVec(ifs, &init_idcs);
  Utils::LoadVec(ifs, &init_idcs.phase_begin);
}

void MpiBuffers::DumpToTxt(std::ofstream &ofs) const {
  Utils::DumpTxt("sendcounts", sendcounts, ofs);
  Utils::DumpTxt("sdispls   ", sdispls, ofs);
  Utils::DumpTxt("recvcounts", recvcounts, ofs);
  Utils::DumpTxt("rdispls   ", rdispls, ofs);
  Utils::DumpTxt("sbuf_idcs", sbuf_idcs, ofs);
  Utils::DumpTxt("sbuf_idcs.begin", sbuf_idcs.phase_begin, ofs);
  Utils::DumpTxt("init_idcs", init_idcs, ofs);
  Utils::DumpTxt("init_idcs.begin", init_idcs.phase_begin, ofs);
}
