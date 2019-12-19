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

/// Prepare @ref Buffers using pdMPK and save them to disk.
///
/// @param argc Three parameters should be provided.
///
/// @param argv
/// - `argv[1]` is the name of the `.mtx` file;
/// - `argv[2]` is the number of partitions/processes to be used;
/// - `argv[3]` is the number of levels which needs to be achieved.
int main(int argc, char *argv[]) {
  assert(argc == 4);
  int npart = std::stoi(argv[2]);
  level_t nlevels = std::stoi(argv[3]);

  CommCompPatterns comm_comp_patterns(argv[1], (idx_t)npart, nlevels);
  comm_comp_patterns.Stats(argv[1]);

  for (int i = 0; i < npart; i++) {
    comm_comp_patterns.bufs[i].Dump(i);
    comm_comp_patterns.bufs[i].DumpTxt(i);
  }

  std::cout << "pdmpk_prep done" << std::endl;

  return 0;
}
