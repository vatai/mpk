/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-12-01

#pragma once

#include <metis.h>
#include <string>
#include <vector>

#include "args.h"

/// Class to store the final results of (PD)MPK: the value of the
/// final result at index `vect_idv[i]` is `val[i]`.
class Results {
public:
  /// Constructor.
  ///
  /// @param args Arguments from the command-line.
  Results(const Args &args);
  /// Fill the @ref values array: vales[i] = mbuf[mbuf_idcs[i]].
  ///
  /// @param mbuf A buffer of source values.
  void FillVal(const std::vector<double> &mbuf);
  /// Fill `results[vect_idx[i]] = val[i]`.  Used in `pdmpk_test`.
  ///
  /// @param[out] results Array of length @ref CSR.n which will be
  /// filled.
  void FillResults(std::vector<double> *results);
  /// Save original index - `mbuf` index pair.
  ///
  /// @param idx Original index saved.
  ///
  /// @param mbuf_idx `mbuf` index.
  void SaveIndex(const idx_t &idx, const idx_t &mbuf_idx);
  /// Save the results.
  ///
  /// @param rank MPI rank.
  void Dump(const int &rank);
  /// Load the results.
  ///
  /// @param rank MPI rank.
  void Load(const int &rank);
  /// Save the results as text.
  ///
  /// @param rank MPI rank.
  void DumpTxt(const int &rank);

  /// Holds the `mbuf` index where the vertices at level `nlevel` can
  /// be found in the given partition. This member is public and
  /// serialised by @ref Buffers.
  std::vector<idx_t> mbuf_idcs;

private:
  /// Arguments from the command-line.
  const Args args;
  /// `original_idcs[i]` is the index of `values[i]`.
  std::vector<idx_t> original_idcs;
  /// `values[i]` is the value at `originals_idcs[i]` of the final result.
  std::vector<double> values;

  /// Generate a filename from `args`, `rank` and `ext`.
  ///
  /// @param rank MPI rank of the buffer.
  ///
  /// @param ext File extension appended to the filename.
  std::string Filename(const int &rank, const std::string &ext) const;
};
