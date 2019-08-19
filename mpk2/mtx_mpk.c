/*  Author: Emil VATAI <emil.vatai@gmail.com> */
/*  Date: 2019-08-19 */

/* This is a quick and dirty:
 *
 * - MPK which uses MTX format (custom mtx reading).
 *
 * For this reason it is not included in Makefile, it can be * build
 * without any dependencies. */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void spmv(double *ptr, int N, int nz, int *I, int *J, double *val) {
  double *out = ptr + N;
  for (int i = 0; i < N; i++) out[i] = 0.0;
  for (int i = 0; i < nz; i++) {
    int ii = I[i];
    int jj = J[i];
    int vv = val[i];
    out[ii] += vv * ptr[jj];
    if (jj != ii)
      out[jj] += vv * ptr[ii];
  }
}

int main(int argc, char *argv[]) {
  char fname[1024];
  sprintf(fname, "%s", argv[1]);
  char line[1024] = "";
  FILE *f = fopen(fname, "r");
  while(fgets(line, 1024, f)) {
    if(line[0] != '%')
      break;
  }
  int M, N, nz;
  printf("%s", line);
  sscanf(line, "%d %d %d", &M, &N, &nz);
  printf("M=%d, N=%d, nz=%d\n", M, N, nz);

  int *I = malloc(sizeof(int) * nz);
  int *J = malloc(sizeof(int) * nz);
  double *val = malloc(sizeof(double) * nz);

  int cnt = 0;
  while(fgets(line, 1024, f) != NULL) {
    if(line[0] != '%') {
      sscanf(line, "%d %d %lg", &I[cnt], &J[cnt], &val[cnt]);
      I[cnt]--;
      J[cnt]--;
      assert(0 <= I[cnt] && I[cnt] < N);
      assert(0 <= J[cnt] && J[cnt] < N);
      // printf("check %d %d %lg\n", I[cnt], J[cnt], val[cnt]);
      cnt++;
    }
  }
  assert(cnt == nz);
  fclose(f);

  // Print val[] array.
  printf("val[i]: ");
  for (int i = 0; i < nz; i++) printf("%lg ", val[i]);
  printf("\n");

  // Open file.
  sprintf(fname, "%s.gold.bin", argv[1]);
  FILE *g = fopen(fname, "wb");

  // Get nlevel.
  int nlevel;
  sscanf(argv[2], "%d", &nlevel);
  printf("nlevel=%d\n", nlevel);

  // Alloc ptr.
  double *ptr = malloc(sizeof(double) * N * (nlevel + 1));
  // Init ptr.
  for (int i = 0; i < N; i++)
    ptr[i] = 1.0;
  // Fill ptr.
  for (int i = 0; i < nlevel; i++) {
    spmv(ptr + i * N, N, nz, I, J, val);
  }
  fwrite(ptr, sizeof(double), N * (nlevel + 1), g);

  fclose(g);
  free(I);
  free(J);
  free(val);
  free(ptr);
  return 0;
}
