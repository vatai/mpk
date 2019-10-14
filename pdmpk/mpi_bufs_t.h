#pragma once

#include <iostream>
#include <ostream>
#include <vector>

#include <metis.h>

#include "typedefs.h"

/**
 * Temporary buffers for the `MPI_alltoallv` call for each partition.
 * These will be combined.
 */
struct mpi_bufs_t {
  /// `npart` is the number of partitions.
  mpi_bufs_t (const idx_t npart);

  void add_svect(const idx_t src, const idx_t tgt, const std::vector<idx_t> &src_idcs);
  void add_ivect(const idx_t src, const idx_t tgt, const idx_t idx);
  void fill_displs();
  /// Clear all containers.
  void clear();
  /// Output the contents of the MPI buffers.
  friend std::ostream &operator<<(std::ostream &os, const mpi_bufs_t &bufs);

  std::vector<std::vector<int>> recvcount;
  std::vector<std::vector<int>> sendcount;
  std::vector<std::vector<int>> recvdispl;
  std::vector<std::vector<int>> senddispl;

  /// Relative indices in the target partition which need to be
  /// modified after `combine()`.
  std::vector<std::vector<int>> recvbuf;

  /// The indices of the source partition `mbuf`, which need to be
  /// copied to the send buffer.
  std::vector<std::vector<int>> sendidcs;

  /// Number of partitions, or "world size" in MPI terminology.
  idx_t npart;
};
