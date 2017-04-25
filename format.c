#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lib.h"

typedef struct {
  int row, col;
} ent_t;

int comp(const void *_x, const void *_y) {
  ent_t* x = (ent_t*) _x;
  ent_t* y = (ent_t*) _y;
  if (x->row < y->row)
    return -1;
  if (x->row > y->row)
    return 1;
  if (y->col < y->col)
    return -1;
  else
    return 1;
}

int main(int argc, char **argv) {

  if (argc != 3) {
    fprintf(stderr, "usage: %s infile oufile\n", argv[0]);
    exit(1);
  }

  FILE *fi = fopen(argv[1], "r");
  if (fi == NULL) {
    fprintf(stderr, "cannot open %s\n", argv[1]);
    exit(1);
  }

  FILE *fo = fopen(argv[2], "w");
  if (fo == NULL) {
    fprintf(stderr, "cannot open %s\n", argv[2]);
    exit(1);
  }

  char line[10240];
  fgets(line, 10240, fi);

  if (strstr(line, "%%MatrixMarket matrix coordinate") != line) {
    fprintf(stderr, "file %s unknown format\n", argv[1]);
    fprintf(stderr, "%s", line);
    exit(1);
  }

  int sym = (strstr(line, "symmetric") != NULL);

  do {
    fgets(line, 10240, fi);
  } while (line[0] == '%');

  int nrow, ncol, nnz;
  sscanf(line, "%d %d %d", &nrow, &ncol, &nnz);
  if (nrow != ncol) {
    fprintf(stderr, "file %s not-rectangular (%dx%d)\n",
	    argv[1], nrow, ncol);
    exit(1);
  }

  ent_t ent[nnz*2];
  int i, p=0;
  for (i=0; i< nnz; i++) {
    fgets(line, 10240, fi);
    sscanf(line, "%d %d", &ent[p].row, &ent[p].col);
    if (ent[p].row != ent[p].col) {
      p ++;
      if (sym) {
	ent[p].col = ent[p-1].row;
	ent[p].row = ent[p-1].col;
	p ++;
      }
    }
  }

  qsort(ent, p, sizeof(ent_t), comp);

  crs0_t *g = new_crs(nrow, p);
  g->ptr[0] = 0;
  for (i=0; i< p; i++) {
    if (i > 0 && ent[i].row != ent[i-1].row)
      g->ptr[ent[i].row - 1] = i;
    g->col[i] = ent[i].col - 1;
  }
  g->ptr[nrow] = p;

  write_crs(fo, g);

  return 0;
}
