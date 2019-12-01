/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-11-28

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <unistd.h>
#include <vector>

#include "Results.h"
#include "Buffers.h"
#include "csr_t.h"
#include "utils.hpp"

/// @page pdmpk_test pdmpk_test
///
/// Body of `pdmpk_test` page.

int main(int argc, char *argv[])
{
  assert(argc == 4);
  csr_t csr(argv[1]);
  const int npart = std::stoi(argv[2]);
  const int nlevels = std::stoi(argv[3]);

  // Calculate "gold" result.
  std::vector<double> goldResult(csr.n);
  for (auto &v : goldResult)
    v = 1.0;
  csr.mpk(nlevels, goldResult);

  // Get finalResult.
  std::vector<double> loadResult(csr.n);
  for (auto i = 0; i < npart; i++) {
    Results results;
    results.Load(i);
    const size_t size = results.val.size();
    for (size_t i = 0; i < size; i++) {
      auto idx = results.vectIdx[i];
      loadResult[idx] = results.val[i];
    }
  }
  assert(loadResult == goldResult);

  std::cout << "Test over" << std::endl;
  return 0;
}
