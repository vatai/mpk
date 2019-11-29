/// @file
/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-10-31

#include <assert.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#include "buffers.h"

static void alloc_fill_mval(buffers_t *bufs, double *val, crs0_t *g) {
  bufs->mval_buf = (double *)malloc(sizeof(*bufs->mval_buf) * bufs->mcol_count);
  assert(bufs->mval_buf != NULL);
  int *ptr = g->ptr;
  for (int phase = 0; phase <= bufs->nphase; phase++) {
    long *idx_buf = bufs->idx_buf + bufs->mbuf_offsets[phase];
    long *mptr = bufs->mptr_buf + bufs->mptr_offsets[phase];
    long *mcol = bufs->mcol_buf + bufs->mcol_offsets[phase];
    double *mval = bufs->mval_buf + bufs->mcol_offsets[phase];
    for (int mi = 0; mi < bufs->mcount[phase]; mi++) {
      long i = idx_buf[mi] % bufs->n;
      for (int mj = mptr[mi]; mj < mptr[mi + 1]; mj++) {
        long j = mj - mptr[mi] + ptr[i];
        mval[mj] = val[j];
      }
    }
  }
}

/// @brief This is the second stage of pre\-processing, after the
/// computation of the levels using the `driver` program.
///
/// @details The program is called with one parameter: DIRNAME, which
/// contains the output of the driver program.  This information is
/// stored as a `comm_data_t` `struct`.  This information is used to
/// create buffers (of type `buffer_t`) which hold the computation and
/// communication patterns.  The `mval` array of the buffer is filled
/// separately (see the function above).

int main(int argc, char* argv[]) {
  check_args(argc, argv[0]);
  MPI_Init(&argc, &argv);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  comm_data_t *cd = new_comm_data(argv[1], rank);
  buffers_t *bufs = new_bufs(cd);
  fill_buffers(cd, bufs);

  double *val = alloc_read_val(cd->graph, argv[1]);
  alloc_fill_mval(bufs, val, cd->graph);

  write_buffers(bufs, argv[1]);

  del_bufs(bufs);
  del_comm_data(cd);
  MPI_Finalize();
  return 0;
}
