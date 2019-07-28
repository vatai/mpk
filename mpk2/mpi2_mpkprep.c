#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mpi.h>

#include "lib.h"
#include "mpi2_lib.h"

static int get_ct_idx(mpk_t *mg, int src_part, int tgt_part, int vv_idx) {
  int from_part_to_part = mg->npart * src_part + tgt_part;
  return mg->nlevel * mg->n * from_part_to_part + vv_idx;
}

comm_data_t *new_comm_data(mpk_t *mg) {
  comm_data_t *cd = malloc(sizeof(*cd));
  cd->n = mg->n;
  cd->nlevel = mg->nlevel;
  int npart = cd->npart = mg->npart;
  int nphase = cd->nphase = mg->nphase;

  // Send and receive buffers for vertex values (from vv).
  cd->vv_sbufs = malloc(sizeof(*cd->vv_sbufs) * (nphase + 1));
  assert(cd->vv_sbufs != NULL);
  cd->vv_rbufs = malloc(sizeof(*cd->vv_rbufs) * (nphase + 1));
  assert(cd->vv_rbufs != NULL);
  // Send and receive buffers for (vv) indices.
  cd->idx_sbufs = malloc(sizeof(*cd->idx_sbufs) * (nphase + 1));
  assert(cd->idx_sbufs != NULL);
  cd->idx_rbufs = malloc(sizeof(*cd->idx_rbufs) * (nphase + 1));
  assert(cd->idx_rbufs != NULL);

  // `sendcounts[phase * npart + p]` and `recvcounts[phase * npart +
  // p]` are is the number of elements sent/received to/from partition
  // `p`.
  cd->sendcounts = malloc(sizeof(*cd->sendcounts) * npart * (nphase + 1));
  assert(cd->sendcounts != NULL);
  cd->recvcounts = malloc(sizeof(*cd->recvcounts) * npart * (nphase + 1));
  assert(cd->recvcounts != NULL);
  cd->phase_scnt = malloc(sizeof(*cd->phase_scnt) * (nphase + 1));
  assert(cd->phase_scnt != NULL);
  cd->phase_rcnt = malloc(sizeof(*cd->phase_rcnt) * (nphase + 1));
  assert(cd->phase_rcnt != NULL);

  // `sdispls[phase * npart + p]` and `rsdispls[phase * npart + p]` is
  // the displacement (index) in the send/receive buffers where the
  // elements sent to partition/process `p` start.
  cd->sdispls = malloc(sizeof(*cd->sdispls) * npart * (nphase + 1));
  assert(cd->sdispls != NULL);
  cd->rdispls = malloc(sizeof(*cd->rdispls) * npart * (nphase + 1));
  assert(cd->rdispls != NULL);

  return cd;
}

void del_comm_data(comm_data_t *cd) {
  int i;
  free(cd->rdispls);
  free(cd->sdispls);
  free(cd->recvcounts);
  free(cd->sendcounts);
  free(cd->phase_rcnt);
  free(cd->phase_scnt);

  // TODO(vatai): receive buffers can't be deallocated. Why? Check
  // this after the communication part is written.
  for (i = 1; i < cd->nphase; i++) {
    free(cd->idx_rbufs[i]);
    free(cd->idx_sbufs[i]);
    free(cd->vv_rbufs[i]);
    free(cd->vv_sbufs[i]);
  }
  free(cd->idx_rbufs);
  free(cd->idx_sbufs);
  free(cd->vv_rbufs);
  free(cd->vv_sbufs);

  cd->nphase = 0;
  cd->npart = 0;
  cd->nlevel = 0;
  cd->n = 0;
}

static char *new_comm_table(mpk_t *mg) {
  // Communication of which vertex (n) from which level (nlevel)
  // from which process/partition (npart) to which process/partition
  // (npart).
  int num_ct_elem = mg->nlevel * mg->n * mg->npart * mg->npart;
  char *ct = malloc(sizeof(*ct) * num_ct_elem);
  assert(ct != NULL);
  return ct;
}

static int *new_store_part(mpk_t *mg) {
  // Initialise store_part[i] to be the id of the 0th partition for
  // level 0, and -1 for all other levels
  int n = mg->n;
  int *pl = mg->plist[0]->part;
  int sp_num_elem = n * (mg->nlevel + 1);
  int *store_part = (int*) malloc(sizeof(*store_part) * sp_num_elem);
  assert(store_part != NULL);
  for (int i = 0; i < n * (mg->nlevel + 1); i++)
    store_part[i] = i < n ? pl[i] : -1;
  return store_part;
}

static void proc_vertex(int curpart, int i, int l, mpk_t *mg, char *comm_table,
                        int *store_part) {
  crs0_t *g0 = mg->g0;
  assert(g0 != NULL);
  for (int j = g0->ptr[i]; j < g0->ptr[i + 1]; j++) { // All neighbours
    int k_vvidx = mg->n * (l - 1) + g0->col[j];       // source vv index
    assert(store_part[k_vvidx] != -1);
    if (store_part[k_vvidx] != curpart) {
      int idx = get_ct_idx(mg, store_part[k_vvidx], curpart, k_vvidx);
      comm_table[idx] = 1;
    }
  }
}

