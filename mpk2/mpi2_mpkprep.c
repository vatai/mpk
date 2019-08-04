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
  cd->mg = mg;
  cd->n = mg->n;
  cd->nlevel = mg->nlevel;
  int npart = cd->npart = mg->npart;
  int nphase = cd->nphase = mg->nphase;

  cd->recvcounts = malloc(sizeof(*cd->recvcounts) * npart * (nphase + 1));
  assert(cd->recvcounts != NULL);
  cd->sendcounts = malloc(sizeof(*cd->sendcounts) * npart * (nphase + 1));
  assert(cd->sendcounts != NULL);

  cd->rdispls = malloc(sizeof(*cd->rdispls) * npart * (nphase + 1));
  assert(cd->rdispls != NULL);
  cd->sdispls = malloc(sizeof(*cd->sdispls) * npart * (nphase + 1));
  assert(cd->sdispls != NULL);

  cd->rcount = malloc(sizeof(*cd->rcount) * (nphase + 1));
  assert(cd->rcount != NULL);
  cd->mcount = malloc(sizeof(*cd->mcount) * (nphase + 1));
  assert(cd->mcount != NULL);
  cd->scount = malloc(sizeof(*cd->scount) * (nphase + 1));
  assert(cd->scount != NULL);

  cd->idx_rbufs = malloc(sizeof(*cd->idx_rbufs) * (nphase + 1));
  assert(cd->idx_rbufs != NULL);
  cd->idx_mbufs = malloc(sizeof(*cd->idx_mbufs) * (nphase + 1));
  assert(cd->idx_mbufs != NULL);
  cd->idx_sbufs = malloc(sizeof(*cd->idx_sbufs) * (nphase + 1));
  assert(cd->idx_sbufs != NULL);

  cd->vv_rbufs = malloc(sizeof(*cd->vv_rbufs) * (nphase + 1));
  assert(cd->vv_rbufs != NULL);
  cd->vv_mbufs = malloc(sizeof(*cd->vv_mbufs) * (nphase + 1));
  assert(cd->vv_mbufs != NULL);
  cd->vv_sbufs = malloc(sizeof(*cd->vv_sbufs) * (nphase + 1));
  assert(cd->vv_sbufs != NULL);

  return cd;
}

