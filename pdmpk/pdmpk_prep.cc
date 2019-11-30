/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-09-17

/// @page pdmpk_prep pdmpk_prep
///
/// The body of `pdmpk_prep` page.

#include <cassert>
#include <iostream>

#include "typedefs.h"
#include "partial_cd.h"

int main(int argc, char *argv[])
{
  assert(argc == 4);
  int npart = std::stoi(argv[2]);
  level_t nlevels = std::stoi(argv[3]);

  partial_cd pcd(argv[1], (idx_t)npart, nlevels);

  for (int i = 0; i < npart; i++) {
    pcd.bufs[i].dump(i);
    pcd.bufs[i].dump_txt(i);
  }

  return 0;
}
