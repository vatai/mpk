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
  void phase_finalize(const int phase);

  void exec();
  void do_comp(int phase, std::vector<double> &mbuf);
  void do_comm(int phase, std::vector<double> &mbuf, std::ofstream &os);
  void dump(const int rank);
  void load(const int rank);
  void dump_txt(const int rank);
  void load_txt(const int rank);

  mpi_bufs_t mpi_bufs;
  mcsr_t mcsr;

  /// - BUF (one over all phases):
  ///   - `mbuf` (`mbuf_begin`, **uses** `mptr_begin` from csr)
  ///   - `sbuf_idcs` (**uses** `sendcount` and `sdispls`)
  ///   - `ibuf` (`ibuf_begin`)

  /// The index in `mbuf` where a vertex will be stored.
  idx_t mbuf_idx;
  std::vector<idx_t> mbuf_begin;

 private:
  template <typename T>
  friend std::ostream & operator<<(std::ostream &os, const std::vector<T>& vec);
  buffers_t();
};

template <typename T>
std::ostream & operator<<(std::ostream &os, const std::vector<T>& vec)
{
  std::cout << "flags: " << os.flags() << std::endl;
  return os;
}
