#pragma once

/// Class converting `argc` and `argv` into C++ types.
#include <metis.h>
#include <string>

struct Args {
  Args(int &argc, char *argv[]);
  std::string mtxname;
  int npart;
  int nlevel;
  idx_t opt[METIS_NOPTIONS];
};
