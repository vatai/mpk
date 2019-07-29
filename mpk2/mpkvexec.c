// Code to execute MPK with matrix values (not just 1/(#adj(v))).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lib.h"

// do_task in mpktexec too, defined static
static void do_task(mpk_t *mg, double *vv, double *val, int phase, int part) {

  assert(mg != NULL && vv != NULL);

  int n = mg->n;

  task_t *tl = mg->tlist + phase * mg->npart + part;
  int *ptr = mg->g0->ptr;
  int *col = mg->g0->col;

  // tl->th = omp_get_thread_num();
  // tl->t0 = omp_get_wtime();

  int t;
  for (t=0; t< tl->n; t++) {
    int tidx = tl->idx[t];
    int level = tidx / n;
    int l1n = (level - 1) * n;
    int i = tidx - level * n; /*tidx % n*/

    double s = 0.0;
    int j;
    for (j = ptr[i]; j < ptr[i+1]; j++) {
      s += val[j] * vv[l1n + col[j]];
    }
    vv[level * n + i] = s;
  }

  // tl->t1 = omp_get_wtime();
}

void vexec_mpk(mpk_t *mg, double *vv, double *val) {
  assert(mg != NULL && vv != NULL);

  int n = mg->n;
  int npart = mg->npart;
  int nlevel = mg->nlevel;
  int nphase = mg->nphase;

  int phase;
  for (phase = 0; phase <= nphase; phase ++) {
    int part;
    for (part = 0; part < npart; part ++)
      do_task(mg, vv, val, phase, part);
  }
}
