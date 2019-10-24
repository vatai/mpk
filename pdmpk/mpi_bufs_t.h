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
  /// Fill displacement buffers (`sdispls` and `rdispls`) from the
  /// count buffers (`sendcount` and `recvcount`).
  void fill_dipls(int phase);
  /// Get `rbuf` size from `recvcount` and `rdispls`.
  int sbuf_size(int phase);
  /// Get `sbuf` size form `sendcount` and `sdispls`.
  int rbuf_size(int phase);
  /// Resize `sendcount`, `recvcount`, `sdispls` and `rdispls`.
  void resize(size_t size);

  /// - MPI (one for each phase):
  ///   - `sendbuf` and `recvbuf`
  ///   - `sendcount` and `recvcount`
  ///   - `sdispls` and `rdispls`
  std::vector<int> sendcounts;
  std::vector<int> recvcounts;
  std::vector<int> sdispls;
  std::vector<int> rdispls;
  /// @todo(vatai): Fill `sbuf_idcs`
  /// `mbuf` indices, which need to be copied to the send buffer.
  std::vector<idx_t> sbuf_idcs;

  /// @todo(vatai): It would be nice to "remove" this.
  const idx_t npart;

 private:
  mpi_bufs_t();
};
