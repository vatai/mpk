/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-10-19
 */

#pragma once

class mpi_bufs_t {
public:
  /// - MPI (one for each phase):
  ///   - `sendbuf` and `recvbuf`
  ///   - `sendcount` and `recvcount`
  ///   - `sdispls` and `rdispls`
  std::vector<int> recvcounts; // MPI
  std::vector<int> sendcounts; // MPI
  std::vector<int> rdispls;  // MPI
  std::vector<int> sdispls;  // MPI
  std::vector<idx_t> sbuf_idx;   // MPI
};
