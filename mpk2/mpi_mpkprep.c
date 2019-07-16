#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mpi.h>

#include "lib.h"
#include "mpi_lib.h"

static int get_ct_idx(mpk_t *mg, int src_part, int tgt_part, int vv_idx) {
  int from_part_to_part = mg->npart * src_part + tgt_part;
  return mg->nlevel * mg->n * from_part_to_part + vv_idx;
}

static void new_cd(mpk_t *mg, comm_data_t *cd) {
  cd->n = mg->n;
  cd->npart = mg->npart;
  cd->nlevel = mg->nlevel;
  cd->nphase = mg->nphase;

  // Send and receive buffers for vertex values (from vv).
  cd->vv_sbufs = malloc(sizeof(*cd->vv_sbufs) * (mg->nphase + 1));
  cd->vv_rbufs = malloc(sizeof(*cd->vv_rbufs) * (mg->nphase + 1));
  // Send and receive buffers for (vv) indices.
  cd->idx_sbufs = malloc(sizeof(*cd->idx_sbufs) * (mg->nphase + 1));
  cd->idx_rbufs = malloc(sizeof(*cd->idx_rbufs) * (mg->nphase + 1));

  // `sendcounts[phase * npart + p]` and `recvcounts[phase * npart +
  // p]` are is the number of elements sent/received to/from partition
  // `p`.
  cd->sendcounts =
      malloc(sizeof(*cd->sendcounts) * mg->npart * (mg->nphase + 1));
  cd->recvcounts =
      malloc(sizeof(*cd->recvcounts) * mg->npart * (mg->nphase + 1));

  // `sdispls[phase * npart + p]` and `rsdispls[phase * npart + p]` is
  // the displacement (index) in the send/receive buffers where the
  // elements sent to partition/process `p` start.
  cd->sdispls = malloc(sizeof(*cd->sdispls) * mg->npart * (mg->nphase + 1));
  cd->rdispls = malloc(sizeof(*cd->rdispls) * mg->npart * (mg->nphase + 1));
}

static int *new_comm_table(mpk_t *mg) {
  int num_ct_elem = mg->nlevel * mg->n * mg->npart * mg->npart;
  return malloc(sizeof(int) * num_ct_elem);
}

static int *new_store_part(mpk_t *mg) {
  // Initialise store_part[i] to be the id of the 0th partition for
  // level 0, and -1 for all other levels
  int n = mg->n;
  int *pl = mg->plist[0]->part;
  int sp_num_elem = n * (mg->nlevel + 1);
  int *store_part = (int*) malloc(sizeof(*store_part) * sp_num_elem);
  for (int i = 0; i < n * (mg->nlevel + 1); i++)
    store_part[i] = i < n ? pl[i] : -1;
  return store_part;
}

