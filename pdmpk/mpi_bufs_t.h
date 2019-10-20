/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-10-19
 */

#pragma once

#include <vector>
#include <metis.h>

class mpi_bufs_t {
 public:
  mpi_bufs_t(const idx_t npart);
  void fill_dipls(int phase);
  int rbuf_size(int phase);

  /// - MPI (one for each phase):
  ///   - `sendbuf` and `recvbuf`
  ///   - `sendcount` and `recvcount`
  ///   - `sdispls` and `rdispls`
  std::vector<int> recvcounts;
  std::vector<int> sendcounts;
  std::vector<int> rdispls;
  std::vector<int> sdispls;
  std::vector<idx_t> sbuf_idx;

  const idx_t npart;

 private:
  mpi_bufs_t();
};