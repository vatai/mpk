/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-10-20
 */
#pragma once

#include <fstream>
#include <vector>
#include <metis.h>

#include "phased_vector.hpp"

/// @todo(vatai): Document `mcsr_t`
class mcsr_t {
 public:
  void rec_mptr();
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
