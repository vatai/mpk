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
 * The `pdmpk_prep` program generates the "computation and
 * communication pattern" for a given matrix \f$A\f$. The computation
 * and communication pattern mentioned above is a set of "buffers", or
 * arrays, which can be contiguously stored in memory, or on disk, and
 * processed, that is used to make the above vectors of the MPK.  The
 * processing takes place in multiple phases of local computation,
 * separated by communication.  The buffers describing the
 * communication and computation pattern (for all partitions) are
 * collected in `partial_cd` more precisely, `buffers_t` in
 * `partial_cd::bufs`:
 *
 * - `partitions`, `weights`, `levels` and `partials`
 *   @see pdmpk_bufs_t
 *
 * - `mptr` (with `mptr_begin`), `mcol`, `mval`: the calculation
 *   itself is very similar to a regular SpMV operation with CSR
 *   format and uses these arrays @see mcsr_t
 *
 * - `sendcount`, `recvcount`, `sdispls` and `rdispls`: are the
 *   buffers corresponding to the `MPI_Alltoallv()` parameters.  These
 *   buffers have a fixed size of `nphase * npart * npart`.
 *   @see mpi_bufs_t
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
 * @see @ref pdmpk_prep_page
 * @see @ref pdmpk_exec_page
 *
 * @todo(vatai): Finish documentation here.
 *
 * @todo(vatai): Change comm_dict to store sets instead of vectors to
 * avoid duplicates in sbuf.
 *
 * @todo(vatai): Implement custom input vector loading.
 *
 * @todo(vatai): Collect results.
 *
 * @todo(vatai): Implement true mval test.
 *
 * @todo(vatai): Implement custom input vector test.
 *
 * @todo(vatai): Implement measurements.
 *
 */

/**
 * @page pdmpk_prep_page `pdmpk_prep` page
 * The body of `pdmpk_prep` page.
 *
 */

#include <iostream>

#include "typedefs.h"
#include "partial_cd.h"

int main(int argc, char *argv[])
{
  int npart = std::stoi(argv[2]);
  level_t nlevels = std::stoi(argv[3]);

  partial_cd pcd(argv[1], (idx_t)npart, nlevels);

  for (int i = 0; i < npart; i++) {
    pcd.bufs[i].dump(i);
    pcd.bufs[i].dump_txt(i);
  }

  return 0;
}
