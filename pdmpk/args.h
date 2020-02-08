#pragma once

/// Class converting `argc` and `argv` into C++ types.
#include <metis.h>
#include <string>

struct Args {
  Args(int &argc, char *argv[]); ///< Constructor which processes
                                 /// command line arguments.
  std::string mtxname;           ///< Path to the `.mtx` file.
  int npart;  ///< Number of partitions (world size) obtained from
              /// environment variable or command line parameter.
  int nlevel; ///< Target number of level.
  idx_t opt[METIS_NOPTIONS]; ///< Metis options.
};
