/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-10-14
 */

#pragma once

#include <vector>

#include <metis.h>

#include "comm_dict_t.h"
#include "csr_t.h"
#include "mcsr_t.h"

/**
 * Buffers collecting computation and communication patterns.
 */
class buffers_t_new {
 public:
  buffers_t_new(const idx_t _npart);
  void pre_phase();
  void post_phase(comm_dict_t &cd);
  void add_to_mptr(const size_t rank, const idx_t val);
  void add_to_mcol(const size_t rank, const idx_t val);

  int phase;

  // @todo(vatai): record_phase() ??? sbuf_offset.push_back(sbuf.size()) etc
 private:
  const idx_t npart;
  std::vector<mcsr_t> mcsr_bufs;
};
