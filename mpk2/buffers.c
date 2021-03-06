#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "buffers.h"
#include "reduce_comm.h"

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
  char *ct = (char *)malloc(sizeof(*ct) * num_elem);
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

static int get_ct_idx(int src, int tgt, int idx, int n, int npart, int nlevel) {
  int from_part_to_part = npart * src + tgt;
  return nlevel * n * from_part_to_part + idx;
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

static void collect_comm(
    int phase,
    comm_data_t *cd,
    int *store_part,
    char *comm_table)
{
  assert(cd->plist[phase] != NULL);
  const int size = cd->npart * cd->npart;
  clear_comm_table(cd, comm_table);
  int n = cd->n;
  int *ll = cd->llist[phase]->level;
  int lmax = max_or_nlevel(cd, phase);
  int prevlmin = min_or_0(cd, phase - 1);
  for (int level = prevlmin + 1; level <= lmax; level++) {
    for (int i = 0; i < n; i++) {
      int i_vvidx = n * level + i;
      int prevli = phase ? cd->llist[phase - 1]->level[i] : 0;
      if (prevli < level && level <= ll[i]) {
        const int tgt_part = cd->plist[phase]->part[i];
        for (int j = cd->graph->ptr[i]; j < cd->graph->ptr[i + 1]; j++) {
          const int k_vvidx = cd->n * (level - 1) + cd->graph->col[j];
          const int src_part = store_part[k_vvidx];
          if (src_part != -1) {
            int idx = get_ct_idx(src_part, tgt_part, k_vvidx, cd->n,
                                 cd->npart, cd->nlevel);
            comm_table[idx] = 1;
          }
        }
      }
    }
  }
}

static void phase_comm_table(
    int phase,
    comm_data_t *cd,
    char *comm_table,
    int *store_part,
    char *cursp)
{
  // TODO(vatai): first_run
  assert(cd->plist[phase] != NULL);
  int n = cd->n;
  int *ll = cd->llist[phase]->level;
  int lmax = max_or_nlevel(cd, phase);
  int prevlmin = min_or_0(cd, phase - 1);
  for (int level = prevlmin + 1; level <= lmax; level++) {
    for (int i = 0; i < n; i++) {
      int i_vvidx = n * level + i;
      int prevli = phase ? cd->llist[phase - 1]->level[i] : 0;
      if (prevli < level && level <= ll[i] &&
          (cursp == NULL || store_part[i_vvidx] == -1)) {
        int curpart = cd->plist[phase]->part[i];
        proc_vertex(curpart, i, level, cd, comm_table, store_part);
        if (cursp != NULL) {
          store_part[i_vvidx] = curpart;
          cursp[i_vvidx] = 1;
        }
      }
    }
  }
}

static void skirt_comm_table(
    comm_data_t *cd,
    char *comm_table,
    int *store_part)
{
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
static void iterator(
    int cond(int, int, int, comm_data_t *cd),
    int phase,
    comm_data_t *cd,
    buffers_t *bufs)
{
  int prevlmin = get_prevlmin(phase, cd);
  bufs->mcount[phase] = 0;
  int max = max_or_nlevel(cd, phase);
  long *idx_mbuf = bufs->idx_buf + bufs->mbuf_offsets[phase];
  for (int level = prevlmin + 1; level <= max; level++) {
    for (int i = 0; i < cd->n; i++) {
      if (cond(phase, i, level, cd)) {
        if (bufs->idx_buf != NULL) {
          idx_mbuf[bufs->mcount[phase]] = level * cd->n + i;
        }
        bufs->mcount[phase]++;
      }
    }
  }
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

static void fill_bufsize_rscount_displs(
    comm_data_t *cd,
    buffers_t *bufs,
    char *comm_table,
    int *store_part)
{
  bufs->idx_buf = NULL;
  clear_comm_table(cd, comm_table);
  init_comm_table(cd, comm_table);
  int size = cd->n * (cd->nlevel + 1);
  char *cursp = (char *)malloc(sizeof(*cursp) * size);
  for (int phase = 0; phase < cd->nphase; phase++) {
    // Clear cursp
    for (int i = 0; i < size; i++) cursp[i] = 0;
    if (phase > 0) {
      collect_comm(phase, cd, store_part, comm_table);
      reduce_comm(phase, cd, comm_table, store_part, cursp);
      clear_comm_table(cd, comm_table);
      for (int i = 0; i < size; i++) if (cursp[i]) store_part[i] = -1;
    }
    phase_comm_table(phase, cd, comm_table, store_part, cursp);
    iterator(phase_cond, phase, cd, bufs);
    fill_rscounts(phase, bufs, comm_table);
    fill_displs(phase, bufs);
    clear_comm_table(cd, comm_table);
  }
  free(cursp);
  skirt_comm_table(cd, comm_table, store_part);
  iterator(skirt_cond, cd->nphase, cd, bufs);
  fill_rscounts(cd->nphase, bufs, comm_table);
  fill_displs(cd->nphase, bufs);
}

static void alloc_bufs(buffers_t *bufs)
{
  bufs->buf_count = 0;
  bufs->buf_scount = 0;
  bufs->mptr_count = 0;
  for (int phase = 0; phase <= bufs->nphase; phase++) {
    bufs->buf_count += bufs->rcount[phase];
    bufs->buf_count += bufs->mcount[phase];
    bufs->buf_scount += bufs->scount[phase];
    bufs->mptr_count += bufs->mcount[phase] + 1;
  }

  bufs->idx_buf = (long *)malloc(sizeof(*bufs->idx_buf) * bufs->buf_count);
  bufs->idx_sbuf = (long *)malloc(sizeof(*bufs->idx_sbuf) * bufs->buf_scount);
  bufs->mptr_buf = (long *)malloc(sizeof(*bufs->mptr_buf) * bufs->mptr_count);
  assert(bufs->idx_buf != NULL);
  assert(bufs->idx_sbuf != NULL);
  assert(bufs->mptr_buf != NULL);

  long count = 0;
  long scount = 0;
  long mptrcount = 0;
  for (int phase = 0; phase <= bufs->nphase; phase++) {
    bufs->rbuf_offsets[phase] = count;
    count += bufs->rcount[phase];

    bufs->mbuf_offsets[phase] = count;
    count += bufs->mcount[phase];

    bufs->sbuf_offsets[phase] = scount;
    scount += bufs->scount[phase];

    bufs->mptr_offsets[phase] = mptrcount;
    mptrcount += bufs->mcount[phase] + 1;
  }
}

static void fill_idx_rsbuf(int phase, char *comm_table, buffers_t *bufs) {
  // Needs idx buffers allocated.
  int idx;
  int scounter = 0;
  int rcounter = 0;
  long *idx_rbuf = bufs->idx_buf + bufs->rbuf_offsets[phase];
  long *idx_sbuf = bufs->idx_sbuf + bufs->sbuf_offsets[phase];
  for (int p = 0; p < bufs->npart; p++) {
    // For all vv indices.
    for (int i = 0; i < bufs->nlevel * bufs->n; ++i) {
      // Here p is the destination (to) partition.
      idx = get_ct_idx(bufs->rank, p, i, bufs->n, bufs->npart, bufs->nlevel);
      if (comm_table[idx]) {
        idx_sbuf[scounter] = i;
        scounter++;
      }
      idx = get_ct_idx(p, bufs->rank, i, bufs->n, bufs->npart, bufs->nlevel);
      if (comm_table[idx]) {
        idx_rbuf[rcounter] = i;
        rcounter++;
      }
    }
  }
}

static void fill_mptr(comm_data_t *cd, buffers_t *bufs, int phase) {
  int *ptr = cd->graph->ptr;
  int mcount = bufs->mcount[phase];
  long *mptr = bufs->mptr_buf + bufs->mptr_offsets[phase];
  long *idx_mbuf = bufs->idx_buf + bufs->mbuf_offsets[phase];
  assert(mptr != NULL);
  mptr[0] = 0;
  for (int mi = 0; mi < mcount; mi++) {
    int i = idx_mbuf[mi] % cd->n;
    mptr[mi + 1] = mptr[mi] + ptr[i + 1] - ptr[i];
  }
}

static void fill_bufs(
    comm_data_t *cd,
    buffers_t *bufs,
    char *comm_table,
    int *store_part)
{
  clear_comm_table(cd, comm_table);
  init_comm_table(cd, comm_table);
  for (int phase = 0; phase < cd->nphase; phase++) {
    phase_comm_table(phase, cd, comm_table, store_part, NULL);
    iterator(phase_cond, phase, cd, bufs);
    fill_idx_rsbuf(phase, comm_table, bufs);
    clear_comm_table(cd, comm_table);
    fill_mptr(cd, bufs, phase);
  }
  skirt_comm_table(cd, comm_table, store_part);
  iterator(skirt_cond, cd->nphase, cd, bufs);
  fill_idx_rsbuf(cd->nphase, comm_table, bufs);
  fill_mptr(cd, bufs, cd->nphase);
}

static void alloc_mcol(buffers_t *bufs) {
  bufs->mcol_count = 0;
  for (int phase = 0; phase <= bufs->nphase; phase++) {
    long *mptr = bufs->mptr_buf + bufs->mptr_offsets[phase];
    int mcount = bufs->mcount[phase];
    bufs->mcol_offsets[phase] = bufs->mcol_count;
    bufs->mcol_count += mptr[mcount];
  }
  bufs->mcol_buf = (long *)malloc(sizeof(*bufs->mcol_buf) * bufs->mcol_count);
  assert(bufs->mcol_buf != NULL);
}

static void fill_mcol(comm_data_t *cd, buffers_t *bufs, int *find_idx) {
  int *ptr = cd->graph->ptr;
  int *col = cd->graph->col;
  for (int phase = 0; phase <= bufs->nphase; phase++){
    long *mptr = bufs->mptr_buf + bufs->mptr_offsets[phase];
    int mcount = bufs->mcount[phase];
    long *idx_mbuf = bufs->idx_buf + bufs->mbuf_offsets[phase];
    long *mcol = bufs->mcol_buf + bufs->mcol_offsets[phase];
    assert(mcol != NULL);
    for (int mi = 0; mi < mcount; mi++) {
      long idx = idx_mbuf[mi];
      long i = idx % cd->n;
      long level = idx / cd->n;

      assert(mptr[mi + 1] - mptr[mi] == ptr[i + 1] - ptr[i]);
      for (int j = ptr[i]; j < ptr[i + 1]; j++) {
        long target = col[j] + cd->n * (level - 1);
        int idx = find_idx[target];
        mcol[j - ptr[i] + mptr[mi]] = idx;
      }
    }
  }
}

static void fill_mcol_sbuf(
    comm_data_t *cd,
    buffers_t *bufs)
{
  alloc_mcol(bufs);

  // TODO(vatai): Combine temporary data
  int N = bufs->n * (bufs->nlevel + 1);
  int *find_idx = (int *)malloc(sizeof(int) * N);
  for (int j = 0; j < N; j++) find_idx[j] = -1;
  for (int i = 0; i < bufs->buf_count; i++) {
    int idx = bufs->idx_buf[i];
    if (find_idx[idx] == -1)
      find_idx[idx] = i;
  }
  fill_mcol(cd, bufs, find_idx);

  // Store the idx_buf indices, because idx_sbufs are used for copying
  // data from idx_buf to sbuf.
  for (int i = 0; i < bufs->buf_scount; i++) {
    int vv_idx = bufs->idx_sbuf[i];
    int buf_idx = find_idx[vv_idx]; // FIX
    bufs->idx_sbuf[i] = buf_idx;
  }
  free(find_idx);
}

void check_args(int argc, char *argv0) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s dirname\n", argv0);
    exit(1);
  }
}

static void alloc_bufs0(buffers_t *bufs) {
  int npart = bufs->npart;
  int nphase = bufs->nphase;

  bufs->recvcounts = (int *)malloc(sizeof(*bufs->recvcounts) * npart * (nphase + 1));
  assert(bufs->recvcounts != NULL);
  bufs->sendcounts = (int *)malloc(sizeof(*bufs->sendcounts) * npart * (nphase + 1));
  assert(bufs->sendcounts != NULL);

  bufs->rdispls = (int *)malloc(sizeof(*bufs->rdispls) * npart * (nphase + 1));
  assert(bufs->rdispls != NULL);
  bufs->sdispls = (int *)malloc(sizeof(*bufs->sdispls) * npart * (nphase + 1));
  assert(bufs->sdispls != NULL);

  bufs->rcount = (int *)malloc(sizeof(*bufs->rcount) * (nphase + 1));
  assert(bufs->rcount != NULL);
  bufs->mcount = (int *)malloc(sizeof(*bufs->mcount) * (nphase + 1));
  assert(bufs->mcount != NULL);
  bufs->scount = (int *)malloc(sizeof(*bufs->scount) * (nphase + 1));
  assert(bufs->scount != NULL);

  bufs->rbuf_offsets = (int *)malloc(sizeof(*bufs->rbuf_offsets) * (nphase + 1));
  assert(bufs->rbuf_offsets != NULL);
  bufs->mbuf_offsets = (int *)malloc(sizeof(*bufs->mbuf_offsets) * (nphase + 1));
  assert(bufs->mbuf_offsets != NULL);
  bufs->sbuf_offsets = (int *)malloc(sizeof(*bufs->sbuf_offsets) * (nphase + 1));
  assert(bufs->sbuf_offsets != NULL);

  bufs->mptr_offsets = (int *)malloc(sizeof(*bufs->mptr_offsets) * (nphase + 1));
  assert(bufs->mptr_offsets != NULL);
  bufs->mcol_offsets = (int *)malloc(sizeof(*bufs->mcol_offsets) * (nphase + 1));
  assert(bufs->mcol_offsets != NULL);

  // use alloc_val_bufs for these
  bufs->vv_buf = NULL;
  bufs->vv_sbuf = NULL;
  bufs->mval_buf = NULL;
}

buffers_t *new_bufs(comm_data_t *cd) {
  buffers_t *bufs = (buffers_t *)malloc(sizeof(*bufs));
  bufs->n = cd->n;
  bufs->npart = cd->npart;
  bufs->nlevel = cd->nlevel;
  bufs->nphase = cd->nphase;
  bufs->rank = cd->rank;
  alloc_bufs0(bufs);
  return bufs;
}

void alloc_val_bufs(buffers_t *bufs) {
  // TODO(vatai): eventually mval
  bufs->vv_buf = (double *)malloc(sizeof(*bufs->vv_buf) * bufs->buf_count);
  bufs->vv_sbuf = (double *)malloc(sizeof(*bufs->vv_sbuf) * bufs->buf_scount);
  assert(bufs->vv_buf != NULL);
  assert(bufs->vv_sbuf != NULL);
}

void del_bufs(buffers_t *bufs) {
  free(bufs->recvcounts);
  free(bufs->sendcounts);
  free(bufs->rdispls);
  free(bufs->sdispls);

  free(bufs->rcount);
  free(bufs->mcount);
  free(bufs->scount);

  free(bufs->rbuf_offsets);
  free(bufs->mbuf_offsets);
  free(bufs->sbuf_offsets);

  free(bufs->idx_buf);
  free(bufs->idx_sbuf);

  free(bufs->mptr_buf);
  free(bufs->mcol_buf);

  free(bufs->mptr_offsets);
  free(bufs->mcol_offsets);

  if (bufs->vv_buf != NULL)
    free(bufs->vv_buf);
  if (bufs->vv_sbuf != NULL)
    free(bufs->vv_sbuf);
  if (bufs->mval_buf != NULL)
    free(bufs->mval_buf);
  free(bufs);
}

void fill_buffers(comm_data_t *cd, buffers_t *bufs) {
  assert(cd != NULL);

  char *comm_table = new_comm_table(cd->n, cd->npart, cd->nlevel);
  int *store_part = new_store_part(cd);

  fill_bufsize_rscount_displs(cd, bufs, comm_table, store_part);
  alloc_bufs(bufs);
  fill_bufs(cd, bufs, comm_table, store_part);
  fill_mcol_sbuf(cd, bufs);

  free(comm_table);
  free(store_part);
}

void write_buffers(buffers_t *bufs, char *dir) {
  int count;
  char fname[1024];
  sprintf(fname, "%s/rank%d.bufs", dir, bufs->rank);
  FILE *file = fopen(fname, "w");

  count = 1;
  fwrite(&bufs->n, sizeof(bufs->n), count, file);
  fwrite(&bufs->npart, sizeof(bufs->npart), count, file);
  fwrite(&bufs->nlevel, sizeof(bufs->nlevel), count, file);
  fwrite(&bufs->nphase, sizeof(bufs->nphase), count, file);
  fwrite(&bufs->rank, sizeof(bufs->rank), count, file);
  fwrite(&bufs->buf_count, sizeof(bufs->buf_count), count, file);
  fwrite(&bufs->buf_scount, sizeof(bufs->buf_scount), count, file);
  fwrite(&bufs->mptr_count, sizeof(bufs->mptr_count), count, file);
  fwrite(&bufs->mcol_count, sizeof(bufs->mcol_count), count, file);

  count = bufs->npart * (bufs->nphase + 1);
  fwrite(bufs->recvcounts, sizeof(*bufs->recvcounts), count, file);
  fwrite(bufs->sendcounts, sizeof(*bufs->sendcounts), count, file);
  fwrite(bufs->rdispls, sizeof(*bufs->rdispls), count, file);
  fwrite(bufs->sdispls, sizeof(*bufs->sdispls), count, file);

  count = bufs->nphase + 1;
  fwrite(bufs->rcount, sizeof(*bufs->rcount), count, file);
  fwrite(bufs->mcount, sizeof(*bufs->mcount), count, file);
  fwrite(bufs->scount, sizeof(*bufs->scount), count, file);
  fwrite(bufs->rbuf_offsets, sizeof(*bufs->rbuf_offsets), count, file);
  fwrite(bufs->mbuf_offsets, sizeof(*bufs->mbuf_offsets), count, file);
  fwrite(bufs->sbuf_offsets, sizeof(*bufs->sbuf_offsets), count, file);
  fwrite(bufs->mptr_offsets, sizeof(*bufs->mptr_offsets), count, file);
  fwrite(bufs->mcol_offsets, sizeof(*bufs->mcol_offsets), count, file);

  // variable length vectors
  fwrite(bufs->idx_buf, sizeof(*bufs->idx_buf), bufs->buf_count, file);
  fwrite(bufs->idx_sbuf, sizeof(*bufs->idx_sbuf), bufs->buf_scount, file);
  fwrite(bufs->mptr_buf, sizeof(*bufs->mptr_buf), bufs->mptr_count, file);
  fwrite(bufs->mcol_buf, sizeof(*bufs->mcol_buf), bufs->mcol_count, file);
  // If we don't use bufs->mval_buf
  if (bufs->mval_buf != NULL)
    fwrite(bufs->mval_buf, sizeof(*bufs->mval_buf), bufs->mcol_count, file);

  fclose(file);
}

buffers_t *read_buffers(char *dir, int rank) {
  buffers_t *bufs = (buffers_t *)malloc(sizeof(*bufs));

  char fname[1024];
  sprintf(fname, "%s/rank%d.bufs", dir, rank);
  FILE *file = fopen(fname, "r");
  if (file == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }

  int count = 1;
  fread(&bufs->n, sizeof(bufs->n), count, file);
  fread(&bufs->npart, sizeof(bufs->npart), count, file);
  fread(&bufs->nlevel, sizeof(bufs->nlevel), count, file);
  fread(&bufs->nphase, sizeof(bufs->nphase), count, file);
  fread(&bufs->rank, sizeof(bufs->rank), count, file);
  fread(&bufs->buf_count, sizeof(bufs->buf_count), count, file);
  fread(&bufs->buf_scount, sizeof(bufs->buf_scount), count, file);
  fread(&bufs->mptr_count, sizeof(bufs->mptr_count), count, file);
  fread(&bufs->mcol_count, sizeof(bufs->mcol_count), count, file);

  alloc_bufs0(bufs);

  count = bufs->npart * (bufs->nphase + 1);
  fread(bufs->recvcounts, sizeof(*bufs->recvcounts), count, file);
  fread(bufs->sendcounts, sizeof(*bufs->sendcounts), count, file);
  fread(bufs->rdispls, sizeof(*bufs->rdispls), count, file);
  fread(bufs->sdispls, sizeof(*bufs->sdispls), count, file);

  count = bufs->nphase + 1;
  fread(bufs->rcount, sizeof(*bufs->rcount), count, file);
  fread(bufs->mcount, sizeof(*bufs->mcount), count, file);
  fread(bufs->scount, sizeof(*bufs->scount), count, file);
  fread(bufs->rbuf_offsets, sizeof(*bufs->rbuf_offsets), count, file);
  fread(bufs->mbuf_offsets, sizeof(*bufs->mbuf_offsets), count, file);
  fread(bufs->sbuf_offsets, sizeof(*bufs->sbuf_offsets), count, file);
  fread(bufs->mptr_offsets, sizeof(*bufs->mptr_offsets), count, file);
  fread(bufs->mcol_offsets, sizeof(*bufs->mcol_offsets), count, file);

  // variable length vectors
  bufs->idx_buf = (long *)malloc(sizeof(*bufs->idx_buf) * bufs->buf_count);
  fread(bufs->idx_buf, sizeof(*bufs->idx_buf), bufs->buf_count, file);
  bufs->idx_sbuf = (long *)malloc(sizeof(*bufs->idx_sbuf) * bufs->buf_scount);
  fread(bufs->idx_sbuf, sizeof(*bufs->idx_sbuf), bufs->buf_scount, file);
  bufs->mptr_buf = (long *)malloc(sizeof(*bufs->mptr_buf) * bufs->mptr_count);
  fread(bufs->mptr_buf, sizeof(*bufs->mptr_buf), bufs->mptr_count, file);
  bufs->mcol_buf = (long *)malloc(sizeof(*bufs->mcol_buf) * bufs->mcol_count);
  fread(bufs->mcol_buf, sizeof(*bufs->mcol_buf), bufs->mcol_count, file);

  // If we don't use bufs->mval_buf
  bufs->mval_buf = (double *)malloc(sizeof(*bufs->mval_buf) * bufs->mcol_count);
  int chread =
      fread(bufs->mval_buf, sizeof(*bufs->mval_buf), bufs->mcol_count, file);
  if (chread < bufs->mcol_count){
    free(bufs->mval_buf);
    bufs->mval_buf = NULL;
  }

  // These should be allocated with alloc_val_bufs()!
  bufs->vv_buf = NULL;
  bufs->vv_sbuf = NULL;
  fclose(file);
  return bufs;
}

