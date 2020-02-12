#pragma once

/// Class converting `argc` and `argv` into C++ types.
#include <metis.h>
#include <string>

struct Args {
  /// Constructor which processes
  /// command line arguments.
  Args(int &argc, char *argv[]);

  /// Path to the `.mtx` file.
  std::string mtxname;

  /// Number of partitions (world size) obtained from environment
  /// variable or command line parameter.
  int npart;

  /// Target number of level.
  int nlevel;

  /// Number of cycles for @ref
  /// CommCompPatterns::ProcAllPhasesCyclePartitions.
  int cycle;

  /// Metis options.
  idx_t opt[METIS_NOPTIONS];
};
