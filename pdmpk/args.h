#pragma once

/// Class converting `argc` and `argv` into C++ types.
#include <string>

struct Args {
  Args(int &argc, char *argv[]);
  std::string mtxname;
  int npart;
  int nlevels;
};
