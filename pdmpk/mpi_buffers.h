/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-10-19

#pragma once

#include <fstream>
#include <metis.h>
#include <vector>

#include "phased_vector.hpp"
#include "typedefs.h"

/// MPI buffers, containing information/patterns how to perform the
/// communication for each partition.
class MPIBuffers {
public:
  MPIBuffers(const idx_t &npart);

  /// Fill displacement buffers (`sdispls` and `rdispls`) from the
  /// count buffers (`sendcount` and `recvcount`).
  void FillDispls(const int &phase);

  /// Get `rbuf` size from `recvcount` and `rdispls`.
  size_t SbufSize(const int &phase) const;
  /// Get `sbuf` size form `sendcount` and `sdispls`.
  size_t RbufSize(const int &phase) const;
  /// Allocate {send,recv}counts and {s,r}displs.
  void AllocMpiBufs();
  /// Dump the contents to a binary `fstream`.
  void DumpToOFS(std::ofstream &ofs);
  /// Load the contents from a binary `fstream`.
  void LoadFromIFS(std::ifstream &ifs);
  /// Dump to a txt file.
  void DumpToTxt(std::ofstream &ofs);

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
  MPIBuffers();
};
