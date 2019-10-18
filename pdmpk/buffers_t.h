/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-09-17
 *
 * @todo(vatai): Make yas work on ulp-at59.
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
  std::vector<int> recvcounts;
  std::vector<int> sendcounts;
  std::vector<int> rdispls;
  std::vector<int> sdispls;

  std::vector<std::pair<idx_t, level_t>> pair_mbuf;

  // int *rbuf_offsets;
  // int *mbuf_offsets;
  // int *sbuf_offsets;
  // int *mptr_offsets;
  // int *mcol_offsets;

  std::vector<idx_t> sbuf;
  std::vector<size_t> sbuf_offset;
  std::vector<size_t> sbuf_count;
  std::vector<idx_t> mptr;
  std::vector<size_t> mptr_offset;
  std::vector<size_t> mptr_count;
  std::vector<idx_t> mcol;
  std::vector<double> mval;
  std::vector<size_t> mcol_offset;
  std::vector<size_t> mcol_count;


  // long *idx_buf; // Important note in iterator() function comments
  // long *idx_sbuf;
  std::forward_list<std::pair<idx_t, level_t>> pair_sbuf;
  // long *mptr_buf;
  // long *mcol_buf;

  // double *vv_buf;
  // double *vv_sbuf;
  // double *mval_buf;
  void record_phase();

  void dump(const int rank);
  void load(const int rank);
};
