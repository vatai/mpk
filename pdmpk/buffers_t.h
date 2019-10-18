/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-09-17
 *
 * @todo(vatai): Separate MPI and CSR buffers.
 *
 * @todo(vatai): Consider how to implement the temporary and the final
 * versions of MPI and CSR buffers.
 */

/**
 * The buffers collected on a single partition.
 *
 * `buffers_t` contains the MPI buffers, the modified CSP buffers and
 * `mbuf`, `sbuf_idx` and `ibuF`.
 */
#pragma once

#include <forward_list>
#include <vector>
#include <utility>
#include <map>

#include <metis.h>

#include "typedefs.h"

// static void mpk_exec_bufs_val(buffers_t *bufs) {
//   do_task(bufs, 0);
//   for (int phase = 1; phase <= bufs->nphase; phase++) {
//     do_comm(phase, bufs);
//     do_task(bufs, phase);
//   }
// }

class buffers_t {
public:
  std::vector<int> recvcounts; // MPI
  std::vector<int> sendcounts; // MPI
  std::vector<int> rdispls;  // MPI
  std::vector<int> sdispls;  // MPI
  std::vector<idx_t> sbuf_idx;   // MPI

  std::vector<std::pair<idx_t, level_t>> pair_mbuf;

  std::vector<idx_t> mptr;  // CSR
  std::vector<size_t> mptr_begin; // CSR
  std::vector<idx_t> mcol; // CSR
  std::vector<double> mval; // CSR

  void record_phase();

  void dump(const int rank);
  void load(const int rank);
};