void del_comm_data(comm_data_t *cd) {
  free(cd->recvcounts);
  free(cd->sendcounts);
  free(cd->rdispls);
  free(cd->sdispls);
  free(cd->rcount);
  free(cd->mcount);
  free(cd->scount);

  free(cd->idx_rbufs);
  free(cd->idx_mbufs);
  free(cd->idx_sbufs);
  free(cd->vv_rbufs);
  free(cd->vv_mbufs);
  free(cd->vv_sbufs);

  free(cd->idx_buf);
  free(cd->vv_buf);

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

static void fill_comm_table_one_vertex(int curpart, int i, int level, mpk_t *mg,
                                       char *comm_table, int *store_part) {
  crs0_t *g0 = mg->g0;
  assert(g0 != NULL);
  for (int j = g0->ptr[i]; j < g0->ptr[i + 1]; j++) { // All neighbours
    int k_vvidx = mg->n * (level - 1) + g0->col[j];   // source vv index
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

static void zeroth_comm_table(mpk_t *mg, char *comm_table, int *store_part) {
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
        fill_comm_table_one_vertex(curpart, i, l, mg, comm_table, store_part);
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
      if (prevl[i] < l && l <= ll[i]) {
        int curpart = mg->plist[phase]->part[i];
        fill_comm_table_one_vertex(curpart, i, l, mg, comm_table, store_part);
        store_part[i_vvidx] = curpart;
      }
    }
  }
}

static void skirt_comm_table(mpk_t *mg, char *comm_table, int *store_part) {
  int n = mg->n;
  int nlevel = mg->nlevel;
  int nphase = mg->nphase;
  int *sl = mg->sg->levels;
  int prevlmin = min_or_0(mg, nphase - 1);
  clear_comm_table(mg, comm_table);
  for (int p = 0; p < mg->npart; p++) {
    for (int l = prevlmin + 1; l <= nlevel; l++) {
      for (int i = 0; i < n; i++) {
        int above_prevl = (nphase ? mg->llist[nphase - 1]->level[i] : 0) < l;
        int skirt_active = sl[p * n + i] >= 0;
        int not_over_max = l <= nlevel - sl[p * n + i];
        if (above_prevl && skirt_active && not_over_max) {
          fill_comm_table_one_vertex(p, i, l, mg, comm_table, store_part);
          if (store_part[n * l + i] == -1)
            store_part[n * l + i] = p;
        }
      }
    }
  } // end partition loop
}

// Input:
// - filled comm_table
//
// Output: cd->
// - scount[phase], rcount[phase],
// - sendcount[phase, part], recvvount[phase, part]
static void fill_rscounts(int phase, comm_data_t *cd, char *comm_table) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  cd->scount[phase] = 0;
  cd->rcount[phase] = 0;
  for (int p = 0; p < cd->npart; p++) {
    cd->sendcounts[phase * cd->npart + p] = 0;
    cd->recvcounts[phase * cd->npart + p] = 0;
  }
  for (int p = 0; p < cd->npart; p++) {
    for (int i = 0; i < cd->n * cd->nlevel; i++) {
      int idx = get_ct_idx(cd->mg, rank, p, i);
      if (comm_table[idx]) {
        cd->scount[phase]++;
        cd->sendcounts[phase * cd->npart + p]++;
      }
      idx = get_ct_idx(cd->mg, p, rank, i);
      if (comm_table[idx]) {
        cd->rcount[phase]++;
        cd->recvcounts[phase * cd->npart + p]++;
      }
    }
  }
}

