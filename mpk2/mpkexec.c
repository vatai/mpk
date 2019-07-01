#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lib.h"
#include <omp.h>
#include <mpi.h>

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
  printf(">>> mpi_exec_mpk BEGIN\n");
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
  for (phase = 0; phase <= nphase; phase++) {
    if (phase > 0) {
      for (int i = 0; i < sendcount[npart - 1] + sdispls[npart - 1]; ++i) {
        cd->vv_sbufs[phase][i] = vv[cd->idx_sbufs[phase][i]];
      }

      MPI_Alltoallv(cd->vv_sbufs[phase], sendcount, sdispls, MPI_FLOAT,
                    cd->vv_rbufs[phase], recvcount, rdispls, MPI_FLOAT,
                    MPI_COMM_WORLD);
      MPI_Alltoallv(cd->idx_sbufs[phase], sendcount, sdispls, MPI_INT,
                    cd->idx_rbufs[phase], recvcount, rdispls, MPI_INT,
                    MPI_COMM_WORLD);
      for (int i = 0; i < recvcount[npart - 1] + rdispls[npart - 1]; ++i) {
        vv[cd->idx_rbufs[phase][i]] = cd->vv_rbufs[phase][i];
      }
    }
    do_task(mg, vv, phase, rank);
    // TODO(vatai): Add the alltoall call to idx buffers and place the
    // vv values in the vv array (to index found in idx buffer).

    sendcount += npart;
    recvcount += npart;
    sdispls += npart;
    rdispls += npart;
  }
}
