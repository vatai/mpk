#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lib.h"
#include <omp.h>

void spmv_exec_seq(crs0_t *g0, double *vv, int nlevel) {
  assert(g0 != NULL && vv != NULL);

  int n = g0->n;
  int *ptr = g0->ptr;
  int *col = g0->col;

  int l;
  for (l = 1; l <= nlevel; l++) {
    int l1n = (l - 1) * n;
    int i;
    for (i=0; i< n; i++) {
      double a = 1.0 / (ptr[i+1] - ptr[i]); // 1/(no. of elemets in row i+1)
      double s = 0.0;
      int j;
      for (j = ptr[i]; j < ptr[i+1]; j++) // gives a*sigma(vv[l1n+col[ptr[i]]].......vv[l1n+col[ptr[i+1]]])
	s += a * vv[l1n + col[j]];
      vv[l * n + i] = s; // saving the results in the last n indices
    }
  }
}

void spmv_exec_par(crs0_t *g0, double *vv, int nlevel, int nth) {
  assert(g0 != NULL && vv != NULL);

  int n = g0->n;
  int *ptr = g0->ptr;
  int *col = g0->col;

  int l;
#pragma omp parallel num_threads(nth) private(l)
  for (l = 1; l <= nlevel; l++) {
    int l1n = (l - 1) * n;
    int i;
#pragma omp for schedule(static)
    for (i=0; i< n; i++) {
      double a = 1.0 / (ptr[i+1] - ptr[i]);
      double s = 0.0;
      int j;
      for (j = ptr[i]; j < ptr[i+1]; j++)
	s += a * vv[l1n + col[j]];
      vv[l * n + i] = s;
    }
  }
}
