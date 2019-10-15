/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-09-17
 *
 * @mainpage Partial Diamon Matrix Powers Kernel (PDMPK)
 *
 * Given a (sparse) matrix \f$A \in \mathbb{R}^{n \times n}\f$ and
 * vector \f$v \in \mathbb{R}\f$ MPK generates \f[ Av, A^2v, \ldots,
 * A^m v \f]
 *
 * This program generates the "computation and communication pattern"
 * for a given matrix \f$A\f$. The computation and communication
 * pattern mentioned above is a set of "buffers", or arrays, which can
 * be contiguously stored in memory, or on disk, and processed, that
 * is used to make the above vectors of the MPK.  The processing takes
 * place in multiple phases of local computation, separated by
 * communication.  The buffers describing the communication and
 * computation pattern is described here:
 *
 * - `mptr`, `mcol`, `mval`: the calculation itself is
 *   very similar to a regular SpMV operation with CSR format and uses
 *   these arrays
 *   @see mpi_bufs_t
 *   @see partial_cd
 *
 * - `ibuf`, `sbuf` and `ibuf`:
 *
 * - `sendcount`, `recvcount`, `sdispls` and `rdispls`: are the
 *   buffers corresponding to the `MPI_Alltoallv()` parameters.  These
 *   buffers have a fixed size of `nphase * npart * npart`.
 *
 * Since segments of most of these buffers are accessed per phase, if
 * a buffer `buf` (here `buf` is the name of any of the above buffers)
 * has a corresponding `buf_count` and/or `buf_offset` array, then for
 * phase `t` the relevant segment of `buf` is:
 *
 * - `buf_count` only case:
 *   - if `t == 0`, then `buf[0..buf_count[t]]`,
 *   - if `t > 0`, then `buf[buf_count[t - 1]..buf_count[t]]`
 *
 * - `buf_offset` only case: `buf[buf_offset[t]..buf_offset[t + 1]]`
 *
 * - `buf_offset` and `buf_count` case:
 *   `buf[buf_offset[t]..buf_offset[t] + buf_count[t]]`
 *
 * @todo(vatai): Finish documentation here.
 * @todo(vatai): Remove rank if not needed.
 * @todo(vatai): Implement MPI class if needed.
 *
 * Implementation plan:
 * @todo(vatai): Write documentation in `mcsr_t.h`
 * @todo(vatai): Find out what needs to be done in the init.
 */

#include <iostream>
#include <sstream>
#include <mpi.h>

#include "typedefs.h"
#include "partial_cd.h"

int main(int argc, char *argv[])
{
  idx_t npart;
  level_t nlevels;

  // Get npart
  std::stringstream npart_ss(argv[2]);
  npart_ss >> npart;
  std::stringstream nlevels_ss(argv[3]);
  nlevels_ss >> nlevels;

  partial_cd pcd(argv[1], npart, nlevels);

  return 0;
}
