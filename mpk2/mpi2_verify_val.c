#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lib.h"
#include "comm_data.h"

static int get_nleve(char *dir) {
  int i = 0, j = 0;
  for (j = strlen(dir); j >= 0; j--) {
    if (dir[j] == '_')
      i++;
    if (i == 3)
      break;
  }
  if (!(i == 3 && j >= 0 && dir[j] == '_'))
    dir_name_error(dir);

  int npart = -1, nlevel = -1, nphase = -1;
  i = sscanf(dir + j, "_%d_%d_%d", &npart, &nlevel, &nphase);
  if (i != 3)
    dir_name_error(dir);
  assert(npart > 0 && nlevel > 0 && nphase >= 0);

  return nlevel;
}

static void spmv(double *in, double *out, crs0_t *g, double *val) {
  for (int i = 0; i < g->n; i++) {
    double sum = 0.0;
    for (int j = g->ptr[i]; j < g->ptr[i + 1]; j++)
      sum += val[j] * in[g->col[j]];
    out[i] = sum;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s dirname\n", argv[0]);
    exit(1);
  }
  crs0_t *graph = read_matrix(argv[1]);
  int nlevel = get_nleve(argv[1]);
  double *val = alloc_read_val(graph, argv[1]);
  int count = graph->n * (nlevel + 1);
  double *vv = malloc(sizeof(*vv) * count);
  for (int i = 0; i < graph->n; i++)
    vv[i] = 1.0;
  for (int level = 0; level < nlevel; level++) {
    double *in = vv + level * graph->n;
    double *out = vv + (level + 1) * graph->n;
    spmv(in, out, graph, val);
  }
  char fname[1024];
  sprintf(fname, "%s/gold_result.bin", argv[1]);
  FILE *file = fopen(fname, "wb");
  fwrite(vv, sizeof(*vv), count, file);
  fclose(file);
  free(val);
  free(vv);
  del_crs(graph);
  return 0;
}
