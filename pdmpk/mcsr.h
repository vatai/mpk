/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-10-20

#pragma once

#include <fstream>
#include <metis.h>
#include <vector>

#include "phased_vector.hpp"

/// Modified CSR, containing information/patterns how to perform the
/// computations for each partition.
class Mcsr {
public:
  /// Return the size of an mptr buffer.
  size_t MptrSize(const int &phase) const;

  /// Inserts the size of `mcol` at the end `mptr` vector.
  void NextMcolIdxToMptr();

  /// Dump the contents to a binary `fstream`.
  void DumpToOFS(std::ofstream &ofs);

  /// Load the contents from a binary `fstream`.
  void LoadFromIFS(std::ifstream &ifs);

  /// Dump to a txt file.
  void DumpToTxt(std::ofstream &ofs);

  /// The modified `ptr` array @see CSR::ptr.
  phased_vector<idx_t> mptr;

  /// The modified `col` array @see CSR::col.
  std::vector<idx_t> mcol;

  /// The modified `val` array @see CSR::val.
  std::vector<double> mval;
};
