/// @file
/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-11-28

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <unistd.h>
#include <vector>

#include "../pdmpk/args.h"
#include "../pdmpk/buffers.h"
#include "../pdmpk/csr.h"
#include "../pdmpk/results.h"
#include "../pdmpk/utils.hpp"

/// @page pdmpk_check pdmpk_check
///
/// Body of `pdmpk_check` page.

/// "Infinitesimaly" small value, under which we consider two floats
/// equal.
const double kEpsilon = 1e-09;

/// Calculate the cosine of vectors `v1` and `v2`.
///
/// @param v1 First vecotor `v1`.
///
/// @param v2 Second vector `v2`.
double Cos(const std::vector<double> &v1, const std::vector<double> &v2) {
  const auto n = v1.size();
  assert(n == v2.size());
  double sum = 0.0;
  double n1 = 0.0;
  double n2 = 0.0;
  for (size_t i = 0; i < n; i++) {
    sum += v1[i] * v2[i];
    n1 += v1[i] * v1[i];
    n2 += v2[i] * v2[i];
  }
  return (sum * sum) / (n1 * n2);
}

/// Test the results of @ref pdmpk_exec. The arguments are the same as
/// for @ref prep.c::main
int main(int argc, char *argv[]) {
  const Args args(argc, argv);
  Csr csr(args.mtxname);

  // Calculate "gold" result.
  std::vector<double> goldResult(csr.n);
  for (auto &v : goldResult)
    v = 1.0;
  csr.MPK(args.nlevel, goldResult);

  // Get finalResult.
  std::vector<double> loadResult(csr.n);
  for (auto i = 0; i < args.npart; i++) {
    Results results(args);
    results.Load(i);
    results.FillResults(&loadResult);
  }

  double max = 0;
  for (size_t i = 0; i < loadResult.size(); i++) {
    auto diff = loadResult[i] - goldResult[i];
    if (diff < 0.0)
      diff = -diff;
    if (max < diff)
      max = diff;
  }
  const auto cos = Cos(loadResult, goldResult);
  std::cout << argv[0] << ": Maximum absolute error: " << max << std::endl;
  std::cout << argv[0]
            << ": Maximum absolute is zero: " << (max == 0.0 ? "true" : "false")
            << std::endl;
  std::cout << argv[0] << ": for " << args.mtxname << " finished" << std::endl;
  std::cout << argv[0] << ": Cos(): " << cos << std::endl;
  // return max < kEpsilon ? 0 : 1;
  return !(1 - kEpsilon < cos and cos < 1 + kEpsilon);
}
