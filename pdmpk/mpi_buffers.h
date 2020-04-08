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
class MpiBuffers {
public:
  /// Constructor.
  ///
  /// @param npart number of partitions.
  MpiBuffers(const idx_t &npart);

  /// Call @ref MpiBuffers::ResizeMpiBufs, and `rec_phase_begin` for
  /// `sbuf_idcs` and `init_idcs`.
  void PhaseInit();

  /// Fill the last `npart` number of entries in the displacement
  /// buffers (`sdispls` and `rdispls`) from the data in the count
  /// buffers (`sendcount` and `recvcount`).
  void PhaseFinalize();

  /// Sort @ref MpiBuffers::init_idcs;
  void SortInitIdcs();

  /// Get `rbuf` size from `recvcount` and `rdispls`.
  size_t SbufSize(const int &phase) const;

  /// Get `sbuf` size form `sendcount` and `sdispls`.
  size_t RbufSize(const int &phase) const;

  /// Dump the contents to a binary `fstream`.
  void DumpToOFS(std::ofstream &ofs) const;

  /// Load the contents from a binary `fstream`.
  void LoadFromIFS(std::ifstream &ifs);

  /// Dump to a txt file.
  void DumpToTxt(std::ofstream &ofs) const;

  std::vector<int> sendcounts; ///< MPI send count array.
  std::vector<int> recvcounts; ///< MPI recieve count array.
  std::vector<int> sdispls;    ///< MPI send displcaement array.
  std::vector<int> rdispls;    ///< MPI recieve displacement array.

  /// Indices into @ref Buffers::mbuf which need to be copied to the
  /// send buffer.
  phased_vector<idx_t> sbuf_idcs;

  /// Source-target index pairs into @ref Buffers::mbuf, to initialise
  /// mbuf elements.
  phased_vector<sidx_tidx_t> init_idcs;

  const idx_t npart; ///< Number of partitions/processors.

  /// @todo(vatai): It would be nice to remove `npart` from
  /// `MpiBuffers`.
private:
  MpiBuffers();

  /// Allocate additional npart elements to {send,recv}counts and
  /// {s,r}displs.
  void ResizeMpiBufs();
};
