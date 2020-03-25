#pragma once

#include <metis.h>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

/// Class converting `argc` and `argv` into C++ types.
struct Args {
  /// Constructor which processes
  /// command line arguments.
  Args(int argc, char *argv[]);

  /// Generate filename based on arguments.
  ///
  /// @param suffix appended to the filename.
  ///
  /// @param rank MPI rank of the buffer or -1 if no buffer rank needs to be
  /// appended.
  ///
  /// @returns The string containing relevant data from the `Args`
  /// object (and the MPI rank optionally).
  std::string Filename(const std::string &suffix, const int &rank = -1) const;

  /// Convert the `Args` to a JSON format.
  ///
  /// @returns Relevant data from `Args` in JSON format.
  json ToJson() const;

  /// Path to the `.mtx` file.
  std::string mtxname;

  /// Number of partitions (world size) obtained from environment
  /// variable or command line parameter.
  int npart;

  /// Target number of level.
  int nlevel;

  /// Index to select the mirror method.
  size_t mirror_method;

  /// Index to select the weight update method.
  size_t weight_update_method;

  /// Metis options.
  idx_t opt[METIS_NOPTIONS];

  /// Metis default options used to determine which options were set.
  idx_t default_opt[METIS_NOPTIONS];

  /// If true keep files = don't cleanup flag.
  bool keepfiles;

private:
  /// Gets relevant args from environment variables.
  void GetEnvArgs();

  /// Read args using `getopt()`.
  void ReadArgs(int argc, char *argv[]);
};
