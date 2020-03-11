#pragma once

/// Class converting `argc` and `argv` into C++ types.
#include <metis.h>
#include <string>

struct Args {
  /// Type of files for which we can generate filenames.
  enum FileType {
    kBinBuffer,
    kBinResult,
    kTxtBuffer,
    kTxtResult,
    kTxtMbuf,
    kStatusFileName,
    kStatusContents
  };

  /// Constructor which processes
  /// command line arguments.
  Args(int &argc, char *argv[]);

  /// Generate filename.
  ///
  /// @param rank MPI rank of the buffer.
  ///
  /// @param filetype @ref FileType.
  std::string Filename(const int &rank, const FileType &filetype) const;

  /// Generate run summary.
  ///
  /// @param fullpath When set to `true` returns the full path, not
  /// just the filename and other params.
  std::string Summary(const bool fullpath) const;

  /// Path to the `.mtx` file.
  std::string mtxname;

  /// Number of partitions (world size) obtained from environment
  /// variable or command line parameter.
  int npart;

  /// Target number of level.
  int nlevel;

  /// Metis options.
  idx_t opt[METIS_NOPTIONS];
};
