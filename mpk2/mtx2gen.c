// Author: Emil VATAI (emil.vatai@gmail.com)
// Date: 2019-08-17

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffers.h"
#include "lib.h"
#include "mmio.h"

int mm_read_sparse(
    const char *fname,
    int *M_,
    int *N_,
    int *nz_,
    double **val_,
    int **I_,
    int **J_)
{
  FILE *f;
  MM_typecode matcode;
  int M, N, nz;
  int i;
  double *val;
  int *I, *J;

  if ((f = fopen(fname, "r")) == NULL)
    return -1;

  if (mm_read_banner(f, &matcode) != 0) {
    printf("mm_read_sparse: Could not process Matrix Market banner ");
    printf(" in file [%s]\n", fname);
    return -1;
  }

  if (!(mm_is_real(matcode) && mm_is_matrix(matcode) &&
        mm_is_sparse(matcode))) {
    fprintf(stderr, "Sorry, this application does not support ");
    fprintf(stderr, "Market Market type: [%s]\n", mm_typecode_to_str(matcode));
    return -1;
  }
  int sym = mm_is_symmetric(matcode);

  if (mm_read_mtx_crd_size(f, &M, &N, &nz) != 0) {
    fprintf(stderr,
            "read_unsymmetric_sparse(): could not parse matrix size.\n");
    return -1;
  }
  assert(N == M);

  int mem_nz = sym ? 2 * nz - M : nz;

  I = (int *)malloc(mem_nz * sizeof(int));
  J = (int *)malloc(mem_nz * sizeof(int));
  val = (double *)malloc(mem_nz * sizeof(double));

  /* NOTE: when reading in doubles, ANSI C requires the use of the "l"  */
  /*   specifier as in "%lg", "%lf", "%le", otherwise errors will occur */
  /*  (ANSI C X3.159-1989, Sec. 4.9.6.2, p. 136 lines 13-15)            */

  int ii = 0;
  for (i = 0; i < nz; i++) {
    fscanf(f, "%d %d %lg\n", &I[ii], &J[ii], &val[ii]);
    I[ii]--; /* adjust from 1-based to 0-based */
    J[ii]--;
    ii++;
    if (sym && I[ii - 1] != J[ii - 1]) {
      I[ii] = J[ii - 1];
      J[ii] = I[ii - 1];
      val[ii] = val[ii - 1];
      ii++;
    }
  }
  fclose(f);

  *M_ = M;
  *N_ = N;
  *nz_ = ii;
  *val_ = val;
  *I_ = I;
  *J_ = J;

  return 0;
}

void write_crs_as_gen(
    int M,
    int nz,
    int *I,
    int *J,
    double *val,
    char *fn)
{
  char fname[1024];

  sprintf(fname, "%s.g0", fn);
  FILE *f = fopen(fname, "w");
  int numdiag = 0;
  for (int i = 0; i < nz; i++)
    if (I[i] == J[i])
      numdiag++;
  fprintf(f, "%d %d\n", M, (nz - numdiag) / 2);

  sprintf(fname, "%s.loop.g0", fn);
  FILE *fl = fopen(fname, "w");
  fprintf(fl, "%d %d\n", M, (nz + numdiag) / 2);

  sprintf(fname, "%s.val", fn);
  FILE *fv = fopen(fname, "w");

  for (int i = 0; i < M; i++) {
    for (int j = 0; j < nz; j++) {
      if (I[j] == i) {
        if (I[j] != J[j]) {
          fprintf(f, "%d ", J[j] + 1);
        }
        fprintf(fl, "%d ", J[j] + 1);
        fprintf(fv, "%lf ", val[j]);
      }
    }
    fprintf(f, "\n");
    fprintf(fl, "\n");
  }
  fclose(f);
  fclose(fl);
  fclose(fv);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s matrix.mtx\n", argv[0]);
    exit(1);
  }

  MM_typecode matcode;
  int M, N, nz;
  int *I, *J;
  double *val;
  mm_read_sparse(argv[1], &M, &N, &nz, &val, &I, &J);
  /* for (int i = 0; i < nz; i++) { */
  /*   printf("%d %d %f\n", I[i], J[i], val[i]); */
  /* } */

  char fname[1024];
  sprintf(fname, "%s", argv[1]);
  fname[strlen(argv[1]) - 4] = 0;
  write_crs_as_gen(M, nz, I, J, val, fname);

  return 0;
}

