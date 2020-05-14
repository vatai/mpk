/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-09-17

/// @brief The buffers collected on a single partition.
///
/// @details @ref Buffers contains the MPI buffers, the modified CSR
/// buffers @ref Buffers::mcsr, @ref Buffers::mbuf and ibuf.

#pragma once

#include <fstream>
#include <map>
#include <mpi.h>
#include <utility>
#include <vector>

#include <metis.h>

#include "args.h"
#include "mcsr.h"
#include "mpi_buffers.h"
#include "phase_descriptor.h"
#include "phased_vector.hpp"
#include "results.h"
#include "timing.h"
#include "typedefs.h"

/// The main buffers containing information/patterns how to perform
/// the computation and communication for each partition.
class Buffers {
public:
  /// Only constructor for @ref Buffers.
  ///
  /// @param args Arguments passed to the program.
  Buffers(const Args &args);

  /// Code executed before each batch for a single buffer.
  void PreBatch();

  /// Code executed after each batch for a single buffer.
  ///
  /// @param batch The index of the batch processed.
  void PostBatch(const int &batch);

  /// Execute the computations and communication for all phases.
  void Exec(Timing *timing);

  /// Execute the computation for one phase.
  void DoComp(const int &phase);

  /// Execute the communication for one phase.
  void DoComm(const int &phase);

  /// Async execute the computations and communication for all phases.
  void AsyncExec();

  /// Async execute the communication for one phase.
  void AsyncDoComm(const int &phase, const level_t &lvl);

  /// Send calculated vertices "home", where the corresponding input
  /// vector values were in the initial phase (so calling exec again
  /// would continue calculating further powers).
  void SendHome();

  /// Load the values of the input vector based on the
  void LoadInput();

  /// Store @ref Buffers to disk which should be loaded using @ref
  /// Buffers::Load.
  ///
  /// @param rank MPI rank.
  void Dump(const int &rank) const;

  /// Load @ref Buffers from disk saved using @ref Buffers::Dump.
  ///
  /// @param rank MPI rank.
  void Load(const int &rank);

  /// Store @ref Buffers to disk in `.txt` format.
  ///
  /// @param rank MPI rank.
  void DumpTxt(const int &rank) const;

  /// Store @ref Buffers::mbuf to disk in `.txt` format.
  ///
  /// @param rank MPI rank.
  void DumpMbufTxt(const int &rank) const;

  /// Check @ref Buffers invariants.
  void DbgCheck() const;

  /// Delete created files
  ///
  /// @param rank MPI rank.
  void CleanUp(const int &rank) const;

  /// Return the number of phases.
  int GetNumPhases() const;

  /// MPI related buffers: {send,recv}counts, {s,r}displs, sbuf_idcs,
  /// init_idcs.
  MpiBuffers mpi_bufs;

  /// The largest send buffer size needed in all phases.
  size_t max_sbuf_size;

  /// Send buffer used by MPI.
  std::vector<double> sbuf;

  /// (Modified) CSR, which will be used for the
  /// execution/computation.
  Mcsr mcsr;

  /// The index (in `mbuf`) where "the next" vertex will be stored
  /// (basically corresponding to the the size of `mbuf`).  `mbuf_idx`
  /// is updated in the `pdmpk_prep` program, and used to allocate
  /// `mbuf` in the `pdmpk_exec` program.
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

  /// Information needed to reconstruct the result from the output of
  /// the partitions (store ind Buffers).
  Results results;

  /// mbuf indices (from the last phase) where the corresponding value
  /// appears in the first phase, i.e. `mbuf[i] = mbuf[home_idcs[i]]`
  /// (@see Buffers::SendHome).
  std::vector<idx_t> home_idcs;

  /// Indices of the original vector, which are sent to the current
  /// @ref Buffers (when the algorithm starts).
  std::vector<idx_t> original_idcs;

  /// The @ref PhaseDescriptor for each phase.
  std::vector<PhaseDescriptor> phase_descriptors;

  /// @todo(vatai): Delete this.
  std::vector<size_t> dbg_idx; ///< Debug data.

private:
  /// Arguments passed to the main program.
  const Args &args;

  /// Disabled default constructor.
  Buffers();

  /// MPI request objects to synchronise communication.
  std::vector<MPI_Request> requests;
};
