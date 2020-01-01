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

#include "args.h"
#include "buffers.h"
#include "csr.h"
#include "results.h"
#include "utils.hpp"

/// @page pdmpk_test pdmpk_test
///
/// Body of `pdmpk_test` page.

const double kEpsilon = 1e-09;

/// Test the results of @ref pdmpk_exec. The arguments are the same as
/// for @ref pdmpk_prep.c::main
int main(int argc, char *argv[]) {
  const Args args(argc, argv);
  CSR csr(args.mtxname);

  // Calculate "gold" result.
  std::vector<double> goldResult(csr.n);
  for (auto &v : goldResult)
    v = 1.0;
  csr.MPK(args.nlevels, goldResult);

  // Get finalResult.
  std::vector<double> loadResult(csr.n);
  for (auto i = 0; i < args.npart; i++) {
    Results results(args.mtxname);
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
  std::cout << argv[0] << ": Maximum absolute error: " << max << std::endl;
  std::cout << argv[0]
            << ": Maximum absolute is zero: " << (max == 0.0 ? "true" : "false")
            << std::endl;
  std::cout << argv[0] << ": for " << argv[1] << " finished" << std::endl;
  return max < kEpsilon ? 0 : 1;
}
