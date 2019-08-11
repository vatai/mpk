#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mpi.h>

#include "lib.h"
#include "mpi2_lib.h"

static int find_idx(long *ptr, int size, long target) {
  for (int i = 0; i < size; i++)
    if (ptr[i] == target)
      return i;
  return -1;
}

// TODO(vatai): move to mpi2_comm_data.c
static void dir_name_error(char *dir) {
  fprintf(stderr, "  unknwon directory naming %s\n", dir);
  fprintf(stderr, "  expecting ghead_npart_nlevel_nphase\n");
  exit(1);
}

// TODO(vatai): move to mpi2_comm_data.c
static void read_dir(comm_data_t *cd) {
  int i = 0, j = 0;
  for (j= strlen(cd->dir); j >= 0; j--) {
    if (cd->dir[j] == '_') i++;
    if (i == 3) break;
  }
  if (!(i == 3 && j >= 0 && cd->dir[j] == '_'))
    dir_name_error(cd->dir);

  cd->npart = cd->nlevel = cd->nphase = -1;
  i = sscanf(cd->dir + j, "_%d_%d_%d", &cd->npart, &cd->nlevel, &cd->nphase);
  if (i != 3)
    dir_name_error(cd->dir);
  assert(cd->npart > 0 && cd->nlevel > 0 && cd->nphase >= 0);

  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  assert(world_size == cd->npart);
}

// TODO(vatai): move to mpi2_comm_data.c
static void read_matrix(comm_data_t *cd) {
  char fname[1024];
  FILE *f;
  sprintf(fname, "%s/g0", cd->dir);
  f = fopen(fname, "r");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }
  crs0_t *g0 = read_crs(f); // Closes f
  cd->graph = g0;
  cd->n = g0->n;
}

// TODO(vatai): move to mpi2_comm_data.c
static void alloc_mpk_data(comm_data_t *cd) {
  if (cd->nphase == 0) { // PA1
    cd->plist = (part_t**) malloc(sizeof(part_t*));
    cd->llist = NULL;
  } else {
    cd->plist = (part_t**) malloc(sizeof(part_t*) * cd->nphase);
    cd->llist = (level_t**) malloc(sizeof(level_t*) * cd->nphase);
    int i;
    for (i=0; i< cd->nphase; i++) {
      cd->plist[i] = NULL;
      cd->llist[i] = NULL;
    }
  }
  cd->skirt = NULL;
}

// TODO(vatai): move to mpi2_comm_data.c
static void fill_part(int phase, comm_data_t *cd) {
  char fname[1024];
  FILE *f;
  sprintf(fname, "%s/g%d.part.%d", cd->dir, phase, cd->npart);
  printf("  reading %s...\n", fname);

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

// TODO(vatai): move to mpi2_comm_data.c
static void fill_level(int phase, comm_data_t *cd) {
  char fname[1024];
  FILE *f;
  sprintf(fname, "%s/l%d", cd->dir, phase);
  printf("  reading %s...\n", fname);

  f = fopen(fname, "r");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }

  level_t *lg = new_level(cd->plist[phase]);
  read_level(f, lg); // fclose(f); ... in read_level
  cd->llist[phase] = lg;
}

// TODO(vatai): move to mpi2_comm_data.c
static void fill_skirt(comm_data_t *cd) {
  char fname[1024];
  FILE *f;
  sprintf(fname, "%s/s%d", cd->dir, cd->nphase);
  printf("  reading %s...\n", fname);

  f = fopen(fname, "r");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }

  skirt_t *sk = new_skirt(cd->plist[0]);
  read_skirt(f, sk, cd->nlevel); // fclose(f); in read_skirt
  cd->skirt = sk;
}

