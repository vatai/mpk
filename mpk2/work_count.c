#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "comm_data.h"
#include "buffers.h"

int count_comm_phase(int phase, buffers_t **bufs_arr) {
  int npart = bufs_arr[0]->npart;
  int sum = 0;
  for (int p = 0; p < npart; p++) {
    buffers_t *bufs = bufs_arr[p];
    sum += bufs->rcount[phase];
    sum -= bufs->recvcounts[npart * phase + p];
    assert(bufs->sendcounts[npart * phase + p] ==
           bufs->recvcounts[npart * phase + p]);
  }
  return sum;
}

int count_comm(buffers_t **bufs_arr) {
  // For each phase sum up the number of communicated doubles, minus
  // the elements to remain on the same node.
  int nphase = bufs_arr[0]->nphase;
  int sum = 0;
  for (int phase = 0; phase <= nphase; phase++) {
    sum += count_comm_phase(phase, bufs_arr);
  }
  return sum;
}

int count_ops_phase(int phase, buffers_t **bufs_arr) {
  int npart = bufs_arr[0]->npart;
  int sum = 0;
  for (int p = 0; p < npart; p++) {
    buffers_t *bufs = bufs_arr[p];
    long *mptr = bufs->mptr_buf + bufs->mptr_offsets[phase];
    int mcount = bufs->mcount[phase];
    for (int t = 0; t < mcount; t++) {
      sum += mptr[t + 1] - mptr[t];
    }
  }
  return sum;
}

int count_ops(buffers_t **bufs_arr) {
  int nphase = bufs_arr[0]->nphase;
  int sum = 0;
  for (int phase = 0; phase <= nphase; phase++) {
    sum += count_ops_phase(phase, bufs_arr);
  }
  return sum;
}

int count_min_ops(comm_data_t *cd) {
  int n = cd->graph->n;
  int sum = 0;
  int *ptr = cd->graph->ptr;
  for (int i = 0; i < n; i++) {
    sum += ptr[i + 1] - ptr[i];
  }
  return (cd->nlevel * sum);
}

void redundancy(buffers_t **bufs_arr) {
  // This subprogram checks for redundancies, i.e. if a vertex was
  // both sent and calculated on a partition.
  int npart = bufs_arr[0]->npart;
  int size = bufs_arr[0]->n * (bufs_arr[0]->nlevel + 1);
  char *tmp = (char *)malloc(sizeof(*tmp) * size);
  for (int p = 0; p < npart; p++) {
    buffers_t* bufs = bufs_arr[p];
    for (int phase = 0; phase <= bufs->nphase; phase++) {
      // Clear tmp[].
      for (int i = 0; i < size; i++)
        tmp[i] = 0;
      // For every index in the current rbuf set the first bit.
      int rcount = bufs->rcount[phase];
      long *rbuf = bufs->idx_buf + bufs->rbuf_offsets[phase];
      for (int ri = 0; ri < rcount; ri++)
        tmp[rbuf[ri]] += 1;
      for (int ph = 0; ph <= bufs->nphase; ph++) {
        // For every index in all mbufs (for all phases) set the
        // second bit.
        int mcount = bufs->mcount[ph];
        long *mbuf = bufs->idx_buf + bufs->mbuf_offsets[ph];
        for (int mi = 0; mi < mcount; mi++)
          tmp[mbuf[mi]] += 2;
      }
      for (int i = 0; i < size; i++) {
        if (tmp[i] == 3) {
          printf("!!!Redundancy i=%d!!!\n", i);
          printf("!!!This shouldn't happen (%s::%d)\n", __FILE__, __LINE__);
        }
      }
    }
  }
}

void print_header(char *dir) {
  printf("Summary for %s\n", dir);
}

void print_comm(int sum) {
  printf(
      "%d number of doubles (of size %lu) send/received. %lu bytes in total.\n",
      sum, sizeof(double), sizeof(double) * sum);
}

void print_ops(int ops, int minops, int redops, int nvert) {
  printf(
      "%d ops to compute %d number of vertices (minimum ops %d, reduntant %d)\n",
      ops, nvert, minops, redops);
}

void save_summary(char *dir, int comm, int ops, int minops, int nvert) {
  char fname[1024];
  sprintf(fname, "summary-%s.log", dir);
  FILE *file = fopen(fname, "w");
  fprintf(file, "#%14s%15s%15s%15s\n", "communication", "ops", "minops", "nvert");
  fprintf(file, "%15d%15d%15d%15d\n", comm, ops, minops, nvert);
  fclose(file);
}

int main(int argc, char *argv[]) {
  comm_data_t *cd = new_comm_data(argv[1], 0);
  int npart = cd->npart;
  buffers_t **bufs_arr = (buffers_t **)malloc(sizeof(*bufs_arr) * npart);
  for (int p = 0; p < npart; p++)
    bufs_arr[p] = read_buffers(argv[1], p);

  print_header(argv[1]);
  int elems = count_comm(bufs_arr);
  print_comm(elems);

  int ops = count_ops(bufs_arr);
  int minops = count_min_ops(cd);
  int nvert = cd->n * cd->nlevel;
  int redops = ops - minops;
  print_ops(ops, minops, redops, nvert);
  save_summary(argv[1], elems, ops, minops, nvert);
  redundancy(bufs_arr);
  return 0;
}
