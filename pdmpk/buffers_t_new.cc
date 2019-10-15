#include <iostream>

#include "buffers_t_new.h"

buffers_t_new::buffers_t_new(const idx_t npart)
    : npart {npart},
      mcsr_bufs(npart)
{
  std::cout << "basement" << std::endl;
}

void buffers_t_new::add_to_mptr(const size_t rank, const idx_t val)
{
  mcsr_bufs[rank].mptr_count[phase]++;
  mcsr_bufs[rank].mptr.push_back(val);
}

void buffers_t_new::add_to_mcol(const size_t rank, const idx_t val)
{
  mcsr_bufs[rank].mcol.push_back(val);
}

void buffers_t_new::pre_phase()
{
  std::cout << "pre_phase()" << std::endl;
  for (int r = 0; r < npart; r++) {
    auto &buf = mcsr_bufs[r];
    buf.mptr_count.push_back(0);
  }
}

void buffers_t_new::post_phase(comm_dict_t &cd)
{
  std::cout << "post_phase()" << std::endl;
  cd.process();
  // insert {recv,send}{count,displs}
  // implant rbuf, sbuf, ibuf
  cd.clear();
}
