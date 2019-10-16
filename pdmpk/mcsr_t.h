/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-10-15
 *
 * Modified CSR (for one partition/rank).  A vector `npart` number of
 * `mcsr_t` is stored, and written to the disk in the end.  In phase
 * `p`, `mptr_count[p]` elements of `mptr[]` are processed starting
 * from `mptr_offset[p]`.
 */

#pragma once
#include <vector>

#include <metis.h>

class mcsr_t {
public:
  std::vector<idx_t> mptr;
  std::vector<size_t> mptr_count;
  std::vector<size_t> mptr_offset;

  std::vector<idx_t> mcol;
  std::vector<idx_t> mval_idx;
};
