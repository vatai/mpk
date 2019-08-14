#include <assert.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#include "buffers.h"

static void alloc_fill_mval(buffers_t *bufs, double *val, crs0_t *g) {
  bufs->mval_buf = malloc(sizeof(*bufs->mval_buf) * bufs->mcol_count);
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

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s dirname\n", argv[0]);
    exit(1);
  }

  // Init MPI
  MPI_Init(&argc, &argv);
  comm_data_t *cd = new_comm_data(argv[1]);
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
