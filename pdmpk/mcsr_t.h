/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-10-20
 */
#pragma once

#include <fstream>
#include <vector>
#include <metis.h>

#include "phased_vector.hpp"

/// Modified CSR, containing information/patterns how to perform the
/// computations for each partition.
class mcsr_t {
 public:
  /// Inserts the size of `mcol` at the end `mptr` vector.
  void next_mcol_idx_to_mptr();
  /// Dump the contents to a binary `fstream`.
  void dump_to_ofs(std::ofstream &ofs);
  /// Load the contents from a binary `fstream`.
  void load_from_ifs(std::ifstream &ifs);
  /// Dump to a txt file.
  void dump_to_txt(std::ofstream &ofs);

  phased_vector<idx_t> mptr;
  std::vector<idx_t> mcol;
  std::vector<double> mval;
};
