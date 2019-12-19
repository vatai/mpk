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
    Results results;
    results.Load(i);
    const size_t size = results.val.size();
    for (size_t i = 0; i < size; i++) {
      auto idx = results.vect_idx[i];
      loadResult[idx] = results.val[i];
    }
  }

  double max = 0;
  for (size_t i = 0; i < loadResult.size(); i++) {
    auto diff = loadResult[i] - goldResult[i];
    if (diff < 0.0)
      diff = -diff;
    if (max < diff)
      max = diff;
  }
  std::cout << "pdmpk_test: Maximum absolute error: " << max << std::endl;
  std::cout << "pdmpk_test: Maximum absolute is zero: "
            << (max == 0.0 ? "true" : "false") << std::endl;
  return 0;
}
