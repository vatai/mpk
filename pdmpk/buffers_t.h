/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-09-17
 *
 * @brief The buffers collected on a single partition.
 *
 * @details `buffers_t` contains the MPI buffers, the modified CSP
 * buffers and `mbuf` and `ibuF`.
 */
#pragma once

#include <forward_list>
#include <fstream>
#include <vector>
#include <utility>
#include <map>

#include <metis.h>

#include "typedefs.h"
#include "mpi_bufs_t.h"
#include "mcsr_t.h"
#include "phased_vector.hpp"

/// @todo(vatai): Document `buffers_t`.
class buffers_t {
 public:
  buffers_t(const idx_t npart);
  void phase_finalize(const int phase);

  void exec();
  void do_comp(int phase);
  void do_comm(int phase, std::ofstream &os);
  void dump(const int rank);
  void load(const int rank);
  void dump_txt(const int rank);

  /// MPI related buffers: {send,recv}counts, {s,r}displs, sbuf_idcs,
  /// init_idcs.
  mpi_bufs_t mpi_bufs;
  /// (Modified) CSR, which will be used for the
  /// execution/computation.
  mcsr_t mcsr;

  /// The index (in `mbuf`) where "the next" vertex will be stored.
  /// `mbuf_idx` is updated in the `pdmpk_prep` program, and used to
  /// allocate `mbuf` in the `pdmpk_exec` program.
  idx_t mbuf_idx;

  /// The buffer where the to store the results of the computation
  /// (including the part of the input assigned to this partition, the
  /// intermediate results, the `rbuf`s and the final results).  It is
  /// only used in the `pdmpk_exec` program.
  /// For each `phase`, `mbuf.begin[phase]` is the index (of `mbuf`)
  /// of the first element which will be calculated in the given
  /// phase.  The `rbuf` of the corresponding phase, starts at
  /// `mbuf.begin[phase] - mpi_bufs.rbuf_size(phase)`.
  phased_vector<double> mbuf;

 private:
  buffers_t();
};
