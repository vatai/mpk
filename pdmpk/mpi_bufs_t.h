/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-10-19
 */

#pragma once

#include <fstream>
#include <vector>
#include <metis.h>

#include "typedefs.h"
#include "phased_vector.hpp"

/// MPI buffers, containing information/patterns how to perform the
/// communication for each partition.
class mpi_bufs_t {
 public:
  mpi_bufs_t(const idx_t npart);
  /// Fill displacement buffers (`sdispls` and `rdispls`) from the
  /// count buffers (`sendcount` and `recvcount`).
  void fill_displs(int phase);
  /// Get `rbuf` size from `recvcount` and `rdispls`.
  size_t sbuf_size(int phase) const;
  /// Get `sbuf` size form `sendcount` and `sdispls`.
  size_t rbuf_size(int phase) const;
  /// Allocate {send,recv}counts and {s,r}displs.
  void alloc_mpi_bufs();
  /// Dump the contents to a binary `fstream`.
  void dump_to_ofs(std::ofstream &ofs);
  /// Load the contents from a binary `fstream`.
  void load_from_ifs(std::ifstream &ifs);
  /// Dump to a txt file.
  void dump_to_txt(std::ofstream &ofs);

  /// - MPI (one for each phase):
  ///   - `sendcount` and `recvcount`
  ///   - `sdispls` and `rdispls`
  std::vector<int> sendcounts;
  std::vector<int> recvcounts;
  std::vector<int> sdispls;
  std::vector<int> rdispls;
  /// `mbuf` indices, which need to be copied to the send buffer.
  phased_vector<idx_t> sbuf_idcs;
  /// `mbuf` source-target pairs, to initialise mbuf elements.
  phased_vector<sidx_tidx_t> init_idcs;

  /// @todo(vatai): It would be nice to "remove" this.
  const idx_t npart;

 private:
  mpi_bufs_t();
};
