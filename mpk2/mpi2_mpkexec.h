#ifndef _MPI2_MPKEXEC_H_
#define _MPI2_MPKEXEC_H_

#include "buffers.h"

#ifdef __cplusplus
extern "C" {
#endif

void mpi_exec_mpk(buffers_t *);

void collect_results(buffers_t *, double *);

int check_results(buffers_t *, double *);

#ifdef __cplusplus
}
#endif

#endif
