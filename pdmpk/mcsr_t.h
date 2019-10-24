/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-10-20
 */
#pragma once

#include <vector>
#include <metis.h>

/**
 * Modified CSR for with `mptr_begin`.
 */

/// - CSR (one over all phases):
///   - `mptr` (`mptr_begin`): DONE
///   - `mcol`: DONE
///   - `mval` (or `mval_idx`)
class mcsr_t {
 public:
  void rec_mptr_begin();
  void mcol_push_back(const idx_t idx);

  std::vector<idx_t> mptr;
  std::vector<idx_t> mptr_begin;
  std::vector<idx_t> mcol;
  std::vector<double> mval;
};
