#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "buffers.h"

int get_npart(char *dir) {
  int n = strlen(dir);
  int count = 0;
  for (int i = 0; i < n; i++) {
    if (dir[i] == '_') count++;
  }
  assert(count == 3);

  count = 0;
  int npart;
  while(dir[count] != '_') count++;
  sscanf(dir + count + 1, "%d", &npart);
  return npart;
}

int sum_all(buffers_t **bufs_arr){
  int npart = bufs_arr[0]->npart;
  int nphase = bufs_arr[0]->nphase;
  int sum = 0;
  for (int phase = 0; phase < nphase; phase++) {
    for (int p = 0; p < npart; p++) {
      sum += bufs_arr[p]->rcount[phase];
    }
  }
  return sum;
}

int main(int argc, char *argv[]) {
  int npart = get_npart(argv[1]);
  buffers_t **bufs_arr = malloc(sizeof(*bufs_arr) * npart);
  for (int p = 0; p < npart; p++)
    bufs_arr[p] = read_buffers(argv[1], p);
  int sum = sum_all(bufs_arr);
  printf("communication: %d\n", sum);
  return 0;
}
