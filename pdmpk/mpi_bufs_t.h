#pragma once

#include <iostream>
#include <ostream>
#include <vector>

#include <metis.h>

#include "typedefs.h"

/**
 * Buffers for the `MPI_alltoallv` call for each partition.
 */
struct mpi_bufs_t {
  /// `npart` is the number of partitions.
  mpi_bufs_t (const idx_t npart);

  void clear();
  void fill_displs();
  /// Output the contents of the MPI buffers.
  friend std::ostream &operator<<(std::ostream &os, const mpi_bufs_t &bufs);

  std::vector<std::vector<int>> recvbuf;
  std::vector<std::vector<int>> sendbuf;
  std::vector<std::vector<int>> recvcount;
  std::vector<std::vector<int>> sendcount;
  std::vector<std::vector<int>> recvdispl;
  std::vector<std::vector<int>> senddispl;

  idx_t npart;
};

