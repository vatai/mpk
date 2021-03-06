/// @file
/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-09-17

/// @page pdmpk_prep pdmpk_prep
///
/// The body of `pdmpk_prep` page.

#include <cassert>
#include <iostream>

#include "../pdmpk/args.h"
#include "../pdmpk/comm_comp_patterns.h"
#include "../pdmpk/typedefs.h"

/// Prepare @ref Buffers using pdMPK and save them to disk.
///
/// @param argc Three parameters should be provided.
///
/// @param argv
/// - `argv[1]` is the name of the `.mtx` file;
/// - `argv[2]` is the number of partitions/processes to be used;
/// - `argv[3]` is the number of levels which needs to be achieved.
int main(int argc, char *argv[]) {
  const Args args(argc, argv);
  CommCompPatterns comm_comp_patterns(args);

  comm_comp_patterns.Stats();

  for (int i = 0; i < args.npart; i++) {
    comm_comp_patterns.bufs[i].DbgCheck();
    comm_comp_patterns.bufs[i].Dump(i);
    comm_comp_patterns.bufs[i].DumpTxt(i);
  }

  std::cout << argv[0] << ": for " << args.mtxname << " finished" << std::endl;

  return 0;
}
