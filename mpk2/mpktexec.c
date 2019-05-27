#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lib.h"
#include <omp.h>

static void do_task(mpk_t *mg, double *vv, int phase, int part) {
  assert(mg != NULL && vv != NULL);

  int n = mg->n;

  task_t *tl = mg->tlist + phase * mg->npart + part;
  int *ptr = mg->g0->ptr;
  int *col = mg->g0->col;
  int nlevel1 = mg->nlevel + 1;

  tl->th = omp_get_thread_num();
  tl->t0 = omp_get_wtime();

  int t;
  for (t=0; t< tl->n; t++) {
    int tidx = tl->idx[t];
    int level = tidx / n;
    int i = tidx - level * n; /*tidx % n*/
    double a = 1.0 / (ptr[i+1] - ptr[i]); /* simply... */

    double s = 0.0;
    int j;
    for (j = ptr[i]; j < ptr[i+1]; j++) {
      s += a * vv[col[j] * nlevel1 + level-1];
    }
    vv[i * nlevel1 + level] = s;
  }

  tl->t1 = omp_get_wtime();
}

void exec_mpkt(mpk_t *mg, double *vv, int nth) {
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