static void proc_vertex(int curpart, int i, int l, mpk_t *mg, int *comm_table,
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

static void clear_comm_table(mpk_t *mg, int *comm_table) {
  // Communication of which vertex (n) from which level (nlevel)
  // from which process/partition (npart) to which process/partition
  // (npart).
  int N = mg->nlevel * mg->n * mg->npart * mg->npart;
  for (int i = 0; i < N; i++)
    comm_table[i] = 0;
}

static void phase_comm_table(int phase, mpk_t *mg, int *comm_table,
                             int *store_part, int prevlmin, int lmax,
                             int *prevl) {
  // Communication of which vertex (n) from which level (nlevel)
  // from which process/partition (npart) to which process/partition
  // (npart).
  int *pl = mg->plist[phase]->part;
  int n = mg->n;
  clear_comm_table(mg, comm_table);
  int *ll = mg->llist[phase]->level;
  for (int l = prevlmin + 1; l <= lmax; l++) { // initially prevlmin =0
    for (int i = 0; i < n; i++) {
      int i_vvidx = n * l + i;
      if (prevl[i] < l && l <= ll[i] && store_part[i_vvidx] == -1) {
        int curpart = mg->plist[phase]->part[i];
        if (phase != 0)
          proc_vertex(curpart, i, l, mg, comm_table, store_part);
        store_part[i_vvidx] = curpart;
      }
    }
  }
}

static void skirt_comm_table(mpk_t *mg, int *comm_table, int *store_part,
                             int prevlmin) {
  int n = mg->n;
  int nlevel = mg->nlevel;
  int npart = mg->npart;
  int nphase = mg->nphase;
  int *sl = mg->sg->levels;
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

static void lminmax(int phase, mpk_t *mg, int *_lmin, int *_lmax) {
  assert(mg->llist[phase] != NULL);
  int lmin, lmax;
  int *ll = mg->llist[phase]->level;
  lmin = lmax = ll[0];
  for (int i = 1; i < mg->n; i++) {
    if (lmin > ll[i])
      lmin = ll[i];
    if (lmax < ll[i])
      lmax = ll[i];
  }

  if (lmax > mg->nlevel)
    lmax = mg->nlevel;

  *_lmin = lmin;
  *_lmax = lmax;
}

/*
 * Convert `comm_table[]` to comm. data `cd`.
 */
static void mpi_prepbufs_mpk(mpk_t *mg, int *comm_table, comm_data_t *cd,
                             int phase) {
  int npart = mg->npart;
  int nlevel = mg->nlevel;
  int n = mg->n;
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // rcount and scount are pointers to the first element of the
  // recvcount and sendcount for the current phase.
  int *rcount = cd->recvcounts + phase * npart;
  int *scount = cd->sendcounts + phase * npart;
  int *rdisp = cd->rdispls + phase * npart;
  int *sdisp = cd->sdispls + phase * npart;

  int numb_of_send = 0;
  int numb_of_rec = 0;
  int i;
  for (i = 0; i < npart; i++) {
    rcount[i] = 0;
    scount[i] = 0;
  }

  // For all "other" partitions `p` (other = other partitions we are
  // sending to, and other partitions we are receiving from).
  // [Note] Each partition is creating its unique scount,sdisp
  // rcount and rdisp
  for (int p = 0; p < npart; p++) {
    // For all vv indices.
    for (i = 0; i < nlevel * n; i++) {
      // Here p is the source (from) partition.
      int idx = get_ct_idx(mg, p, rank, i);
      if (comm_table[idx]) {
        // So we increment:
        numb_of_rec++;
        rcount[p]++;
      }
      // Here `p` is the target (to) partition.
      idx = get_ct_idx(mg, rank, p, i);
      if (comm_table[idx]) {
        // So we increment:
        numb_of_send++;
        scount[p]++;
      }
    }
    // Do a scan on rdisp/sdisp.
    if (p == 0) {
      sdisp[p] = 0;
      rdisp[p] = 0;
    } else {
      sdisp[p] = sdisp[p - 1] + scount[p - 1];
      rdisp[p] = rdisp[p - 1] + rcount[p - 1];
    }
  }

  cd->vv_sbufs[phase] = malloc(sizeof(*cd->vv_sbufs[phase]) * numb_of_send);
  cd->vv_rbufs[phase] = malloc(sizeof(*cd->vv_rbufs[phase]) * numb_of_rec);
  cd->idx_sbufs[phase] = malloc(sizeof(*cd->idx_sbufs[phase]) * numb_of_send);
  cd->idx_rbufs[phase] = malloc(sizeof(*cd->idx_rbufs[phase]) * numb_of_rec);

  int counter = 0;
  for (int p = 0; p < npart; p++) {
    // For all vv indices.
    for (int i = 0; i < nlevel * n; ++i) {
      // Here p is the destination (to) partition.
      int idx = get_ct_idx(mg, rank, p, i);
      if (comm_table[idx]) {
        // cd->vv_sbufs[phase][counter] = vv[i];
        cd->idx_sbufs[phase][counter] = i;
        counter++;
      }
    }
  }
}

void testcomm_table(mpk_t *mg, int *comm_table, int phase, int rank) {
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
  printf("preparing mpi buffers for communication...");  fflush(stdout);

  assert(mg != NULL);
  int n = mg->n;

  new_cd(mg, cd);
  int *comm_table = new_comm_table(mg);
  int *store_part = new_store_part(mg);

  assert(mg->plist[0] != NULL);

  int l0[n];
  for (int i = 0; i < n; i++)
    l0[i] = 0;
  int *prevl = l0;
  int prevlmin = 0;
  int *prevpartl = mg->plist[0]->part;

  for (int phase = 0; phase < mg->nphase; phase++) {
    assert(mg->plist[phase] != NULL);
    if (phase > 0)
      prevpartl = mg->plist[phase - 1]->part;

    int lmin, lmax;
    lminmax(phase, mg, &lmin, &lmax);
    phase_comm_table(phase, mg, comm_table, store_part, prevlmin, lmax, prevl);

    if (phase != 0)
      mpi_prepbufs_mpk(mg, comm_table, cd, phase);

    // Prepare for the next phase.
    prevl = mg->llist[phase]->level;
    prevlmin = lmin;
  }

  skirt_comm_table(mg, comm_table, store_part, prevlmin);
  mpi_prepbufs_mpk(mg, comm_table, cd, mg->nphase);

  free(comm_table);
  free(store_part);
  printf(" done\n");
}

void mpi_del_cd(comm_data_t *cd) {
  int i;
  free(cd->rdispls);
  free(cd->sdispls);
  free(cd->recvcounts);
  free(cd->sendcounts);

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
