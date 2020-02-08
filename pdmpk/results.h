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
  /// Fill the `val[]` array: val[i] = mbuf[idcs[i]].
  ///
  /// @param idcs Indices of `mbuf` which need to be put into @ref Results::val.
  ///
  /// @param mbuf A buffer of source values.
  void FillVal(const std::vector<idx_t> &idcs, const std::vector<double> &mbuf);
  /// Fill `results[vect_idx[i]] = val[i]`.  Used in `pdmpk_test`.
  ///
  /// @param[out] results Array of length @ref CSR.n which will be
  /// filled.
  void FillResults(std::vector<double> *results);
  /// Save index in `vect_idx`.
  ///
  /// @param idx Index saved in `vect_idx`.
  void SaveIndex(const int &idx);
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

private:
  const Args args;             ///< Arguments from the command-line.
  std::vector<idx_t> vect_idx; ///< `vect_idx[i]` is the index of `val[i]`.
  std::vector<double> val;     ///< `val[i]` is the value at
                               ///`vect_idx[i]` of the final result.
  /// Generate a filename from `args`, `rank` and `ext`.
  ///
  /// @param rank MPI rank of the buffer.
  ///
  /// @param ext File extension appended to the filename.
  std::string Filename(const int &rank, const std::string &ext) const;
};
