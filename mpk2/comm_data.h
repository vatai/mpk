#ifndef _COMM_DATA_H_
#define _COMM_DATA_H_

#include "lib.h"

#ifdef __cplusplus
extern "C" {
#endif

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

void dir_name_error(char *dir);

crs0_t *read_matrix(char *dir);

double *alloc_read_val(crs0_t *g0, char *dir);

comm_data_t *new_comm_data(char *dir, int rank);

void del_comm_data(comm_data_t *cd);

#ifdef __cplusplus
}
#endif

#endif
