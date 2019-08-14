#include <assert.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#include "buffers.h"
#include "mpi2_mpkexec.h"

static void do_task(buffers_t *bufs, int phase) {
  // TODO(vatai): tl->th = omp_get_thread_num();
  // TODO(vatai): tl->t0 = omp_get_wtime();

  int mcount = bufs->mcount[phase];
  long *mptr = bufs->mptr_buf + bufs->mptr_offsets[phase];
  long *mcol = bufs->mcol_buf + bufs->mcol_offsets[phase];
  double *mval = bufs->mval_buf + bufs->mcol_offsets[phase];
  double *vv_mbuf = bufs->vv_buf + bufs->mbuf_offsets[phase];

  for (int mi = 0; mi < mcount; mi++) {
    double tmp = 0.0;
    for (int mj = mptr[mi]; mj < mptr[mi + 1]; mj++)
      tmp += mval[mj] * bufs->vv_buf[mcol[mj]];
    vv_mbuf[mi] = tmp;
  }
  // TODO(vatai): tl->t1 = omp_get_wtime();
}

static void do_comm(int phase, buffers_t *bufs) {
  int offset = bufs->npart * phase;
  int scount = bufs->scount[phase];
  long *idx_sbuf = bufs->idx_sbuf + bufs->sbuf_offsets[phase];
  double *vv_rbuf = bufs->vv_buf + bufs->rbuf_offsets[phase];
  double *vv_sbuf = bufs->vv_sbuf + bufs->sbuf_offsets[phase];

  // Copy data to send buffers.
  for (int i = 0; i < scount; i++) {
    int buf_idx = idx_sbuf[i];
    vv_sbuf[i] = bufs->vv_buf[buf_idx];
  }

  MPI_Alltoallv(vv_sbuf, bufs->sendcounts + offset, bufs->sdispls + offset,
                MPI_DOUBLE, vv_rbuf, bufs->recvcounts + offset,
                bufs->rdispls + offset, MPI_DOUBLE, MPI_COMM_WORLD);
}

static void mpk_exec_bufs_val(buffers_t *bufs) {
  do_task(bufs, 0);
  for (int phase = 1; phase <= bufs->nphase; phase++) {
    do_comm(phase, bufs);
    do_task(bufs, phase);
  }
}

static void write_results(buffers_t *bufs, double *vv, char *dir) {
  char fname[1024];
  sprintf(fname, "%s/results.txt", dir);
  FILE *file = fopen(fname, "w");
  for (int lvl = 0; lvl < bufs->nlevel; lvl++) {
    for (int i = 0; i < bufs->n; i++) {
      fprintf(file, "%lf ", vv[bufs->n * lvl + i]);
    }
    fprintf(file, "\n");
  }
  fclose(file);

  int count = bufs->n * (bufs->nlevel + 1);
  sprintf(fname, "%s/results.bin", dir);
  file = fopen(fname, "wb");
  fwrite(vv, sizeof(*vv), count, file);
  fclose(file);
}

int main(int argc, char *argv[]) {
  check_args(argc, argv[0]);
  MPI_Init(&argc, &argv);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  buffers_t *bufs = read_buffers(argv[1], rank);
  alloc_val_bufs(bufs);

  // Fill part of input vector in current partition.
  for (int i = 0; i < bufs->rcount[0]; i++)
    bufs->vv_buf[i] = 1.0;

  const int nreps = 5;
  double mintime;
  for (int t = 0; t < nreps; t++) {
    double start = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    mpk_exec_bufs_val(bufs); // Do the calculation.
    MPI_Barrier(MPI_COMM_WORLD);
    double newtime = MPI_Wtime() - start;
    if (t == 0)
      mintime = newtime;
    else
      mintime = newtime < mintime ? newtime: mintime;
  }
  double wtick = MPI_Wtime();
  printf("Min time for %d runs is %lf (Wtick: %lf)\n", nreps, mintime, wtick);

  // Collect the results on rank == 0.
  double *vv = (double*) malloc(sizeof(double) * bufs->n * (bufs->nlevel+1));
  assert(vv != NULL);
  collect_results(bufs, vv);
  if (bufs->rank == 0) write_results(bufs, vv, argv[1]);

  del_bufs(bufs);
  free(vv);
  MPI_Finalize();
  return 0;
}
