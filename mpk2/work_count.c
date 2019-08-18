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

int main(int argc, char *argv[]) {
  comm_data_t *cd = new_comm_data(argv[1], 0);
  int npart = cd->npart;
  buffers_t **bufs_arr = malloc(sizeof(*bufs_arr) * npart);
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
  return 0;
}