static void clear_comm_table(mpk_t *mg, char *comm_table) {
  // Communication of which vertex (n) from which level (nlevel)
  // from which process/partition (npart) to which process/partition
  // (npart).
  int N = mg->nlevel * mg->n * mg->npart * mg->npart;
  for (int i = 0; i < N; i++)
    comm_table[i] = 0;
}

static int min_or_0(mpk_t *mg, int phase) {
  if (mg->nphase == 0) return 0;
  int *ll = mg->llist[phase]->level;
  int rv = ll[0];
  for (int i = 0; i < mg->n; i++)
    if (ll[i] < rv)
      rv = ll[i];
  return rv;
}

static int amax(int *ll, int n) {
  int rv = ll[0];
  for (int i = 0; i < n; i++)
    if (ll[i] > rv)
      rv = ll[i];
  return rv;
}

static void zeroth_comm_table(mpk_t *mg, char *comm_table,
                             int *store_part) {
  int n = mg->n;
  int *ll = mg->llist[0]->level;
  clear_comm_table(mg, comm_table);
  for (int i = 0; i < n; i++) {
    int part = mg->plist[0]->part[i];
    int idx = get_ct_idx(mg, part, part, i);
    comm_table[idx] = 1;
  }
  int lmax = amax(ll, n);
  for (int l = 1; l <= lmax; l++) { // initially prevlmin =0
    for (int i = 0; i < n; i++) {
      int i_vvidx = n * l + i;
      if (0 < l && l <= ll[i] && store_part[i_vvidx] == -1) {
        int curpart = mg->plist[0]->part[i];
        proc_vertex(curpart, i, l, mg, comm_table, store_part);
        store_part[i_vvidx] = curpart;
      }
    }
  }
}

static void phase_comm_table(int phase, mpk_t *mg, char *comm_table,
                             int *store_part) {
  int n = mg->n;
  int *pl = mg->plist[phase]->part;
  int *ll = mg->llist[phase]->level;
  int *prevl = mg->llist[phase - 1]->level;
  int lmax = amax(ll, n);
  int prevlmin = min_or_0(mg, phase - 1);
  clear_comm_table(mg, comm_table);
  for (int l = prevlmin + 1; l <= lmax; l++) { // initially prevlmin =0
    for (int i = 0; i < n; i++) {
      int i_vvidx = n * l + i;
      if (prevl[i] < l && l <= ll[i] && store_part[i_vvidx] == -1) {
        int curpart = mg->plist[phase]->part[i];
        proc_vertex(curpart, i, l, mg, comm_table, store_part);
        store_part[i_vvidx] = curpart;
      }
    }
  }
}

static void skirt_comm_table(mpk_t *mg, char *comm_table, int *store_part) {
  int n = mg->n;
  int nlevel = mg->nlevel;
  int npart = mg->npart;
  int nphase = mg->nphase;
  int *sl = mg->sg->levels;
  int prevlmin = min_or_0(mg, nphase - 1);
  clear_comm_table(mg, comm_table);
  for (int p = 0; p < npart; p++) {
    for (int l = prevlmin + 1; l <= nlevel; l++) {
      for (int i = 0; i < n; i++) {
        // TODO(vatai): Why is the commented out condition breaking
        // the program?
        int above_prevl = (nphase ? mg->llist[nphase - 1]->level[i] : 0) < l;
        int skirt_active = sl[p * n + i] >= 0;
        int not_over_max = l <= nlevel - sl[p * n + i];
        if (/* store_part[i_vvidx] != -1 && */ above_prevl && skirt_active &&
            not_over_max) {
          proc_vertex(p, i, l, mg, comm_table, store_part);
          store_part[n * l + i] = p;
        }
      }
    }
  } // end partition loop
}

static void fill_counts(int phase, char *comm_table, mpk_t *mg,
                              comm_data_t *cd) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  cd->phase_scnt[phase] = 0;
  cd->phase_rcnt[phase] = 0;
  for (int p = 0; p < mg->npart; p++) {
    cd->sendcounts[phase * mg->npart + p] = 0;
    cd->recvcounts[phase * mg->npart + p] = 0;
  }
  for (int p = 0; p < mg->npart; p++) {
    for (int i = 0; i < mg->n * mg->nlevel; i++) {
      int idx = get_ct_idx(mg, rank, p, i);
      if (comm_table[idx]) {
        cd->phase_scnt[phase]++;
        cd->sendcounts[phase * mg->npart + p]++;
      }
      idx = get_ct_idx(mg, p, rank, i);
      if (comm_table[idx]) {
        cd->phase_rcnt[phase]++;
        cd->recvcounts[phase * mg->npart + p]++;
      }
    }
  }
}

