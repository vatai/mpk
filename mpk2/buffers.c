#include <assert.h>
#include <stdlib.h>

#include "buffers.h"

static int *new_store_part(comm_data_t *cd) {
  // Initialise store_part[i] to be the id of the 0th partition for
  // level 0, and -1 for all other levels
  int n = cd->n;
  int *pl = cd->plist[0]->part;
  int sp_num_elem = n * (cd->nlevel + 1);
  int *store_part = (int *)malloc(sizeof(*store_part) * sp_num_elem);
  assert(store_part != NULL);
  for (int i = 0; i < n * (cd->nlevel + 1); i++)
    store_part[i] = i < n ? pl[i] : -1;
  return store_part;
}

static char *new_comm_table(int n, int npart, int nlevel) {
  // Communication of which vertex (n) from which level (nlevel)
  // from which process/partition (npart) to which process/partition
  // (npart).
  int num_elem = nlevel * n * npart * npart;
  char *ct = malloc(sizeof(*ct) * num_elem);
  assert(ct != NULL);
  return ct;
}

static void clear_comm_table(comm_data_t *cd, char *comm_table) {
  // Communication of which vertex (n) from which level (nlevel)
  // from which process/partition (npart) to which process/partition
  // (npart).
  int N = cd->nlevel * cd->n * cd->npart * cd->npart;
  for (int i = 0; i < N; i++)
    comm_table[i] = 0;
}

static int get_ct_idx(int src_part, int tgt_part, int vv_idx, int n, int npart,
                      int nlevel) {
  int from_part_to_part = npart * src_part + tgt_part;
  return nlevel * n * from_part_to_part + vv_idx;
}

static void init_comm_table(comm_data_t *cd, char *comm_table) {
  for (int i = 0; i < cd->n; i++) {
    int part = cd->plist[0]->part[i];
    int idx = get_ct_idx(part, part, i, cd->n, cd->npart, cd->nlevel);
    comm_table[idx] = 1;
  }
}

static void proc_vertex(int curpart, int i, int level, comm_data_t *cd,
                        char *comm_table, int *store_part) {
  crs0_t *g0 = cd->graph;
  assert(g0 != NULL);
  for (int j = g0->ptr[i]; j < g0->ptr[i + 1]; j++) { // All neighbours
    int k_vvidx = cd->n * (level - 1) + g0->col[j];   // source vv index
    assert(store_part[k_vvidx] != -1);
    if (store_part[k_vvidx] != curpart) {
      int idx = get_ct_idx(store_part[k_vvidx], curpart, k_vvidx, cd->n,
                           cd->npart, cd->nlevel);
      comm_table[idx] = 1;
    }
  }
}

static int min_or_0(comm_data_t *cd, int phm1) {
  if (cd->nphase == 0 || phm1 == -1)
    return 0;
  int *ll = cd->llist[phm1]->level;
  int rv = ll[0];
  for (int i = 0; i < cd->n; i++)
    if (ll[i] < rv)
      rv = ll[i];
  return rv;
}

static int max_or_nlevel(comm_data_t *cd, int phase) {
  if (cd->nphase == phase)
    return cd->nlevel;
  int *ll = cd->llist[phase]->level;
  int rv = ll[0];
  for (int i = 1; i < cd->n; i++)
    if (ll[i] > rv)
      rv = ll[i];
  if (rv > cd->nlevel)
    return cd->nlevel;
  return rv;
}

static void phase_comm_table(int phase, comm_data_t *cd, buffers_t *bufs,
                             char *comm_table, int *store_part) {
  // TODO(vatai): first_run
  assert(cd->plist[phase] != NULL);
  int n = cd->n;
  int *ll = cd->llist[phase]->level;
  int lmax = max_or_nlevel(cd, phase);
  int prevlmin = min_or_0(cd, phase - 1);
  for (int level = prevlmin + 1; level <= lmax;
       level++) { // initially prevlmin =0
    for (int i = 0; i < n; i++) {
      int i_vvidx = n * level + i;
      int prevli = phase ? cd->llist[phase - 1]->level[i] : 0;
      if (prevli < level && level <= ll[i] &&
          (bufs->idx_buf != NULL || store_part[i_vvidx] == -1)) {
        int curpart = cd->plist[phase]->part[i];
        proc_vertex(curpart, i, level, cd, comm_table, store_part);
        if (bufs->idx_buf == NULL)
          store_part[i_vvidx] = curpart;
      }
    }
  }
}

