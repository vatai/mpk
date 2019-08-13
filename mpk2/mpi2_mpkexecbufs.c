#include <assert.h>
#include <mpi.h>
#include <stdlib.h>

#include "buffers.h"
#include "mpi2_mpkexec.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s dirname\n", argv[0]);
    exit(1);
  }
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  buffers_t *bufs = read_buffers(argv[1], rank);

  alloc_val_bufs(bufs);

  double *vv = (double*) malloc(sizeof(double) * bufs->n * (bufs->nlevel+1));
  assert(vv != NULL);

  // NEW CODE
  for (int i = 0; i < bufs->buf_count; i++) {
    bufs->vv_buf[i] = -42;
  }
  for (int i = 0; i < bufs->rcount[0]; i++) {
    bufs->vv_buf[i] = 1.0;
  }

  mpi_exec_mpk(bufs);

  // Copy from buf to vv[]
  for (int i = 0; i < bufs->buf_count; i++) {
    vv[bufs->idx_buf[i]] = bufs->vv_buf[i];
  }

  collect_results(bufs, vv);

  int result = check_results(bufs, vv);
  del_bufs(bufs);

  free(vv);
  MPI_Finalize();
  return result;
}