static void fill_displs(int phase, comm_data_t *cd) {
  // Needs sendcounts and recvcounts filled for phase.
  for (int p = 0; p < cd->npart; p++) {
    int idx = phase * cd->npart + p;
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
  // Needs scount and rcount filled.
  cd->vv_sbufs[phase] = malloc(sizeof(*cd->vv_sbufs[phase]) * cd->scount[phase]);
  assert(cd->vv_sbufs[phase] != NULL);
  cd->vv_rbufs[phase] = malloc(sizeof(*cd->vv_rbufs[phase]) * cd->rcount[phase]);
  assert(cd->vv_rbufs[phase] != NULL);
  cd->idx_sbufs[phase] = malloc(sizeof(*cd->idx_sbufs[phase]) * cd->scount[phase]);
  assert(cd->idx_sbufs[phase] != NULL);
  cd->idx_rbufs[phase] = malloc(sizeof(*cd->idx_rbufs[phase]) * cd->rcount[phase]);
  assert(cd->idx_rbufs[phase] != NULL);
}

static void fill_idx_rsbuf(int phase, char *comm_table, comm_data_t *cd) {
  // Needs idx buffers allocated.
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int scounter = 0;
  int rcounter = 0;
  for (int p = 0; p < cd->npart; p++) {
    // For all vv indices.
    for (int i = 0; i < cd->nlevel * cd->n; ++i) {
      // Here p is the destination (to) partition.
      int sidx = get_ct_idx(cd->mg, rank, p, i);
      if (comm_table[sidx]) {
        cd->idx_sbufs[phase][scounter] = i;
        scounter++;
      }
      int ridx = get_ct_idx(cd->mg, p, rank, i);
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
static void fill_comm_data(int phase, char *comm_table, comm_data_t *cd) {
  fill_rscounts(phase, cd, comm_table);
  fill_displs(phase, cd);
  alloc_bufs(phase, cd);
  fill_idx_rsbuf(phase, comm_table, cd);
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

// NEWPREP_BEGIN
/* TODO(vatai): new_perp */
/* - no need for rank? */
/* - remove assert */

// Fill all buffer size variables to make allocation possible.
static int get_prevlmin(int phase, comm_data_t *cd) {
  int *prevl = cd->mg->llist[phase - 1]->level;
  int prevlmin = 0;
  if (phase > 0) {
    prevlmin = prevl[0];
    for (int i = 1; i < cd->n; i++)
      if (prevl[i] < prevlmin)
        prevlmin = prevl[i];
  }
  return prevlmin;
}

static void fill_mcounts(int phase, comm_data_t *cd, char *comm_table,
                                  int *store_part) {
  // TODO(vatai): DRY violation - almost the same as fill_idx_mbufs
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int prevlmin = get_prevlmin(phase, cd);
  int count = 0;
  for (int level = prevlmin + 1; level <= cd->nlevel; level++) {
    for (int i = 0; i < cd->n; i++) {
      int prevli = phase ? cd->mg->llist[phase - 1]->level[i] : 0;
      int curpart = phase == cd->nphase ? cd->mg->plist[0]->part[i] :
                    cd->mg->plist[phase]->part[i];
      int curli = phase == cd->nphase ? cd->mg->sg->levels[rank * cd->n + i] :
                  cd->mg->llist[phase]->level[i];
      if (prevli < level && level <= curli && rank == curpart)
        count++;
    }
  }
  cd->mcount[phase] = count;
}

static void fill_idx_mbuf(int phase, char *comm_table, comm_data_t *cd) {
  // TODO(vatai): DRY violation - almost the same as fill_tlist_counts
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int *prevl = cd->mg->llist[phase - 1]->level;
  int prevlmin = get_prevlmin(phase, cd);
  int count = 0;
  for (int level = prevlmin + 1; level <= cd->nlevel; level++) {
    for (int i = 0; i < cd->n; i++) {
      int prevli = phase ? cd->mg->llist[phase - 1]->level[i] : 0;
      int curpart = phase == cd->nphase ? cd->mg->plist[0]->part[i] :
                    cd->mg->plist[phase]->part[i];
      int curli = phase == cd->nphase ? cd->mg->sg->levels[rank * cd->n + i] :
                  cd->mg->llist[phase]->level[i];
      if (prevli < level && level <= curli && rank == curpart) {
        cd->idx_mbufs[phase][count] = level * cd->n + i;
        count++;
      }
    }
  }
}

static void skirt_fill_mcounts(comm_data_t *cd, char *comm_table) {
  // TODO(vatai): DRY violation - almost the same as skirt_fill_idx_mbufs
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int *prevl = cd->mg->llist[cd->nphase - 1]->level;
  int prevlmin = get_prevlmin(cd->nphase, cd);
  int count = 0;
  task_t *tl = cd->mg->tlist + cd->nphase * cd->npart + rank;
  for (int level = prevlmin + 1; level <= cd->nlevel; level++) {
    for (int i = 0; i < cd->n; i++) {
      int prevli = cd->nphase ? prevl[i] : 0;
      // TODO(vatai): new_prep: just for rank
      int slpi = cd->mg->sg->levels[rank * cd->n + i];
      if (prevli < level && 0 <= slpi && level <= cd->nlevel - slpi)
        count++;
    }
  }

  // TODO(vatai): remove this line: assert(tl->n == count);
  cd->mcount[cd->nphase] = count;
}

static void skirt_fill_idx_mbuf(comm_data_t *cd, char *comm_table) {
  // TODO(vatai): DRY violation - almost the same as skirt_fill_mcount
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int *prevl = cd->mg->llist[cd->nphase - 1]->level;
  int prevlmin = get_prevlmin(cd->nphase, cd);
  int count = 0;
  task_t *tl = cd->mg->tlist + cd->nphase * cd->npart + rank;
  for (int level = prevlmin + 1; level <= cd->nlevel; level++) {
    for (int i = 0; i < cd->n; i++) {
      int prevli = cd->nphase ? prevl[i] : 0;
      // TODO(vatai): new_prep: just for rank
      int slpi = cd->mg->sg->levels[rank * cd->n + i];
      if (prevli < level && 0 <= slpi && level <= cd->nlevel - slpi) {
        cd->idx_mbufs[cd->nphase][count] = level * cd->n + i;
        count++;
      }
    }
  }
}

// WET (i.e. non-DRY) code above

static void fill_bufsize_rscount_displs(comm_data_t *cd, char *comm_table, int *store_part) {
  zeroth_comm_table(cd->mg, comm_table, store_part);
  fill_mcounts(0, cd, comm_table, store_part);
  fill_rscounts(0, cd, comm_table);
  fill_displs(0, cd);
  for (int phase = 1; phase < cd->nphase; phase++) {
    phase_comm_table(phase, cd->mg, comm_table, store_part);
    fill_mcounts(phase, cd, comm_table, store_part);
    fill_rscounts(phase, cd, comm_table);
    fill_displs(phase, cd);
  }
  skirt_comm_table(cd->mg, comm_table, store_part);
  skirt_fill_mcounts(cd, comm_table);
  fill_rscounts(cd->nphase, cd, comm_table);
  fill_displs(cd->nphase, cd);
}

static void new_alloc_comm_data(comm_data_t *cd) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  long count = 0;
  for (int phase = 0; phase <= cd->nphase; phase++) {
    count += cd->rcount[phase];
    count += cd->mcount[phase];
    count += cd->scount[phase];
  }
  cd->buf_count = count;

  cd->idx_buf = malloc(sizeof(*cd->idx_buf) * count);
  cd->vv_buf = malloc(sizeof(*cd->vv_buf) * count);
  assert(cd->idx_buf != NULL);
  assert(cd->vv_buf != NULL);

  count = 0;
  for (int phase = 0; phase <= cd->nphase; phase++) {
    cd->idx_rbufs[phase] = cd->idx_buf + count;
    cd->vv_rbufs[phase] = cd->vv_buf + count;
    count += cd->rcount[phase];
    cd->idx_mbufs[phase] = cd->idx_buf + count;
    cd->vv_mbufs[phase] = cd->vv_buf + count;
    count += cd->mcount[phase];
    cd->idx_sbufs[phase] = cd->idx_buf + count;
    cd->vv_sbufs[phase] = cd->vv_buf + count;
    count += cd->scount[phase];
  }
}

// NEWPREP_END

/*
 * Allocate and fill `comm_data_t cd`.
 */
void mpi_prep_mpk(comm_data_t *cd) {
  assert(cd->mg != NULL);
  printf("preparing mpi buffers for communication...");

  char *comm_table = new_comm_table(cd->mg);
  int *store_part = new_store_part(cd->mg);

  // Stage one fill
  fill_bufsize_rscount_displs(cd, comm_table, store_part);

  // Alloc of stage to memory
  new_alloc_comm_data(cd);

  // NEXT, add vv_bufs alloc and remove alloc
  if (cd->nphase > 0) {
    assert(cd->mg->plist[0] != NULL);
    zeroth_comm_table(cd->mg, comm_table, store_part);
    fill_idx_mbuf(0, comm_table, cd);
    fill_idx_rsbuf(0, comm_table, cd);
  }
  for (int phase = 1; phase < cd->nphase; phase++) {
    assert(cd->mg->plist[phase] != NULL);
    phase_comm_table(phase, cd->mg, comm_table, store_part);
    fill_idx_mbuf(phase, comm_table, cd);
    fill_idx_rsbuf(phase, comm_table, cd);
  }
  skirt_comm_table(cd->mg, comm_table, store_part);
  skirt_fill_idx_mbuf(cd, comm_table);
  fill_idx_rsbuf(cd->nphase, comm_table, cd);

  // TODO(vatai): NEXT_LIST below
  // NEXT: del mpk_t *mg;
  // NEXT: continue with mcol devel
  // NEXT: read_cd
  // NEXT: remove non-DRY code
  free(comm_table);
  free(store_part);
  printf(" done\n");
}
