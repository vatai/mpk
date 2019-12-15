#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lapjv.h"
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

static void apply_perm_cd(int phase, int *perm, comm_data_t *cd) {
  int npart = cd->npart;
  int *part = cd->plist[phase]->part;
  for (int i = 0; i < cd->n; i++)
    part[i] = perm[part[i]];
}

static void apply_perm_sp(int phase,       //
                          int *perm,       //
                          comm_data_t *cd, //
                          int *store_part, //
                          char *cursp) {
  int npart = cd->npart;
  int spsize = cd->n * (cd->nlevel + 1);
  for (int i = 0; i < spsize; i++)
    if (cursp[i])
      store_part[i] = perm[store_part[i]];
}

void reduce_comm(int phase,        //
                 comm_data_t *cd,  //
                 char *comm_table, //
                 int *store_part,  //
                 char *cursp) {
  int *perm = (int *)malloc(sizeof(*perm) * cd->npart);
  int *sums = (int *)malloc(sizeof(*sums) * cd->npart * cd->npart);
  make_sums(sums, cd, comm_table);
  lapjv(sums, cd->npart, perm);

  apply_perm_cd(phase, perm, cd);
  apply_perm_sp(phase, perm, cd, store_part, cursp);

  free(sums);
  free(perm);
}
