/// @file
/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-09-17

/// @page pdmpk_prep pdmpk_prep
///
/// The body of `pdmpk_prep` page.

#include <cassert>
#include <iostream>

#include "comm_comp_patterns.h"
#include "typedefs.h"

int main(int argc, char *argv[]) {
  assert(argc == 4);
  int npart = std::stoi(argv[2]);
  level_t nlevels = std::stoi(argv[3]);

  CommCompPatterns comm_comp_patterns(argv[1], (idx_t)npart, nlevels);

  for (int i = 0; i < npart; i++) {
    comm_comp_patterns.bufs[i].Dump(i);
    comm_comp_patterns.bufs[i].DumpTxt(i);
  }

  std::cout << "pdmpk_prep done" << std::endl;

  return 0;
}
