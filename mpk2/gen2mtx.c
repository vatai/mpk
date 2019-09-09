// Author: Emil Vatai <emil.vatai@gmail.com>
// Data: 2019-07-29
//
// Counvert our gen2 (.g0, .val) format to MTX format.
//
// Usage: gen2mtx filename (with or without .g0).

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"

#define USAGE_MSG "%s filename (with or without .g0)."

void get_g0_fname(char *fname, char *ptr) {
  int len = strlen(ptr);
  if (strcmp(".g0", ptr + len - 3) == -1) {
    sprintf(fname, "%s.g0", ptr);
  } else {
    sprintf(fname, "%s", ptr);
  }
}

void get_val_fname(char *fname, char *ptr) {
  int len = strlen(ptr);
  sprintf(fname, "%s", ptr);
  sprintf(fname + len - 2, "%s", "val");
}

void get_mtx_fname(char *fname, char *ptr) {
  int len = strlen(ptr);
  sprintf(fname, "%s", ptr);
  sprintf(fname + len - 2, "%s", "mtx");
}

void save_mtx(crs0_t *g, double *val, FILE *f) {
  fprintf(f, "%%%%MatrixMarket matrix coordinate real general\n");
  fprintf(f, "%d %d %d\n", g->n, g->n, g->ptr[g->n]);
  for (int i = 0; i < g->n; i++) {
    for (int t = g->ptr[i]; t < g->ptr[i + 1]; t++) {
      int j = g->col[t];
      fprintf(f, "%d %d %lf\n", i + 1, j + 1, val[t]);
    }
  }
}

void read_val(FILE *f, double *val, int n) {
  for (int i = 0; i < n; i++) {
    fscanf(f, "%lf ", val + i);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, USAGE_MSG, argv[0]);
    return 1;
  }

  char g0_fname[1024];
  char val_fname[1024];
  char mtx_fname[1024];
  get_g0_fname(g0_fname, argv[1]);
  get_val_fname(val_fname, g0_fname);
  get_mtx_fname(mtx_fname, g0_fname);

  printf("Converting from %s to %s\n", g0_fname, mtx_fname);

  FILE *g0_file = fopen(g0_fname, "r");
  if (g0_file == NULL) {
    fprintf(stderr, "Can't open file %s", g0_fname);
    return 2;
  }
  FILE *val_file = fopen(val_fname, "r");
  if (val_file == NULL) {
    fprintf(stderr, "Can't open file %s", val_fname);
    return 2;
  }
  FILE *mtx_file = fopen(mtx_fname, "w");
  if (mtx_file == NULL) {
    fprintf(stderr, "Can't open file %s", mtx_fname);
    return 2;
  }
  crs0_t *g = read_crs(g0_file); // closes file
  double *val = (double *)malloc(sizeof(*val) * g->n);
  read_val(val_file, val, g->ptr[g->n]);
  save_mtx(g, val, mtx_file);
  fclose(mtx_file);
}
