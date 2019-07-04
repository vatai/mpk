#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lib.h"
#include <omp.h>
#include <mpi.h>

void log_tlist(mpk_t *mg, int phase, FILE *log_file) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  task_t *tl = mg->tlist + phase * mg->npart + rank;
  int n = mg->n;
  for (int i = 0; i < tl->n; i++) {
    int idx = tl->idx[i];
    int level = idx / n;
    int j = idx % n;
    fprintf(log_file, "%d (level:%3d, i: %3d)\n", idx, level, j);
  }
}

void log_cd(double *vv_bufs, int *idx_bufs, int *count, int *displs,
            int n, FILE *log_file) {
  int npart;
  MPI_Comm_size(MPI_COMM_WORLD, &npart);

  int total = 0;
  for (int p = 0; p < npart; p++) {
    total += count[p];
  }
  assert(total == count[npart - 1] + displs[npart - 1]);

  fprintf(log_file, "[i<%3d] %8s, %8s\n", total, "vv_buf", "idx_buf");
  for (int i = 0; i < total; i++) {
    int idx = idx_bufs[i];
    int level = idx / n;
    int j = idx % n;
    fprintf(log_file, "[%5d] %8.3f, %8d (level: %d, j: %d)\n", i, vv_bufs[i], idx,
            level, j);
  }

  fprintf(log_file, "[p<%3d] %8s, %8s\n", npart, "count", "disp");
  for (int p = 0; p < npart; p++) {
    fprintf(log_file, "[%5d] %8d, %8d\n", p, count[p], displs[p]);
  }
}

static void do_task(mpk_t *mg, double *vv, int phase, int part) { // do_task in mpktexec too, defined static
  assert(mg != NULL && vv != NULL);

  int n = mg->n;

  task_t *tl = mg->tlist + phase * mg->npart + part;
  int *ptr = mg->g0->ptr;
  int *col = mg->g0->col;

  tl->th = omp_get_thread_num();
  tl->t0 = omp_get_wtime();

  int t;
  for (t=0; t< tl->n; t++) {
    int tidx = tl->idx[t];
    int level = tidx / n;
    int l1n = (level - 1) * n;
    int i = tidx - level * n; /*tidx % n*/
    double a = 1.0 / (ptr[i+1] - ptr[i]); /* simply... */

    double s = 0.0;
    int j;
    for (j = ptr[i]; j < ptr[i+1]; j++) {
      s += a * vv[l1n + col[j]];
    }
    vv[level * n + i] = s;
  }

  tl->t1 = omp_get_wtime();
}

void exec_mpk(mpk_t *mg, double *vv, int nth) {
  assert(mg != NULL && vv != NULL);

  int n = mg->n;
  int npart = mg->npart;
  int nlevel = mg->nlevel;
  int nphase = mg->nphase;

  int phase;
#pragma omp parallel num_threads(nth) private(phase)
  for (phase = 0; phase <= nphase; phase ++) {
    int part;
    //#pragma omp parallel for num_threads(nth) schedule(static,1)
    //#pragma omp for schedule(dynamic, 1)
#pragma omp for schedule(static, 1)
    for (part = 0; part < npart; part ++)
      do_task(mg, vv, phase, part);
  }
}

void exec_mpk_xs(mpk_t *mg, double *vv, int nth) {
  assert(mg != NULL && vv != NULL);

  int n = mg->n;
  int npart = mg->npart;
  int nlevel = mg->nlevel;
  int nphase = mg->nphase;

  int phase;
#pragma omp parallel num_threads(nth) private(phase)
  for (phase = 0; phase <= nphase; phase ++) {
    int part;
#pragma omp for schedule(static, 1)
    for (part = 0; part < npart; part ++)
      do_task(mg, vv, phase, part);
  }
}