static void skirt_comm_table(comm_data_t *cd, char *comm_table,
                             int *store_part) {
  int n = cd->n;
  int nlevel = cd->nlevel;
  int nphase = cd->nphase;
  int *sl = cd->skirt->levels;
  int prevlmin = min_or_0(cd, nphase - 1);
  for (int p = 0; p < cd->npart; p++) {
    for (int level = prevlmin + 1; level <= nlevel; level++) {
      for (int i = 0; i < n; i++) {
        int above_prevl =
            (nphase ? cd->llist[nphase - 1]->level[i] : 0) < level;
        int skirt_active = sl[p * n + i] >= 0;
        int not_over_max = level <= nlevel - sl[p * n + i];
        if (above_prevl && skirt_active && not_over_max) {
          proc_vertex(p, i, level, cd, comm_table, store_part);
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
static void fill_rscounts(int phase, buffers_t *bufs, char *comm_table) {
  int idx;
  bufs->scount[phase] = 0;
  bufs->rcount[phase] = 0;
  for (int p = 0; p < bufs->npart; p++) {
    bufs->sendcounts[phase * bufs->npart + p] = 0;
    bufs->recvcounts[phase * bufs->npart + p] = 0;
  }
  for (int p = 0; p < bufs->npart; p++) {
    for (int i = 0; i < bufs->n * bufs->nlevel; i++) {
      idx = get_ct_idx(bufs->rank, p, i, bufs->n, bufs->npart, bufs->nlevel);
      if (comm_table[idx]) {
        bufs->scount[phase]++;
        bufs->sendcounts[phase * bufs->npart + p]++;
      }
      idx = get_ct_idx(p, bufs->rank, i, bufs->n, bufs->npart, bufs->nlevel);
      if (comm_table[idx]) {
        bufs->rcount[phase]++;
        bufs->recvcounts[phase * bufs->npart + p]++;
      }
    }
  }
}

static void fill_displs(int phase, buffers_t *bufs) {
  // Needs sendcounts and recvcounts filled for phase.
  for (int p = 0; p < bufs->npart; p++) {
    int idx = phase * bufs->npart + p;
    if (p == 0) {
      bufs->sdispls[idx] = 0;
      bufs->rdispls[idx] = 0;
    } else {
      bufs->sdispls[idx] = bufs->sdispls[idx - 1] + bufs->sendcounts[idx - 1];
      bufs->rdispls[idx] = bufs->rdispls[idx - 1] + bufs->recvcounts[idx - 1];
    }
  }
}

static void fill_idx_rsbuf(int phase, char *comm_table, buffers_t *bufs) {
  // Needs idx buffers allocated.
  int idx;
  int scounter = 0;
  int rcounter = 0;
  for (int p = 0; p < bufs->npart; p++) {
    // For all vv indices.
    for (int i = 0; i < bufs->nlevel * bufs->n; ++i) {
      // Here p is the destination (to) partition.
      idx = get_ct_idx(bufs->rank, p, i, bufs->n, bufs->npart, bufs->nlevel);
      if (comm_table[idx]) {
        bufs->idx_sbufs[phase][scounter] = i;
        scounter++;
      }
      idx = get_ct_idx(p, bufs->rank, i, bufs->n, bufs->npart, bufs->nlevel);
      if (comm_table[idx]) {
        bufs->idx_rbufs[phase][rcounter] = i;
        rcounter++;
      }
    }
  }
}

// Fill all buffer size variables to make allocation possible.
static int get_prevlmin(int phase, comm_data_t *cd) {
  int prevlmin = 0;
  if (phase > 0) {
    int *prevl = cd->llist[phase - 1]->level;
    int prevlmin = prevl[0];
    for (int i = 1; i < cd->n; i++)
      if (prevl[i] < prevlmin)
        prevlmin = prevl[i];
  }
  return prevlmin;
}

static int phase_cond(int phase, int i, int level, comm_data_t *cd) {
  int prevli = phase ? cd->llist[phase - 1]->level[i] : 0;
  int curpart =
      phase == cd->nphase ? cd->plist[0]->part[i] : cd->plist[phase]->part[i];
  int curli = phase == cd->nphase ? cd->skirt->levels[cd->rank * cd->n + i]
                                  : cd->llist[phase]->level[i];
  return prevli < level && level <= curli && cd->rank == curpart;
}

static int skirt_cond(int phase, int i, int level, comm_data_t *cd) {
  int prevli = cd->nphase ? cd->llist[cd->nphase - 1]->level[i] : 0;
  int slpi = cd->skirt->levels[cd->rank * cd->n + i];
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
static void iterator(int cond(int, int, int, comm_data_t *cd), int phase,
                     comm_data_t *cd, buffers_t *bufs, char *comm_table,
                     int *store_part) {
  int prevlmin = get_prevlmin(phase, cd);
  bufs->mcount[phase] = 0;
  int max = max_or_nlevel(cd, phase);
  for (int level = prevlmin + 1; level <= max; level++) {
    for (int i = 0; i < cd->n; i++) {
      if (cond(phase, i, level, cd)) {
        if (bufs->idx_buf != NULL) {
          bufs->idx_mbufs[phase][bufs->mcount[phase]] = level * cd->n + i;
        }
        bufs->mcount[phase]++;
      }
    }
  }
}

static void alloc_bufs(buffers_t *bufs) {
  bufs->buf_count = 0;
  bufs->buf_scount = 0;
  for (int phase = 0; phase <= bufs->nphase; phase++) {
    bufs->buf_count += bufs->rcount[phase];
    bufs->buf_count += bufs->mcount[phase];
    bufs->buf_scount += bufs->scount[phase];
  }

  bufs->idx_buf = malloc(sizeof(*bufs->idx_buf) * bufs->buf_count);
  bufs->idx_sbuf = malloc(sizeof(*bufs->idx_sbuf) * bufs->buf_scount);
  bufs->vv_buf = malloc(sizeof(*bufs->vv_buf) * bufs->buf_count);
  bufs->vv_sbuf = malloc(sizeof(*bufs->vv_sbuf) * bufs->buf_scount);
  assert(bufs->idx_buf != NULL);
  assert(bufs->idx_sbuf != NULL);
  assert(bufs->vv_buf != NULL);
  assert(bufs->vv_sbuf != NULL);

  long count = 0;
  long scount = 0;
  for (int phase = 0; phase <= bufs->nphase; phase++) {
    bufs->idx_rbufs[phase] = bufs->idx_buf + count;
    bufs->vv_rbufs[phase] = bufs->vv_buf + count;
    count += bufs->rcount[phase];
    bufs->idx_mbufs[phase] = bufs->idx_buf + count;
    bufs->vv_mbufs[phase] = bufs->vv_buf + count;
    count += bufs->mcount[phase];

    bufs->idx_sbufs[phase] = bufs->idx_sbuf + scount;
    bufs->vv_sbufs[phase] = bufs->vv_sbuf + scount;
    scount += bufs->scount[phase];
  }
}

static void fill_mptr(comm_data_t *cd, buffers_t *bufs, int phase) {
  // TODO(vatai): Put all mptr[phase] into a single array.
  int *ptr = cd->graph->ptr;
  int mcount = bufs->mcount[phase];
  long *mptr = malloc(sizeof(*mptr) * (mcount + 1));
  long *idx_mbuf = bufs->idx_mbufs[phase];
  assert(mptr != NULL);
  mptr[0] = 0;
  for (int mi = 0; mi < mcount; mi++) {
    assert(idx_mbuf[mi] == bufs->idx_mbufs[phase][mi]);
    int i = idx_mbuf[mi] % cd->n;
    mptr[mi + 1] = mptr[mi] + ptr[i + 1] - ptr[i];
  }
  bufs->mptr[phase] = mptr;
}

static int find_idx(long *ptr, int size, long target) {
  for (int i = 0; i < size; i++)
    if (ptr[i] == target)
      return i;
  return -1;
}

static void fill_mcol(comm_data_t *cd, buffers_t *bufs, int phase) {
  int *ptr = cd->graph->ptr;
  int *col = cd->graph->col;
  long *mptr = bufs->mptr[phase];
  int mcount = bufs->mcount[phase];

  long *mcol = malloc(sizeof(*mcol) * mptr[mcount]);
  assert(mcol != NULL);
  for (int mi = 0; mi < bufs->mcount[phase]; mi++) {
    long idx = bufs->idx_mbufs[phase][mi];
    long i = idx % cd->n;
    long level = idx / cd->n;

    assert(mptr[mi + 1] - mptr[mi] == ptr[i + 1] - ptr[i]);
    for (int j = ptr[i]; j < ptr[i + 1]; j++) {
      long target = col[j] + cd->n * (level - 1);
      int idx = find_idx(bufs->idx_buf, bufs->buf_count, target);
      mcol[j - ptr[i] + mptr[mi]] = idx;
    }
  }
  bufs->mcol[phase] = mcol;
}

static void fill_bufs(comm_data_t *cd, buffers_t *bufs, char *comm_table,
                      int *store_part) {
  clear_comm_table(cd, comm_table);
  init_comm_table(cd, comm_table);
  for (int phase = 0; phase < cd->nphase; phase++) {
    phase_comm_table(phase, cd, bufs, comm_table, store_part);
    iterator(phase_cond, phase, cd, bufs, comm_table, store_part);
    fill_idx_rsbuf(phase, comm_table, bufs);
    clear_comm_table(cd, comm_table);
    fill_mptr(cd, bufs, phase);
    fill_mcol(cd, bufs, phase);
  }
  skirt_comm_table(cd, comm_table, store_part);
  iterator(skirt_cond, cd->nphase, cd, bufs, comm_table, store_part);
  fill_idx_rsbuf(cd->nphase, comm_table, bufs);
  fill_mptr(cd, bufs, cd->nphase);
  fill_mcol(cd, bufs, cd->nphase);

  // Store the idx_buf indices, because idx_sbufs are used for copying
  // data from idx_buf to sbuf.
  for (int i = 0; i < bufs->buf_scount; i++) {
    int vv_idx = bufs->idx_sbuf[i];
    int buf_idx = find_idx(bufs->idx_buf, bufs->buf_count, vv_idx);
    bufs->idx_sbuf[i] = buf_idx;
  }
}

static void fill_bufsize_rscount_displs(comm_data_t *cd, buffers_t *bufs,
                                        char *comm_table, int *store_part) {
  bufs->idx_buf = NULL;
  clear_comm_table(cd, comm_table);
  init_comm_table(cd, comm_table);
  for (int phase = 0; phase < cd->nphase; phase++) {
    phase_comm_table(phase, cd, bufs, comm_table, store_part);
    iterator(phase_cond, phase, cd, bufs, comm_table, store_part);
    fill_rscounts(phase, bufs, comm_table);
    fill_displs(phase, bufs);
    clear_comm_table(cd, comm_table);
  }
  skirt_comm_table(cd, comm_table, store_part);
  iterator(skirt_cond, cd->nphase, cd, bufs, comm_table, store_part);
  fill_rscounts(cd->nphase, bufs, comm_table);
  fill_displs(cd->nphase, bufs);
}

buffers_t *new_bufs(comm_data_t *cd) {
  buffers_t *bufs = malloc(sizeof(*bufs));
  bufs->n = cd->n;
  bufs->rank = cd->rank;
  bufs->nlevel = cd->nlevel;
  int npart = bufs->npart = cd->npart;
  int nphase = bufs->nphase = cd->nphase;

  bufs->recvcounts = malloc(sizeof(*bufs->recvcounts) * npart * (nphase + 1));
  assert(bufs->recvcounts != NULL);
  bufs->sendcounts = malloc(sizeof(*bufs->sendcounts) * npart * (nphase + 1));
  assert(bufs->sendcounts != NULL);

  bufs->rdispls = malloc(sizeof(*bufs->rdispls) * npart * (nphase + 1));
  assert(bufs->rdispls != NULL);
  bufs->sdispls = malloc(sizeof(*bufs->sdispls) * npart * (nphase + 1));
  assert(bufs->sdispls != NULL);

  bufs->rcount = malloc(sizeof(*bufs->rcount) * (nphase + 1));
  assert(bufs->rcount != NULL);
  bufs->mcount = malloc(sizeof(*bufs->mcount) * (nphase + 1));
  assert(bufs->mcount != NULL);
  bufs->scount = malloc(sizeof(*bufs->scount) * (nphase + 1));
  assert(bufs->scount != NULL);

  bufs->idx_rbufs = malloc(sizeof(*bufs->idx_rbufs) * (nphase + 1));
  assert(bufs->idx_rbufs != NULL);
  bufs->idx_mbufs = malloc(sizeof(*bufs->idx_mbufs) * (nphase + 1));
  assert(bufs->idx_mbufs != NULL);
  bufs->idx_sbufs = malloc(sizeof(*bufs->idx_sbufs) * (nphase + 1));
  assert(bufs->idx_sbufs != NULL);

  bufs->vv_rbufs = malloc(sizeof(*bufs->vv_rbufs) * (nphase + 1));
  assert(bufs->vv_rbufs != NULL);
  bufs->vv_mbufs = malloc(sizeof(*bufs->vv_mbufs) * (nphase + 1));
  assert(bufs->vv_mbufs != NULL);
  bufs->vv_sbufs = malloc(sizeof(*bufs->vv_sbufs) * (nphase + 1));
  assert(bufs->vv_sbufs != NULL);

  bufs->mptr = malloc(sizeof(*bufs->mptr) * (nphase + 1));
  assert(bufs->mptr != NULL);
  bufs->mcol = malloc(sizeof(*bufs->mcol) * (nphase + 1));
  assert(bufs->mcol != NULL);

  return bufs;
}

void del_bufs(buffers_t *bufs) {
  free(bufs->recvcounts);
  free(bufs->sendcounts);
  free(bufs->rdispls);
  free(bufs->sdispls);

  free(bufs->rcount);
  free(bufs->mcount);
  free(bufs->scount);

  free(bufs->idx_rbufs);
  free(bufs->idx_mbufs);
  free(bufs->idx_sbufs);
  free(bufs->vv_rbufs);
  free(bufs->vv_mbufs);
  free(bufs->vv_sbufs);

  free(bufs->idx_buf);
  free(bufs->idx_sbuf);
  free(bufs->vv_buf);
  free(bufs->vv_sbuf);

  for (int phase = 0; phase <= bufs->nphase; phase++) {
    free(bufs->mptr[phase]);
    free(bufs->mcol[phase]);
    // free(cd->mval[phase]); // TODO(vatai): when allocate
  }
  free(bufs->mptr);
  free(bufs->mcol);
  // free(cd->mval);
  free(bufs);
}

void fill_buffers(comm_data_t *cd, buffers_t *bufs) {
  assert(cd != NULL);
  printf("preparing mpi buffers for communication...");

  char *comm_table = new_comm_table(cd->n, cd->npart, cd->nlevel);
  int *store_part = new_store_part(cd);

  // Fill stage one data
  fill_bufsize_rscount_displs(cd, bufs, comm_table, store_part);

  // Alloc of stage two to memory
  alloc_bufs(bufs);

  // Fill stage two data
  fill_bufs(cd, bufs, comm_table, store_part);

  free(comm_table);
  free(store_part);
  printf(" done\n");
}
