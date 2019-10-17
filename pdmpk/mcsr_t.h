/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-10-15
 *
 * Modified CSR (for one partition/rank).  A vector `npart` number of
 * `mcsr_t` is stored, and written to the disk in the end.  In phase
 * `p`, `mptr_count[p]` elements of `mptr[]` are processed starting
 * from `mptr_offset[p]`.
 */

/// So my current dilemma: how to combine or separate all the buffers.
///
/// - BUF (one over all phases):
///   - `mbuf` (`mbuf_begin`, **uses** `mptr_begin` from csr)
///   - `sbuf_idcs` (**uses** `sendcount` and `sdispls`)
///   - `ibuf` (`ibuf_begin`)
/// - CSR (one over all phases):
///   - `mptr` (`mptr_begin`)
///   - `mcol`
///   - `mval` (or `mval_idx`)
/// - MPI (one for each phase):
///   - `sendbuf` and `recvbuf`
///   - `sendcount` and `recvcount`
///   - `sdispls` and `rdispls`
///   Note: count = count[npart - 1] + displ[npart - 1]

#pragma once
#include <utility>
#include <vector>

#include <metis.h>

class mcsr_t {
public:
  std::vector<idx_t> mptr;
  std::vector<size_t> mptr_begin;

  std::vector<idx_t> mcol;
  std::vector<idx_t> mval_idx;
};

class bufs_t {
  std::vector<idx_t> mbuf;
  std::vector<idx_t> mbuf_begin;

  std::vector<idx_t> sbuf_idx;

  std::vector<std::pair<idx_t, idx_t>> ibuf;
  std::vector<idx_t> ibuf_begin;
};