// TODO(vatai): move to mpi2_comm_data.c
comm_data_t *new_comm_data(mpk_t *mg, char *dir) {
  comm_data_t *cd = malloc(sizeof(*cd));
  cd->dir = dir;
  cd->mg = mg;
  MPI_Comm_rank(MPI_COMM_WORLD, &cd->rank);
  read_dir(cd);
  read_matrix(cd);
  alloc_mpk_data(cd);
  for (int phase = 0; phase < cd->nphase; phase++) {
    fill_part(phase, cd);
    fill_level(phase, cd);
  }
  if (cd->nphase == 0)
    fill_part(0, cd);
  fill_skirt(cd);

  int npart = cd->npart;
  int nphase = cd->nphase;

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

  cd->mptr = malloc(sizeof(*cd->mptr) * (nphase + 1));
  assert(cd->mptr != NULL);
  cd->mcol = malloc(sizeof(*cd->mcol) * (nphase + 1));
  assert(cd->mcol != NULL);

  return cd;
}

// TODO(vatai): move to mpi2_comm_data.c
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
  free(cd->idx_sbuf);
  free(cd->vv_buf);
  free(cd->vv_sbuf);

  for (int phase = 0; phase <= cd->nphase; phase++) {
    free(cd->mptr[phase]);
    free(cd->mcol[phase]);
    // free(cd->mval[phase]); // TODO(vatai): when allocate
  }
  free(cd->mptr);
  free(cd->mcol);
  // free(cd->mval);

  del_crs(cd->graph);
  for (int phase = 0; phase < cd->nphase; phase++) {
    del_part(cd->plist[phase]);
    del_level(cd->llist[phase]);
  }
  if (cd->nphase == 0) {
    del_part(cd->plist[0]);
  }
  del_skirt(cd->skirt);
  free(cd);
}

// TODO(vatai): group with comm_table
static char *new_comm_table(mpk_t *mg) {
  // Communication of which vertex (n) from which level (nlevel)
  // from which process/partition (npart) to which process/partition
  // (npart).
  int num_ct_elem = mg->nlevel * mg->n * mg->npart * mg->npart;
  char *ct = malloc(sizeof(*ct) * num_ct_elem);
  assert(ct != NULL);
  return ct;
}

// TODO(vatai): group with prep?
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

// TODO(vatai): group with comm_table
static int get_ct_idx(mpk_t *mg, int src_part, int tgt_part, int vv_idx) {
  int from_part_to_part = mg->npart * src_part + tgt_part;
  return mg->nlevel * mg->n * from_part_to_part + vv_idx;
}

// TODO(vatai): group with comm_table
static void init_comm_table(mpk_t *mg, char *comm_table) {
  for (int i = 0; i < mg->n; i++) {
    int part = mg->plist[0]->part[i];
    int idx = get_ct_idx(mg, part, part, i);
    comm_table[idx] = 1;
  }
}

// TODO(vatai): group with comm_table
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

// TODO(vatai): group with comm_table
static void clear_comm_table(mpk_t *mg, char *comm_table) {
  // Communication of which vertex (n) from which level (nlevel)
  // from which process/partition (npart) to which process/partition
  // (npart).
  int N = mg->nlevel * mg->n * mg->npart * mg->npart;
  for (int i = 0; i < N; i++)
    comm_table[i] = 0;
}

// TODO(vatai): group with comm_table
static int min_or_0(mpk_t *mg, int phm1) {
  if (mg->nphase == 0 || phm1 == -1)
    return 0;
  int *ll = mg->llist[phm1]->level;
  int rv = ll[0];
  for (int i = 0; i < mg->n; i++)
    if (ll[i] < rv)
      rv = ll[i];
  return rv;
}

// TODO(vatai): group with comm_table
static int max_or_nlevel(comm_data_t *cd, int phase) {
  if (cd->nphase == phase)
    return cd->nlevel;
  int *ll = cd->mg->llist[phase]->level;
  int rv = ll[0];
  for (int i = 1; i < cd->n; i++)
    if (ll[i] > rv)
      rv = ll[i];
  if (rv > cd->nlevel)
    return cd->nlevel;
  return rv;
}

