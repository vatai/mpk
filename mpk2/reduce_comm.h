#ifndef _REDUCE_COMM_H_
#define _REDUCE_COMM_H_

#include "comm_data.h"

void debug_print(comm_data_t *cd, char *comm_table);

void reduce_comm(
    int phase,
    comm_data_t *cd,
    char *comm_table,
    int *store_part,
    char *cursp);

#endif
