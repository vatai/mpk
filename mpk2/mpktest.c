#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lib.h"
#include <omp.h>

#define DETAIL 1
#define ONEVEC 0
#define ONEENT 0
#define TRANS 0

void show_exinfo(mpk_t *mg) {
  assert(mg != NULL);

#if DETAIL == 0
  return;
#endif

  int npart = mg->npart;
  int nphase = mg->nphase;

  int i;
  double tsum = 0.0;
  int maxth = 0;
  for (i=0; i<= nphase; i++) {
    int j;
    for (j=0; j< npart; j++) {
      task_t *tl = mg->tlist + i * npart + j;
#if DETAIL
      printf("%d %d %d %d %e %e\n", i, j, tl->th, 
	     tl->n, tl->t1 - tl->t0, (tl->t1 - tl->t0) / tl->n);
#endif
      tsum += tl->t1 - tl->t0;
      if (maxth < tl->th)
	maxth = tl->th;
    }
  }
  printf("total work %e, avrg %e\n", tsum, tsum/(maxth+1.0));
}

void check_error(double *vv, int n, int nlevel) {
  int i;
  double e = 0.0;
  for (i=0; i< n; i++) {
    double v = vv[n * nlevel + i];
    e += (v - 1.0) * (v - 1.0);
  }
  printf("error %e\n", sqrt(e/n));
}

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "usage: %s dirname\n", argv[0]);
    exit(1);
  }

  mpk_t *mg = read_mpk(argv[1]);

  int n = mg->n;
  int nlevel = mg->nlevel;

  double *vv = (double*) malloc(sizeof(double) * n * (nlevel+1));
  assert(vv != NULL);

  prep_mpk(mg, vv);

  int i;
  for (i=0; i< n; i++)
    vv[i] = 1.0;
  for (i=0; i< n * nlevel; i++)
    vv[n + i] = -1.0;		/* dummy */

  double min;
  for (i=0; i< 5; i++) {
    double t0 = omp_get_wtime();
    spmv_exec_seq(mg->g0, vv, nlevel);
    double t1 = omp_get_wtime();
    if (i == 0 || min > t1 - t0)
      min = t1 - t0;
  }
  printf("seq spmv time= %e\n", min);
  check_error(vv, n, nlevel);

  int nth;
  for (nth = 1; nth <= mg->npart; nth ++) {
    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      spmv_exec_par(mg->g0, vv, nlevel, nth);
      double t1 = omp_get_wtime();

      //      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("spmv nth= %d time= %e\n", nth, min);
    check_error(vv, n, nlevel);
  }

  /********************************************************************/
#if ONEVEC
  printf("force to refer only one element\n");
  for (i=0; i< mg->g0->ptr[n+1]; i++)
    mg->g0->col[i] = 0;
#endif

#if ONEENT
  printf("force to compute only one element\n");
  for (i=0; i< mg->npart * (mg->nphase+1); i++) {
    int j;
    for (j=0; j< mg->tlist[i].n; j++)
      mg->tlist[i].idx[j] = n;
  }
#endif
  /********************************************************************/


  for (nth = 1; nth <= mg->npart; nth ++) {
#if 1
    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      exec_mpk_xd(mg, vv, nth);
      double t1 = omp_get_wtime();

      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("mpk nth= %d time= %e\n", nth, min);
    check_error(vv, n, nlevel);
#else
    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      exec_mpk_xs(mg, vv, nth);
      double t1 = omp_get_wtime();

      //      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("xs nth= %d time= %e\n", nth, min);

    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      exec_mpk_xd(mg, vv, nth);
      double t1 = omp_get_wtime();

      //      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("xd nth= %d time= %e\n", nth, min);

    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      exec_mpk_is(mg, vv, nth);
      double t1 = omp_get_wtime();

      //      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("is nth= %d time= %e\n", nth, min);

    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      exec_mpk_id(mg, vv, nth);
      double t1 = omp_get_wtime();

      //      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("id nth= %d time= %e\n", nth, min);
#endif
  }

#if TRANS
  for (i=0; i< n * (nlevel+1); i++)
    vv[i] = -1.0;		/* dummy */
  for (i=0; i< n; i++)
    vv[i * (nlevel+1)] = 1.0;

  for (nth = 1; nth <= mg->npart; nth ++) {
    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      exec_mpkt(mg, vv, nth);
      double t1 = omp_get_wtime();

      //      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("mpkt nth= %d time= %e\n", nth, min);
  }
#endif

  return 0;
}
