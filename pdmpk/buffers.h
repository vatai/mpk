/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-09-17

/// @brief The buffers collected on a single partition.
///
/// @details `Buffers` contains the MPI buffers, the modified CSP
/// buffers and `mbuf` and `ibuF`.

#pragma once

#include <fstream>
#include <map>
#include <utility>
#include <vector>

#include <metis.h>

#include "mcsr.h"
#include "mpi_buffers.h"
#include "phased_vector.hpp"
#include "results.h"
#include "typedefs.h"

/// The main buffers containing information/patterns how to perform
/// the computation and communication for each partition.
class Buffers {
public:
  Buffers(const idx_t npart);
  /// Initialise/Allocate appropriate space in buffers to store
  /// communication data.
  void PhaseInit();
  void PhaseFinalize(const int phase);

  void Exec(const int rank);
  void DoComp(int phase);
  /// Execute the communication needed at the beginning of each phase
  /// (before calling the `DoComp()` method).
  void DoComm(int phase);
  void Dump(const int rank);
  void Load(const int rank);
  void DumpTxt(const int rank);
  void DumpMbufTxt(const int rank);

  /// MPI related buffers: {send,recv}counts, {s,r}displs, sbuf_idcs,
  /// init_idcs.
  MPIBuffers mpi_bufs;

  /// The largest send buffer size needed in all phases.
  size_t max_sbuf_size;

  /// Send buffer used by MPI.
  std::vector<double> sbuf;

  /// (Modified) CSR, which will be used for the
  /// execution/computation.
  MCSR mcsr;

  /// The index (in `mbuf`) where "the next" vertex will be stored.
  /// `mbuf_idx` is updated in the `pdmpk_prep` program, and used to
  /// allocate `mbuf` in the `pdmpk_exec` program.
  idx_t mbuf_idx;

  /// The buffer which stores the results of the computation
  /// (including the part of the input assigned to this partition, the
  /// intermediate results, the `rbuf`s and the final results).  It is
  /// used only in the `pdmpk_exec` program.
  ///
  /// The last `mpi_bufs.rbuf_size(phase)` number of elements of
  /// `mbuf[phase]` represent `rbuf[phase]`.  The elements which need
  /// to be calculated start at `mbuf.begin[phase] +
  /// mpi_bufs.rbuf_size(phase)`.
  ///
  /// As a special case, `rbuf[0]` contains the elements of the input
  /// vector assigned to the given partition.  Also (unlike for `phase
  /// > 0`) mbuf.begin[0] points to the element after the last element
  /// of `rbuf[phase]` while `mpi_bufs.rbuf_size(0) == 0`, so the rule
  /// that the first element which should be calculated is at
  /// `mbuf.begin[phase] + mpi_bufs.rbuf_size(phase)` is not violated.
  phased_vector<double> mbuf;

  /// Holds the (vector index, `mbuf` index) pairs where the vertices
  /// at level `nlevel` can be found in the given partition.
  std::vector<idx_t> results_mbuf_idx;

  /// Information needed to reconstruct the result from the output of
  /// the partitions (store ind Buffers).
  Results results;

  std::vector<size_t> dbg_idx;

private:
  Buffers();
};