// TODO(vatai): group with comm_table
static void phase_comm_table(int phase, comm_data_t *cd, char *comm_table,
                             int *store_part) {
  assert(cd->mg->plist[phase] != NULL);
  int n = cd->n;
  int *ll = cd->mg->llist[phase]->level;
  int lmax = max_or_nlevel(cd, phase);
  int prevlmin = min_or_0(cd->mg, phase - 1);
  for (int level = prevlmin + 1; level <= lmax; level++) { // initially prevlmin =0
    for (int i = 0; i < n; i++) {
      int i_vvidx = n * level + i;
      int prevli = phase ? cd->mg->llist[phase - 1]->level[i] : 0;
      if (prevli < level && level <= ll[i] && (cd->idx_buf != NULL || store_part[i_vvidx] == -1)) {
        int curpart = cd->mg->plist[phase]->part[i];
        fill_comm_table_one_vertex(curpart, i, level, cd->mg, comm_table, store_part);
        if (cd->idx_buf == NULL)
          store_part[i_vvidx] = curpart;
      }
    }
  }
}

// TODO(vatai): group with comm_table
static void skirt_comm_table(comm_data_t *cd, char *comm_table,
                             int *store_part) {
  int n = cd->n;
  int nlevel = cd->nlevel;
  int nphase = cd->nphase;
  int *sl = cd->mg->sg->levels;
  int prevlmin = min_or_0(cd->mg, nphase - 1);
  for (int p = 0; p < cd->npart; p++) {
    for (int level = prevlmin + 1; level <= nlevel; level++) {
      for (int i = 0; i < n; i++) {
        int above_prevl =
            (nphase ? cd->mg->llist[nphase - 1]->level[i] : 0) < level;
        int skirt_active = sl[p * n + i] >= 0;
        int not_over_max = level <= nlevel - sl[p * n + i];
        if (above_prevl && skirt_active && not_over_max) {
          fill_comm_table_one_vertex(p, i, level, cd->mg, comm_table, store_part);
          store_part[n * level + i] = p;
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
  cd->scount[phase] = 0;
  cd->rcount[phase] = 0;
  for (int p = 0; p < cd->npart; p++) {
    cd->sendcounts[phase * cd->npart + p] = 0;
    cd->recvcounts[phase * cd->npart + p] = 0;
  }
  for (int p = 0; p < cd->npart; p++) {
    for (int i = 0; i < cd->n * cd->nlevel; i++) {
      int idx = get_ct_idx(cd->mg, cd->rank, p, i);
      if (comm_table[idx]) {
        cd->scount[phase]++;
        cd->sendcounts[phase * cd->npart + p]++;
      }
      idx = get_ct_idx(cd->mg, p, cd->rank, i);
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

static void fill_idx_rsbuf(int phase, char *comm_table, comm_data_t *cd) {
  // Needs idx buffers allocated.
  int scounter = 0;
  int rcounter = 0;
  for (int p = 0; p < cd->npart; p++) {
    // For all vv indices.
    for (int i = 0; i < cd->nlevel * cd->n; ++i) {
      // Here p is the destination (to) partition.
      int sidx = get_ct_idx(cd->mg, cd->rank, p, i);
      if (comm_table[sidx]) {
        cd->idx_sbufs[phase][scounter] = i;
        scounter++;
      }
      int ridx = get_ct_idx(cd->mg, p, cd->rank, i);
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

// Fill all buffer size variables to make allocation possible.
static int get_prevlmin(int phase, comm_data_t *cd) {
  int prevlmin = 0;
  if (phase > 0) {
    int *prevl = cd->mg->llist[phase - 1]->level;
    int prevlmin = prevl[0];
    for (int i = 1; i < cd->n; i++)
      if (prevl[i] < prevlmin)
        prevlmin = prevl[i];
  }
  return prevlmin;
}

static int phase_cond(int phase, int i, int level, comm_data_t *cd) {
  int prevli = phase ? cd->mg->llist[phase - 1]->level[i] : 0;
  int curpart = phase == cd->nphase ? cd->mg->plist[0]->part[i]
                                    : cd->mg->plist[phase]->part[i];
  int curli = phase == cd->nphase ? cd->mg->sg->levels[cd->rank * cd->n + i]
                                  : cd->mg->llist[phase]->level[i];
  return prevli < level && level <= curli && cd->rank == curpart;
}

static int skirt_cond(int phase, int i, int level, comm_data_t *cd) {
  int prevli = cd->nphase ? cd->mg->llist[cd->nphase - 1]->level[i] : 0;
  int slpi = cd->mg->sg->levels[cd->rank * cd->n + i];
  return prevli < level && 0 <= slpi && level <= cd->nlevel - slpi;
}

// This is central iteration loop.
//
// The cond() function returns true or false depending if the given
// vertex (at the given level) needs to be counted/processed.  There
// are two condition functions (see above), phase_cond and skirt_cond.
//
// Another "implicit parameter" which changes the behaviour is the
// value of cd->idx_buf.  If cd->idx_buf == NULL, the idx_mbuf[] is
// not filled, while if non-NULL it is filled.
static void iterator(int cond(int, int, int, comm_data_t *cd),
                     int phase, comm_data_t *cd, char *comm_table,
                     int *store_part) {
  int prevlmin = get_prevlmin(phase, cd);
  cd->mcount[phase] = 0;
  int max = max_or_nlevel(cd, phase);
  for (int level = prevlmin + 1; level <= max; level++) {
    for (int i = 0; i < cd->n; i++) {
      if (cond(phase, i, level, cd)) {
        if (cd->idx_buf != NULL) {
          cd->idx_mbufs[phase][cd->mcount[phase]] = level * cd->n + i;
        }
        cd->mcount[phase]++;
      }
    }
  }
}

static void fill_bufsize_rscount_displs(comm_data_t *cd, char *comm_table,
                                        int *store_part) {
  cd->idx_buf = NULL;
  clear_comm_table(cd->mg, comm_table);
  init_comm_table(cd->mg, comm_table);
  for (int phase = 0; phase < cd->nphase; phase++) {
    phase_comm_table(phase, cd, comm_table, store_part);
    iterator(phase_cond, phase, cd, comm_table, store_part);
    fill_rscounts(phase, cd, comm_table);
    fill_displs(phase, cd);
    clear_comm_table(cd->mg, comm_table);
  }
  skirt_comm_table(cd, comm_table, store_part);
  iterator(skirt_cond, cd->nphase, cd, comm_table, store_part);
  fill_rscounts(cd->nphase, cd, comm_table);
  fill_displs(cd->nphase, cd);
}

static void alloc_bufs(comm_data_t *cd) {
  cd->buf_count = 0;
  cd->buf_scount = 0;
  for (int phase = 0; phase <= cd->nphase; phase++) {
    cd->buf_count += cd->rcount[phase];
    cd->buf_count += cd->mcount[phase];
    cd->buf_scount += cd->scount[phase];
  }

  cd->idx_buf = malloc(sizeof(*cd->idx_buf) * cd->buf_count);
  cd->idx_sbuf = malloc(sizeof(*cd->idx_sbuf) * cd->buf_scount);
  cd->vv_buf = malloc(sizeof(*cd->vv_buf) * cd->buf_count);
  cd->vv_sbuf = malloc(sizeof(*cd->vv_sbuf) * cd->buf_scount);
  assert(cd->idx_buf != NULL);
  assert(cd->idx_sbuf != NULL);
  assert(cd->vv_buf != NULL);
  assert(cd->vv_sbuf != NULL);

  long count = 0;
  long scount = 0;
  for (int phase = 0; phase <= cd->nphase; phase++) {
    cd->idx_rbufs[phase] = cd->idx_buf + count;
    cd->vv_rbufs[phase] = cd->vv_buf + count;
    count += cd->rcount[phase];
    cd->idx_mbufs[phase] = cd->idx_buf + count;
    cd->vv_mbufs[phase] = cd->vv_buf + count;
    count += cd->mcount[phase];

    cd->idx_sbufs[phase] = cd->idx_sbuf + scount;
    cd->vv_sbufs[phase] = cd->vv_sbuf + scount;
    scount += cd->scount[phase];
  }
}

static void fill_mptr(comm_data_t *cd, int phase){
  // TODO(vatai): Put all mptr[phase] into a single array.
  int *ptr = cd->mg->g0->ptr;
  task_t *tl = cd->mg->tlist + phase * cd->npart + cd->rank;
  assert(tl->n == cd->mcount[phase]);
  long *mptr = malloc(sizeof(*mptr) * (tl->n + 1));
  assert(mptr != NULL);
  mptr[0] = 0;
  for (int mi = 0; mi < tl->n; mi++) {
    assert(tl->idx[mi] == cd->idx_mbufs[phase][mi]);
    int i = tl->idx[mi] % cd->n;
    mptr[mi + 1] = mptr[mi] + ptr[i + 1] - ptr[i];
  }
  cd->mptr[phase] = mptr;
}

static void fill_mcol(comm_data_t *cd, int phase) {
  int *ptr = cd->mg->g0->ptr;
  int *col = cd->mg->g0->col;
  task_t *tl = cd->mg->tlist + phase * cd->npart + cd->rank;

  long *mptr = cd->mptr[phase];
  int *rdisp = cd->rdispls + phase * cd->npart;
  int *rcount = cd->recvcounts + phase * cd->npart;

  // alloc and store indices which will be searched

  // alloc size will be sum =0; for() sum += tl->n + phase_rbuf

  assert(tl->n == cd->mcount[phase]);
  long *mcol = malloc(sizeof(*mcol) * mptr[cd->mcount[phase]]);
  assert(mcol != NULL);
  for (int mi = 0; mi < cd->mcount[phase]; mi++) {
    assert(cd->idx_mbufs[phase][mi] == tl->idx[mi]);
    long idx = cd->idx_mbufs[phase][mi];
    long i = idx % cd->n;
    long level = idx / cd->n;

    assert(mptr[mi + 1] - mptr[mi] == ptr[i + 1] - ptr[i]);
    for (int j = ptr[i]; j < ptr[i + 1]; j++) {
      long target = col[j] + cd->n * (level - 1);
      int idx = find_idx(cd->idx_buf, cd->buf_count, target);
      mcol[j - ptr[i] + mptr[mi]] = idx;
    }
  }
  cd->mcol[phase] = mcol;
}

static void fill_bufs(comm_data_t *cd, char *comm_table, int *store_part) {
  clear_comm_table(cd->mg, comm_table);
  init_comm_table(cd->mg, comm_table);
  for (int phase = 0; phase < cd->nphase; phase++) {
    phase_comm_table(phase, cd, comm_table, store_part);
    iterator(phase_cond, phase, cd, comm_table, store_part);
    fill_idx_rsbuf(phase, comm_table, cd);
    clear_comm_table(cd->mg, comm_table);
    fill_mptr(cd, phase);
    fill_mcol(cd, phase);
  }
  skirt_comm_table(cd, comm_table, store_part);
  iterator(skirt_cond, cd->nphase, cd, comm_table, store_part);
  fill_idx_rsbuf(cd->nphase, comm_table, cd);
  fill_mptr(cd, cd->nphase);
  fill_mcol(cd, cd->nphase);

  // Store the idx_buf indices, because idx_sbufs are used for copying
  // data from idx_buf to sbuf.
  for (int i = 0; i < cd->buf_scount; i++) {
    int vv_idx = cd->idx_sbuf[i];
    int buf_idx = find_idx(cd->idx_buf, cd->buf_count, vv_idx);
    cd->idx_sbuf[i] = buf_idx;
  }
}

/*
 * Allocate and fill `comm_data_t cd`.
 */
void mpi_prep_mpk(comm_data_t *cd) {
  assert(cd != NULL);
  printf("preparing mpi buffers for communication...");

  char *comm_table = new_comm_table(cd->mg);
  int *store_part = new_store_part(cd->mg);

  // Fill stage one data
  fill_bufsize_rscount_displs(cd, comm_table, store_part);

  // Alloc of stage two to memory
  alloc_bufs(cd);

  // Fill stage two data
  fill_bufs(cd, comm_table, store_part);

  free(comm_table);
  free(store_part);
  printf(" done\n");
}
