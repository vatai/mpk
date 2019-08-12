#ifndef _COMM_DATA_H_
#define _COMM_DATA_H_

#include "lib.h"

typedef struct comm_table {
  char *dir;
  int n;
  int nlevel;
  int npart;
  int nphase;
  int rank;

  crs0_t *graph;
  part_t **plist;
  level_t **llist;
  skirt_t *skirt;
} comm_data_t;

comm_data_t *new_comm_data(char *);

void del_comm_data(comm_data_t *);

#endif