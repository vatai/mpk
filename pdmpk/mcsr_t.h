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
  void rec_mptr();
  /// Dump the contents to a binary `fstream`.
  void dump_to_ofs(std::ofstream &ofs);
  /// Load the contents from a binary `fstream`.
  void load_from_ifs(std::ifstream &ifs);

  std::vector<idx_t> mptr;
  std::vector<idx_t> mptr_begin;
  std::vector<idx_t> mcol;
  std::vector<double> mval;
};
