/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-09-17
 *
 * @todo(vatai): **Fill sendbuf**.  In reverse order: 1. step. figure
 * out the size of sendbuf (done); 2. step: fill sbuf_idx from a
 * comm_dict (which contains absolute "source" indices); 3. step: fill
 * the comm_dict (with the "source" indices).
 *//**
 * @brief The buffers collected on a single partition.
 *
 * @details `buffers_t` contains the MPI buffers, the modified CSP
 * buffers and `mbuf` and `ibuF`.
 */
#pragma once

#include <forward_list>
#include <vector>
#include <utility>
#include <map>

#include <metis.h>

#include "typedefs.h"
#include "mpi_bufs_t.h"
#include "mcsr_t.h"

// static void mpk_exec_bufs_val(buffers_t *bufs) {
//   do_task(bufs, 0);
//   for (int phase = 1; phase <= bufs->nphase; phase++) {
//     do_comm(phase, bufs);
//     do_task(bufs, phase);
//   }
// }

class buffers_t {
 public:
  buffers_t(const idx_t npart);
  void fill_sbuf_idcs(
      const idx_t src,
      const int phase,
      const std::map<src_tgt_t, std::vector<idx_t>> &comm_dict);

  void dump(const int rank);
  void load(const int rank);

  mpi_bufs_t mpi_bufs;
  mcsr_t mcsr;

  /// - BUF (one over all phases):
  ///   - `mbuf` (`mbuf_begin`, **uses** `mptr_begin` from csr)
  ///   - `sbuf_idcs` (**uses** `sendcount` and `sdispls`)
  ///   - `ibuf` (`ibuf_begin`)

  /// The index in `mbuf` where a vertex will be stored.
  idx_t mbuf_idx;

 private:
  buffers_t();
};
