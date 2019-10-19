/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-09-17
 *
 * @todo(vatai): Separate MPI and CSR buffers.
 *
 * @todo(vatai): Consider how to implement the temporary and the final
 * versions of MPI and CSR buffers.
 *
 * @todo(vatai): Probably make the CSR and MPI buffers with phases,
 * and then use the same, but only for one phase.
 *//**
 * @brief The buffers collected on a single partition.
 *
 * @details `buffers_t` contains the MPI buffers, the modified CSP
 * buffers and `mbuf`, `sbuf_idx` and `ibuF`.
 */
#pragma once

#include <forward_list>
#include <vector>
#include <utility>
#include <map>

#include <metis.h>

#include "typedefs.h"
#include "mpi_bufs_t.h"

// static void mpk_exec_bufs_val(buffers_t *bufs) {
//   do_task(bufs, 0);
//   for (int phase = 1; phase <= bufs->nphase; phase++) {
//     do_comm(phase, bufs);
//     do_task(bufs, phase);
//   }
// }

class buffers_t {
public:
  buffers_t();
  mpi_bufs_t final_mpi_bufs;
  /// - CSR (one over all phases):
  ///   - `mptr` (`mptr_begin`)
  ///   - `mcol`
  ///   - `mval` (or `mval_idx`)
  std::vector<idx_t> mptr;        // CSR
  std::vector<size_t> mptr_begin; // CSR
  std::vector<idx_t> mcol; // CSR
  std::vector<double> mval; // CSR

  /// - BUF (one over all phases):
  ///   - `mbuf` (`mbuf_begin`, **uses** `mptr_begin` from csr)
  ///   - `sbuf_idcs` (**uses** `sendcount` and `sdispls`)
  ///   - `ibuf` (`ibuf_begin`)
  // std::vector<std::pair<idx_t, level_t>> pair_mbuf;
  idx_t mbuf_idx;

  void record_phase();

  void dump(const int rank);
  void load(const int rank);
};
