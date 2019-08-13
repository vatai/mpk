#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mpi.h>
#include <math.h>

#include "mpi2_mpkexec.h"

static void log_tlist(buffers_t *bufs, int phase, FILE *log_file) {
  int n = bufs->n;
  int mcount = bufs->mcount[phase];
  long *idx_mbuf = bufs->idx_buf + bufs->mbuf_offsets[phase];
  for (int i = 0; i < mcount; i++) {
    int idx = idx_mbuf[i];
    int level = idx / n;
    int j = idx % n;
    fprintf(log_file, "%d (level:%3d, i: %3d)\n", idx, level, j);
  }
}

static void log_cd(double *vv_bufs, long *idx_bufs, int *count, int *displs, int n,
            FILE *log_file) {
  int npart;
  MPI_Comm_size(MPI_COMM_WORLD, &npart);

  int total = 0;
  for (int p = 0; p < npart; p++) {
    total += count[p];
  }
  assert(total == count[npart - 1] + displs[npart - 1]);

  fprintf(log_file, "[i<%3d] %8s, %8s\n", total, "vv_buf", "idx_buf");
  for (int i = 0; i < total; i++) {
    long idx = idx_bufs[i];
    int level = idx / n;
    int j = idx % n;
    fprintf(log_file, "[%5d] %8.3f, %8ld (level: %d, j: %d)\n", i, vv_bufs[i],
            idx, level, j);
  }

  fprintf(log_file, "[p<%3d] %8s, %8s\n", npart, "count", "disp");
  for (int p = 0; p < npart; p++) {
    fprintf(log_file, "[%5d] %8d, %8d\n", p, count[p], displs[p]);
  }
}

static void do_comm(int phase, buffers_t *bufs, FILE *log_file) {
  int npart = bufs->npart;

  // Log tlist.
  fprintf(log_file, "\n\n==== TLIST: part/rank %d : phase %d ====)\n", bufs->rank,
          phase);
  log_tlist(bufs, phase, log_file);

  int scount = bufs->scount[phase];
  long *idx_rbuf = bufs->idx_buf + bufs->rbuf_offsets[phase];
  long *idx_sbuf = bufs->idx_sbuf + bufs->sbuf_offsets[phase];
  double *vv_rbuf = bufs->vv_buf + bufs->rbuf_offsets[phase];
  double *vv_sbuf = bufs->vv_sbuf + bufs->sbuf_offsets[phase];
  // Copy data to send buffers.
  for (int i = 0; i < scount; i++) {
    int buf_idx = idx_sbuf[i];
    vv_sbuf[i] = bufs->vv_buf[buf_idx];
  }

  // Log send buffers.
  fprintf(log_file, "\n\n<<<< SEND: part/rank %d : phase %d >>>>\n\n",
          bufs->rank, phase);
  log_cd(vv_sbuf, idx_sbuf, bufs->sendcounts + npart * phase,
         bufs->sdispls + npart * phase, bufs->n, log_file);

  MPI_Alltoallv(vv_sbuf, bufs->sendcounts + npart * phase,
                bufs->sdispls + npart * phase, MPI_DOUBLE, vv_rbuf,
                bufs->recvcounts + npart * phase, bufs->rdispls + npart * phase,
                MPI_DOUBLE, MPI_COMM_WORLD);

  // Log receive buffers.
  fprintf(log_file, "\n\n>>>> RECV: part/rank %d : phase %d <<<<\n\n",
          bufs->rank, phase);
  log_cd(vv_rbuf, idx_rbuf, bufs->recvcounts + npart * phase,
         bufs->rdispls + npart * phase, bufs->n, log_file);
}

static void do_task(buffers_t *bufs, int phase) {
  // TODO(vatai): tl->th = omp_get_thread_num();
  // TODO(vatai): tl->t0 = omp_get_wtime();

  int mcount = bufs->mcount[phase];
  long *mptr = bufs->mptr_buf + bufs->mptr_offsets[phase];
  long *mcol = bufs->mcol_buf + bufs->mcol_offsets[phase];
  double *vv_mbuf = bufs->vv_buf + bufs->mbuf_offsets[phase];

  for (int mi = 0; mi < mcount; mi++) {
    double b = 1.0 / (mptr[mi + 1] - mptr[mi]);
    double tmp = 0.0;
    for (int mj = mptr[mi]; mj < mptr[mi + 1]; mj++){
      tmp += b * bufs->vv_buf[mcol[mj]];
    }
    vv_mbuf[mi] = tmp;
  }
  // TODO(vatai): tl->t1 = omp_get_wtime();
}

void mpi_exec_mpk(buffers_t *bufs) {
  assert(bufs != NULL);

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

void collect_results(buffers_t *bufs, double *vv) {
  for (int i = 0; i < bufs->buf_count; i++)
    vv[bufs->idx_buf[i]] = bufs->vv_buf[i];
  if (bufs->rank == 0) { // recv & copy
    for (int part = 1; part < bufs->npart; part++) {
      int buf_count;
      MPI_Recv(&buf_count, 1, MPI_INT, part, 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
      long *idx_buf = malloc(sizeof(*idx_buf) * buf_count);
      double *vv_buf = malloc(sizeof(*vv_buf) * buf_count);
      MPI_Recv(idx_buf, buf_count, MPI_LONG, part, 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
      MPI_Recv(vv_buf, buf_count, MPI_DOUBLE, part, 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
      for (int i = 0; i < buf_count; i++)
        vv[idx_buf[i]] = vv_buf[i];
      free(idx_buf);
      free(vv_buf);
    }
  } else { // send
    MPI_Send(&bufs->buf_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Send(bufs->idx_buf, bufs->buf_count, MPI_LONG, 0, 0, MPI_COMM_WORLD);
    MPI_Send(bufs->vv_buf, bufs->buf_count, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
  }
}

int check_results(buffers_t *bufs, double *vv) {
  if (bufs->rank == 0) {
    int size = bufs->n * bufs->nlevel;
    for (int i = 0; i < size; i++)
      if (vv[bufs->n + i] != 1.0)
        return -1;
    return 0;
  } else {
    return 0;
  }
}
