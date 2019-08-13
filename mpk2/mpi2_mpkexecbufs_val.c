#include <assert.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#include "buffers.h"
#include "mpi2_mpkexec.h"

// Similar function exists in comm_data.c
static crs0_t *read_matrix(char *dir) {
  char fname[1024];
  FILE *f;
  sprintf(fname, "%s/g0", dir);
  f = fopen(fname, "r");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }
  return read_crs(f); // Closes f
}

static double *alloc_read_val(crs0_t *g0, char *dir) {
  double *val = malloc(sizeof(*val) * g0->ptr[g0->n]);
  assert(val != NULL);

  int cnt = 0; // Check for 3x '_'!
  for (int i = strlen(dir); 0 <= i; i--)
    if (dir[i] == '_') cnt++;
  assert(cnt == 3);

  cnt = 0; // Find first '_'!
  while (dir[cnt] != '_') cnt++;

  char fname[1024]; // Open file!
  sprintf(fname, "%s", dir);
  sprintf(fname + cnt, "%s", ".val");
  FILE *file = fopen(fname, "r");
  if (file == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }
  for (int i = 0; i < g0->ptr[g0->n]; i++)
    fscanf(file, "%lf ", val + i);

  fclose(file);
  return val;
}

static void alloc_make_mval(buffers_t *bufs, double *val, crs0_t *g) {
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
    for (int mj = mptr[mi]; mj < mptr[mi + 1]; mj++){
      // TODO(vatai): Is thsi OK?
      tmp += mval[mj] * bufs->vv_buf[mcol[mj]];
    }
    vv_mbuf[mi] = tmp;
  }
  // TODO(vatai): tl->t1 = omp_get_wtime();
}

static void do_comm(int phase, buffers_t *bufs, FILE *log_file) {
  int npart = bufs->npart;

  int scount = bufs->scount[phase];
  long *idx_sbuf = bufs->idx_sbuf + bufs->sbuf_offsets[phase];
  double *vv_rbuf = bufs->vv_buf + bufs->rbuf_offsets[phase];
  double *vv_sbuf = bufs->vv_sbuf + bufs->sbuf_offsets[phase];
  // Copy data to send buffers.
  for (int i = 0; i < scount; i++) {
    int buf_idx = idx_sbuf[i];
    vv_sbuf[i] = bufs->vv_buf[buf_idx];
  }

  MPI_Alltoallv(vv_sbuf, bufs->sendcounts + npart * phase,
                bufs->sdispls + npart * phase, MPI_DOUBLE, vv_rbuf,
                bufs->recvcounts + npart * phase, bufs->rdispls + npart * phase,
                MPI_DOUBLE, MPI_COMM_WORLD);

}

static void mpk_exec_bufs_val(buffers_t *bufs) {
  char fname[1024];
  sprintf(fname, "mpi_comm_in_exec_rank-%d.log", bufs->rank);
  FILE *log_file = fopen(fname, "w");

  for (int phase = 0; phase <= bufs->nphase; phase++) {
    if (phase > 0) {
      do_comm(phase, bufs, log_file);
    }
    do_task(bufs, phase);
  }
  fclose(log_file);
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
}

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

  crs0_t *graph = read_matrix(argv[1]); // TODO(vatai): opt
  double *val = alloc_read_val(graph, argv[1]); // TODO(vatai): opt
  alloc_make_mval(bufs, val, graph); // TODO(vatai): opt

  for (int i = 0; i < bufs->rcount[0]; i++)
    bufs->vv_buf[i] = 1.0;

  mpk_exec_bufs_val(bufs);

  double *vv = (double*) malloc(sizeof(double) * bufs->n * (bufs->nlevel+1));
  assert(vv != NULL);
  collect_results(bufs, vv);
  if (bufs->rank == 0) write_results(bufs, vv, argv[1]);


  del_bufs(bufs);
  free(vv);
  MPI_Finalize();

  return 0;
}
