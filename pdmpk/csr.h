/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-09-17

#pragma once

#include <fstream>
#include <string>
#include <vector>

#include <metis.h>

class CSR {
public:
  /// Construct a matrix by reading it from a file.
  ///
  /// @param fname The name of an `.mtx` file.
  CSR(const std::string &fname);
  /// Execute an SpMV operation on the given vector and the value
  /// represented by @ref ptr, @ref col and @ref val.
  ///
  /// @param vec The input vector.
  ///
  /// @return The vector which is the result of the matrix vector
  /// product.
  std::vector<double> SpMV(const std::vector<double> &vec) const;
  /// Preform a naive MPK.
  ///
  /// @param nlevels Number of levels the MPK should calculate.
  ///
  /// @param vec Input vector which will both as n output vector.
  void MPK(const int nlevels, std::vector<double> &vec) const;
  /// Number of vertices.
  idx_t n;
  /// Number of non-zero entries.
  idx_t nnz;
  /// The usual `ptr` array of the CSR representation.
  std::vector<idx_t> ptr;
  /// The usual `col` array of the CSR representation.
  std::vector<idx_t> col;
  /// The usual `val` array of the CSR representation.
  std::vector<double> val;

private:
  bool symmetric;
  bool coordinate;
  void MtxCheckBanner(std::ifstream &file);
  void MtxFillSize(std::ifstream &file);
  void MtxFillVectors(std::ifstream &file);
};
