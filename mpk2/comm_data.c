#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#include "buffers.h"
#include "comm_data.h"

void dir_name_error(char *dir) {
  fprintf(stderr, "  unknwon directory naming %s\n", dir);
  fprintf(stderr, "  expecting ghead_npart_nlevel_nphase\n");
  exit(1);
}

void cond_assert_npart(comm_data_t *cd) {
  int flag;
  MPI_Initialized(&flag);
  if (flag) {
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    assert(world_size == cd->npart);
  }
}

static void read_dir(comm_data_t *cd) {
  int i = 0, j = 0;
  for (j = strlen(cd->dir); j >= 0; j--) {
    if (cd->dir[j] == '_')
      i++;
    if (i == 3)
      break;
  }
  if (!(i == 3 && j >= 0 && cd->dir[j] == '_'))
    dir_name_error(cd->dir);

  cd->npart = cd->nlevel = cd->nphase = -1;
  i = sscanf(cd->dir + j, "_%d_%d_%d", &cd->npart, &cd->nlevel, &cd->nphase);
  if (i != 3)
    dir_name_error(cd->dir);
  assert(cd->npart > 0 && cd->nlevel > 0 && cd->nphase >= 0);
}

static void alloc_mpk_data(comm_data_t *cd) {
  if (cd->nphase == 0) { // PA1
    cd->plist = (part_t **)malloc(sizeof(part_t *));
    cd->llist = NULL;
  } else {
    cd->plist = (part_t **)malloc(sizeof(part_t *) * cd->nphase);
    cd->llist = (level_t **)malloc(sizeof(level_t *) * cd->nphase);
    int i;
    for (i = 0; i < cd->nphase; i++) {
      cd->plist[i] = NULL;
      cd->llist[i] = NULL;
    }
  }
  cd->skirt = NULL;
}

static void fill_part(int phase, comm_data_t *cd) {
  char fname[1024];
  FILE *f;
  sprintf(fname, "%s/g%d.part.%d", cd->dir, phase, cd->npart);

  f = fopen(fname, "r");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }

  part_t *pg = new_part(cd->graph);
  read_part(f, pg);
  fclose(f);
  cd->plist[phase] = pg;
}

static void fill_level(int phase, comm_data_t *cd) {
  char fname[1024];
  FILE *f;
  sprintf(fname, "%s/l%d", cd->dir, phase);

  f = fopen(fname, "r");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }

  level_t *lg = new_level(cd->plist[phase]);
  read_level(f, lg); // fclose(f); ... in read_level
  cd->llist[phase] = lg;
}

static void fill_skirt(comm_data_t *cd) {
  char fname[1024];
  FILE *f;
  sprintf(fname, "%s/s%d", cd->dir, cd->nphase);

  f = fopen(fname, "r");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }

  skirt_t *sk = new_skirt(cd->plist[0]);
  read_skirt(f, sk, cd->nlevel); // fclose(f); in read_skirt
  cd->skirt = sk;
}

crs0_t *read_matrix(char *dir) {
  char fname[1024];
  FILE *f;
  sprintf(fname, "%s/loop.g0", dir);
  f = fopen(fname, "r");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }
  return read_crs(f); // Closes f
}

double *alloc_read_val(crs0_t *g0, char *dir) {
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

comm_data_t *new_comm_data(char *dir, int rank) {
  comm_data_t *cd = malloc(sizeof(*cd));
  cd->dir = dir;
  cd->rank = rank;
  read_dir(cd);
  cond_assert_npart(cd);
  cd->graph = read_matrix(dir);
  cd->n = cd->graph->n;
  alloc_mpk_data(cd);
  for (int phase = 0; phase < cd->nphase; phase++) {
    fill_part(phase, cd);
    fill_level(phase, cd);
  }
  if (cd->nphase == 0)
    fill_part(0, cd);
  fill_skirt(cd);

  return cd;
}

void del_comm_data(comm_data_t *cd) {
  del_crs(cd->graph);
  for (int phase = 0; phase < cd->nphase; phase++) {
    del_part(cd->plist[phase]);
    del_level(cd->llist[phase]);
  }
  if (cd->nphase == 0) {
    del_part(cd->plist[0]);
  } else {
    free(cd->llist);
  }
  free(cd->plist);
  del_skirt(cd->skirt);
  free(cd);
}
