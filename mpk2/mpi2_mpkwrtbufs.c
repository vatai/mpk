#include <assert.h>
#include <stdlib.h>
#include <mpi.h>

#include "buffers.h"

static crs0_t *read_matrix(char *dir) {
  char fname[1024];
  FILE *f;
  sprintf(fname, "%s/g0", dir);
  f = fopen(fname, "r");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }
  return read_crs(f); // Closes f
}

static double *alloc_read_val(crs0_t *g0, char *dir) {
  double *val = malloc(sizeof(*val) * g0->ptr[g0->n]);
  assert(val != NULL);

  int cnt = 0; // Check for 3x '_'!
  for (int i = strlen(dir); 0 <= i; i--)
    if (dir[i] == '_') cnt++;
  assert(cnt == 3);

  cnt = 0; // Find first '_'!
  while (dir[cnt] != '_') cnt++;

  char fname[1024]; // Open file!
  sprintf(fname, "%s", dir);
  sprintf(fname + cnt, "%s", ".val");
  FILE *file = fopen(fname, "r");
  if (file == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }
  for (int i = 0; i < g0->ptr[g0->n]; i++)
    fscanf(file, "%lf ", val + i);

  fclose(file);
  return val;
}

static void alloc_fill_mval(buffers_t *bufs, double *val, crs0_t *g) {
  bufs->mval_buf = malloc(sizeof(*bufs->mval_buf) * bufs->mcol_count);
  assert(bufs->mval_buf != NULL);
  int *ptr = g->ptr;
  for (int phase = 0; phase <= bufs->nphase; phase++) {
    long *idx_buf = bufs->idx_buf + bufs->mbuf_offsets[phase];
    long *mptr = bufs->mptr_buf + bufs->mptr_offsets[phase];
    long *mcol = bufs->mcol_buf + bufs->mcol_offsets[phase];
    double *mval = bufs->mval_buf + bufs->mcol_offsets[phase];
    for (int mi = 0; mi < bufs->mcount[phase]; mi++) {
      long i = idx_buf[mi] % bufs->n;
      for (int mj = mptr[mi]; mj < mptr[mi + 1]; mj++) {
        long j = mj - mptr[mi] + ptr[i];
        mval[mj] = val[j];
      }
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s dirname\n", argv[0]);
    exit(1);
  }

  // Init MPI
  MPI_Init(&argc, &argv);
  comm_data_t *cd = new_comm_data(argv[1]);
  buffers_t *bufs = new_bufs(cd);
  fill_buffers(cd, bufs);

  crs0_t *graph = read_matrix(argv[1]);
  double *val = alloc_read_val(graph, argv[1]);
  alloc_fill_mval(bufs, val, graph);

  write_buffers(bufs, argv[1]);

  del_bufs(bufs);
  del_comm_data(cd);
  MPI_Finalize();
  return 0;
}
