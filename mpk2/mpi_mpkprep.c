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
  for (int i = 0; i < mg->nlevel * mg->n * mg->npart * mg->npart; i++)
    comm_table[i] = 0;
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
  // Similar to s/rcount.
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
  for (int p = 0; p < npart; ++p) {
    // For all vv indices.
    for (i = 0; i < nlevel * n; ++i) {
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
  for (int p = 0; p < npart; ++p) {
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
  assert(mg != NULL);

  printf("preparing mpi buffers for communication...");  fflush(stdout);

  int n = mg->n;
  cd->n = n;
  int npart = mg->npart;
  cd->npart = npart;
  int nlevel = mg->nlevel;
  cd->nlevel = nlevel;
  int nphase = mg->nphase;
  cd->nphase = nphase;

  crs0_t *g0 = mg->g0;
  assert(g0 != NULL);

  int i;
  assert(mg->plist[0] != NULL);
  int *pl = mg->plist[0]->part;
  int *prevpartl = mg->plist[0]->part;

  int l0[n];
  for (i=0; i< n; i++)
    l0[i] = 0;

  // store_part[i] is the partition number where vv[i] is stored.
  int *store_part = (int*) malloc(sizeof(*store_part) * n * (nlevel + 1));
  // Initialise store_part[i] to be the id of the 0th partition for
  // level 0, and -1 for all other levels
  for (i = 0; i < n; i++)
    store_part[i] = pl[i];
  for (i = 0; i < n * nlevel; i++)
    store_part[n + i] = -1;

  new_cd(mg, cd);
  int tcount = 0;
  int *prevl = l0;
  int prevlmin = 0;

  int *comm_table = malloc(sizeof(*comm_table) * nlevel * n * npart * npart);
  //______________________________________________
  int phase;
  for (phase = 0; phase < nphase; phase ++) {
    assert(mg->plist[phase] != NULL);
    pl = mg->plist[phase]->part;
    if (phase>0){
      prevpartl = mg->plist[phase-1]->part;
    }
    assert(mg->llist[phase] != NULL);
    int *ll = mg->llist[phase]->level;

    int lmin, lmax;
    lmin = lmax = ll[0];
    for (i=1; i< n; i++) { // give max and min value to lmax and lmin resp.
      if (lmin > ll[i])
	lmin = ll[i];
      if (lmax < ll[i])
	lmax = ll[i];
    }

    if (lmax > nlevel)
      lmax = nlevel;

    clear_comm_table(mg, comm_table);
    phase_comm_table(phase, mg, comm_table, store_part, prevlmin, lmax, prevl);

    if (phase != 0)
      mpi_prepbufs_mpk(mg, comm_table, cd, phase);

    // Prepare for the next phase.
    prevl = mg->llist[phase]->level;
    prevlmin = lmin;
  }

  // Skirt `comm_table`
  assert (phase == nphase);

  skirt_comm_table(mg, comm_table, store_part, prevlmin);
  mpi_prepbufs_mpk(mg, comm_table, cd, phase);

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
