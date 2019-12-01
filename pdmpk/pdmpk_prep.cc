/// @file
/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-09-17

/// @page pdmpk_prep pdmpk_prep
///
/// The body of `pdmpk_prep` page.

#include <cassert>
#include <iostream>

#include "typedefs.h"
#include "comm_comp_patterns.h"

int main(int argc, char *argv[])
{
  assert(argc == 4);
  int npart = std::stoi(argv[2]);
  level_t nlevels = std::stoi(argv[3]);

  CommCompPatterns pcd(argv[1], (idx_t)npart, nlevels);

  for (int i = 0; i < npart; i++) {
    pcd.bufs[i].Dump(i);
    pcd.bufs[i].DumpTxt(i);
  }

  return 0;
}