static void fill_displs(int phase, mpk_t *mg, comm_data_t *cd) {
  // Needs sendcounts and recvcounts filled for phase.
  for (int p = 0; p < mg->npart; p++) {
    int idx = phase * mg->npart + p;
    if (p == 0) {
      cd->sdispls[idx] = 0;
      cd->rdispls[idx] = 0;
    } else {
      cd->sdispls[idx] = cd->sdispls[idx - 1] + cd->sendcounts[idx - 1];
      cd->rdispls[idx] = cd->rdispls[idx - 1] + cd->recvcounts[idx - 1];
    }
  }
}

static void alloc_bufs(int phase, comm_data_t *cd) {
  // Needs phase_scnt and phase_rcnt filled.
  cd->vv_sbufs[phase] = malloc(sizeof(*cd->vv_sbufs[phase]) * cd->phase_scnt[phase]);
  assert(cd->vv_sbufs[phase] != NULL);
  cd->vv_rbufs[phase] = malloc(sizeof(*cd->vv_rbufs[phase]) * cd->phase_rcnt[phase]);
  assert(cd->vv_rbufs[phase] != NULL);
  cd->idx_sbufs[phase] = malloc(sizeof(*cd->idx_sbufs[phase]) * cd->phase_scnt[phase]);
  assert(cd->idx_sbufs[phase] != NULL);
  cd->idx_rbufs[phase] = malloc(sizeof(*cd->idx_rbufs[phase]) * cd->phase_rcnt[phase]);
  assert(cd->idx_rbufs[phase] != NULL);
}

static void fill_idx_buffers(int phase, char *comm_table, mpk_t *mg,
                             comm_data_t *cd) {
  // Needs idx buffers allocated.
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int scounter = 0;
  int rcounter = 0;
  for (int p = 0; p < mg->npart; p++) {
    // For all vv indices.
    for (int i = 0; i < mg->nlevel * mg->n; ++i) {
      // Here p is the destination (to) partition.
      int sidx = get_ct_idx(mg, rank, p, i);
      if (comm_table[sidx]) {
        cd->idx_sbufs[phase][scounter] = i;
        scounter++;
      }
      int ridx = get_ct_idx(mg, p, rank, i);
      if (comm_table[ridx]) {
        cd->idx_rbufs[phase][rcounter] = i;
        rcounter++;
      }
    }
  }
}

/*
 * Convert `comm_table[]` to comm. data `cd`.
 */
static void fill_comm_data(int phase, char *comm_table, mpk_t *mg,
                           comm_data_t *cd) {
  // Fills cd members from the information from comm_table.
  fill_counts(phase, comm_table, mg, cd);
  fill_displs(phase, mg, cd);
  alloc_bufs(phase, cd);
  fill_idx_buffers(phase, comm_table, mg, cd);
}

void testcomm_table(mpk_t *mg, char *comm_table, int phase, int rank) {
  if (rank == 0) {
    printf("Testing and printing commtable in a text doc\n");
    char name[100];
    sprintf(name, "Phase%d_Commtable.log", phase);
    FILE *f = fopen(name, "w");
    if (f == NULL) {
      fprintf(stderr, "cannot open %s\n", name);
      exit(1);
    }
    int n = mg->n;
    int npart = mg->npart;
    int nlevel = mg->nlevel;

    for (int i = 0; i < npart; ++i) {
      fprintf(f, "Source Partition = %d\n", i);
      for (int j = 0; j < npart; ++j) {
        fprintf(f, "Target Partition=%d\n", j);
        for (int k = 0; k < n * nlevel; ++k) {
          if (comm_table[(i * npart * n * nlevel) + (j * n * nlevel) + k]) {
            fprintf(f, " vvposition = %d vvlevel = %d\n", (k % 16), k / 16);
          }
        }
        fprintf(f, "\n");
      }
    }
  }
}

/*
 * Allocate and fill `comm_data_t cd`.
 */
void mpi_prep_mpk(mpk_t *mg, comm_data_t *cd) {
  assert(mg != NULL);
  printf("preparing mpi buffers for communication...");

  char *comm_table = new_comm_table(mg);
  int *store_part = new_store_part(mg);

  if (mg->nphase > 0) {
    assert(mg->plist[0] != NULL);
    zeroth_comm_table(mg, comm_table, store_part);
    fill_comm_data(0, comm_table, mg, cd);
  }
  for (int phase = 1; phase < mg->nphase; phase++) {
    assert(mg->plist[phase] != NULL);
    phase_comm_table(phase, mg, comm_table, store_part);
    fill_comm_data(phase, comm_table, mg, cd);
  }
  skirt_comm_table(mg, comm_table, store_part);
  fill_comm_data(mg->nphase, comm_table, mg, cd);

  free(comm_table);
  free(store_part);
  printf(" done\n");
}

