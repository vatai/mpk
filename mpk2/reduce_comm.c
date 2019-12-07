#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "reduce_comm.h"

static void make_sums(int *sums, comm_data_t *cd, char *comm_table) {
  int npart = cd->npart;
  int size = cd->n * cd->nlevel;
  for (int i = 0; i < npart; i++) {
    int max = 0;
    int maxto = 0;
    for (int j = 0; j < npart; j++) {
      int sum = 0;
      for (int k = 0; k < size; k++) {
        int idx = (i * npart + j) * size + k;
        sum += comm_table[idx];
      }
      sums[npart * i + j] = sum;
    }
  }
}

static void visit_perm(int npart, int *sums, int *tperm, int *perm, int *_max) {
  int sum = 0;
  for (int i = 0; i < npart; i++) {
    int j = tperm[i];
    sum += sums[j * npart + i];
  }
  if (sum > *_max) {
    *_max = sum;
    memcpy(perm, tperm, sizeof(*tperm) * npart);
  }
}

static void make_perm(int *perm, int *sums, int npart) {
  int tmp;
  int *tmpperm = (int *)malloc(sizeof(*tmpperm) * npart);
  // init perm a1 <= a2 <= .. <= an
  for (int i = 0; i < npart; i++) {
    tmpperm[i] = i;
    perm[i] = i;
  }
  int maxsum = 0;
  while (1) {
    visit_perm(npart, sums, tmpperm, perm, &maxsum);
    // Set j.
    int j = npart - 2;
    while (j >= 0 && tmpperm[j] >= tmpperm[j + 1]) j--;
    if (j < 0) break;
    // Set i. (ell in TAOCP)
    int i = npart - 1;
    while (i >= 0 && tmpperm[j] >= tmpperm[i]) i--;
    // Swap tmpperm[i] and tmpperm[j].
    tmp = tmpperm[j];
    tmpperm[j] = tmpperm[i];
    tmpperm[i] = tmp;
    assert(j >= 0);
    j++;
    i = npart - 1;
    while (j < i) {
      // Swap tmpperm[i] == tmpperm[j].
      tmp = tmpperm[j];
      tmpperm[j] = tmpperm[i];
      tmpperm[i] = tmp;
      j++;
      i--;
    }
  }
  free(tmpperm);
}

// TODO(vatai): remove this
static void assert_perm(int *perm, int n) {
  char *tmp = (char *)malloc(sizeof(*tmp) * n);
  for (int i = 0; i < n; i++) tmp[i] = 0;
  for (int i = 0; i < n; i++) {
    int pi = perm[i];
    assert(tmp[pi] == 0);
    tmp[pi] = 1;
  }
}

static void apply_perm_cd(int phase, int *perm, comm_data_t *cd) {
  int npart = cd->npart;
  int *part = cd->plist[phase]->part;
  for (int i = 0; i < cd->n; i++)
    part[i] = perm[part[i]];
}

static void apply_perm_sp(
    int phase,
    int *perm,
    comm_data_t *cd,
    int *store_part,
    char *cursp)
{
  int npart = cd->npart;
  int spsize = cd->n * (cd->nlevel + 1);
  for (int i = 0; i < spsize; i++)
    if (cursp[i])
      store_part[i] = perm[store_part[i]];
}

void debug_print(comm_data_t *cd, char *comm_table) {
  int *sums = (int *)malloc(sizeof(*sums) * cd->npart * cd->npart);
  make_sums(sums, cd, comm_table);
  if (cd->rank == 0) {
    printf("\n");
    for (int i = 0; i < cd->npart; i++) {
      for (int j = 0; j < cd->npart; j++) {
        printf("%4d ", sums[i * cd->npart + j]);
      }
      printf("\n");
    }
  }
  free(sums);
}

static void debug_(
    int phase,
    comm_data_t *cd,
    char *comm_table,
    int *store_part)
{
  char fname[1024];
  sprintf(fname, "debug-comm-phase%d.rank%d.log", phase, cd->rank);
  FILE *file = fopen(fname, "w");

  int npart;
  int *sums = (int *)malloc(sizeof(*sums) * npart * npart);
  make_sums(sums, cd, comm_table);
  if (cd->rank == 0) {
    fprintf(file, "\n");
    for (int i = 0; i < cd->npart; i++) {
      for (int j = 0; j < cd->npart; j++) {
        fprintf(file, "%4d ", sums[i * cd->npart + j]);
      }
      fprintf(file, "\n");
    }
  }
  free(sums);
  fclose(file);
}

// Save/write out the `sums` array.  This output will be used as the
// input, to aid LAPJV development
static void save_sums(int *sums, comm_data_t *cd, int phase) {
  const int N = cd->npart * cd->npart;
  char fname[1024];
  sprintf(fname, "csum_%s_%d.txt", cd->dir, phase);
  FILE *file = fopen(fname, "w");
  fprintf(file, "%d\n", cd->npart);
  for (int i = 0; i < N; i++) {
    fprintf(file, "%d\n", sums[i]);
  }
  fclose(file);
}

void reduce_comm(
    int phase,
    comm_data_t *cd,
    char *comm_table,
    int *store_part,
    char *cursp)
{
  int *perm = (int *)malloc(sizeof(*perm) * cd->npart);
  int *sums = (int *)malloc(sizeof(*sums) * cd->npart * cd->npart);
  make_sums(sums, cd, comm_table);
  save_sums(sums, cd, phase);
  debug_(phase, cd, comm_table, store_part);
  make_perm(perm, sums, cd->npart);

  assert_perm(perm, cd->npart); // TODO(vatai): remove

  apply_perm_cd(phase, perm, cd);
  apply_perm_sp(phase, perm, cd, store_part, cursp);

  free(sums);
  free(perm);
}