void exec_mpk_xd(mpk_t *mg, double *vv, int nth) {
  assert(mg != NULL && vv != NULL);

  int n = mg->n;
  int npart = mg->npart;
  int nlevel = mg->nlevel;
  int nphase = mg->nphase;

  int phase;
#pragma omp parallel num_threads(nth) private(phase)
  for (phase = 0; phase <= nphase; phase ++) {
    int part;
#pragma omp for schedule(dynamic, 1)
    for (part = 0; part < npart; part ++)
      do_task(mg, vv, phase, part);
  }
}

void exec_mpk_is(mpk_t *mg, double *vv, int nth) {
  assert(mg != NULL && vv != NULL);

  int n = mg->n;
  int npart = mg->npart;
  int nlevel = mg->nlevel;
  int nphase = mg->nphase;

  int phase;
  for (phase = 0; phase <= nphase; phase ++) {
    int part;
#pragma omp parallel for num_threads(nth) schedule(static,1)
    for (part = 0; part < npart; part ++)
      do_task(mg, vv, phase, part);
  }
}

void exec_mpk_id(mpk_t *mg, double *vv, int nth) {
  assert(mg != NULL && vv != NULL);

  int n = mg->n;
  int npart = mg->npart;
  int nlevel = mg->nlevel;
  int nphase = mg->nphase;

  int phase;
  for (phase = 0; phase <= nphase; phase ++) {
    int part;
#pragma omp parallel for num_threads(nth) schedule(dynamic,1)
    for (part = 0; part < npart; part ++)
      do_task(mg, vv, phase, part);
  }
}

void mpi_exec_mpk(mpk_t *mg, double *vv, comm_data_t *cd) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  assert(mg != NULL && vv != NULL && cd != NULL);

  int n = mg->n;
  int npart = mg->npart;
  int nlevel = mg->nlevel;
  int nphase = mg->nphase;

  int *sendcount = cd->sendcounts;
  int *recvcount = cd->recvcounts;
  int *sdispls = cd->sdispls;
  int *rdispls = cd->rdispls;

  int phase;

  // TODO(vatai): remove debug messages!
  char fname[1024];
  sprintf(fname, "mpi_comm_in_exec_rank-%d.log", rank);
  FILE *log_file = fopen(fname, "w");
  for (phase = 0; phase <= nphase; phase++) {
    if (phase > 0) {
      fprintf(log_file, "\n\n==== TLIST: part/rank %d : phase %d ====)\n", rank, phase);
      log_tlist(mg, phase, log_file);
      for (int i = 0; i < sendcount[npart - 1] + sdispls[npart - 1]; ++i) {
        cd->vv_sbufs[phase][i] = vv[cd->idx_sbufs[phase][i]];
      }

      fprintf(log_file, "\n\n<<<< SEND: part/rank %d : phase %d >>>>\n\n", rank, phase);
      log_cd(cd->vv_sbufs[phase], cd->idx_sbufs[phase], sendcount, sdispls,
             cd->n, log_file);
      MPI_Alltoallv(cd->vv_sbufs[phase], sendcount, sdispls, MPI_DOUBLE,
                    cd->vv_rbufs[phase], recvcount, rdispls, MPI_DOUBLE,
                    MPI_COMM_WORLD);
      MPI_Alltoallv(cd->idx_sbufs[phase], sendcount, sdispls, MPI_INT,
                    cd->idx_rbufs[phase], recvcount, rdispls, MPI_INT,
                    MPI_COMM_WORLD);
      fprintf(log_file, "\n\n>>>> RECV: part/rank %d : phase %d <<<<\n\n", rank, phase);
      log_cd(cd->vv_rbufs[phase], cd->idx_rbufs[phase], recvcount, rdispls,
             cd->n, log_file);

      for (int i = 0; i < recvcount[npart - 1] + rdispls[npart - 1]; ++i) {
        vv[cd->idx_rbufs[phase][i]] = cd->vv_rbufs[phase][i];
      }
    }

    do_task(mg, vv, phase, rank);

    sendcount += npart;
    recvcount += npart;
    sdispls += npart;
    rdispls += npart;
  }
  fclose(log_file);
}
