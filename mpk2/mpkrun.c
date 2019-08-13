#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "lib.h"


// new stuff
void vexec_mpk(mpk_t *mg, double *vv, double *val);

double *alloc_read_val(mpk_t *mg, char *ptr) {
  double *val = malloc(sizeof(*val) * mg->g0->ptr[mg->n]);
  assert(val != NULL);

  int cnt = 0;
  for (int i = strlen(ptr); 0 <= i; i--)
    if (ptr[i] == '_') cnt++;
  assert(cnt == 3);

  cnt = 0;
  while (ptr[cnt] != '_') cnt++;

  char fname[1024];
  sprintf(fname, "%s", ptr);
  sprintf(fname + cnt, "%s", ".val");
  FILE *file = fopen(fname, "r");
  printf("Val file: %s\n", fname);
  for (int i = 0; i < mg->g0->ptr[mg->n]; i++)
    fscanf(file, "%lf ", val + i);
  fclose(file);
  return val;
}

void write_results(mpk_t *mg, double *vv, char *dir) {
  char fname[1024];

  sprintf(fname, "%s/results.txt", dir);
  FILE *file = fopen(fname, "w");
  for (int lvl = 0; lvl < mg->nlevel; lvl++) {
    for (int i = 0; i < mg->n; i++) {
      fprintf(file, "%lf ", vv[mg->n * lvl + i]);
    }
    fprintf(file, "\n");
  }
  fclose(file);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s dirname\n", argv[0]);
    exit(1);
  }

  mpk_t *mg = read_mpk(argv[1]);
  double *vv = (double*) malloc(sizeof(double) * mg->n * (mg->nlevel+1));
  double *val = alloc_read_val(mg, argv[1]);
  assert(vv != NULL);

  prep_mpk(mg, vv);

  for (int i = 0; i < mg->n; i++) {
    vv[i] = 1.0;
    // vv[i] += (i % 2 == 0 ? -0.001 * i: +0.001 * i);
    // printf("vv: %lf\n", vv[i]);
  }
  for (int i = 0; i < mg->n * mg->nlevel; i++)
    vv[mg->n + i] = -1.0;		/* dummy */

  vexec_mpk(mg, vv, val);
  write_results(mg, vv, argv[1]);

  return 0;
}
