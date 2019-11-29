/// @file
/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-10-31

#ifndef _BUFFERS_H_
#define _BUFFERS_H_

#include "comm_data.h"

#ifdef __cplusplus
extern "C" {
#endif
/// @brief Buffers for processing.
///
/// @details Contains the buffers created by the by the
/// `mpi2_mpkprep.c` program and processed by the
typedef struct buffers_t {
  int n;
  int npart;
  int nlevel;
  int nphase;
  int rank;
  int buf_count;
  int buf_scount;
  int mptr_count;
  int mcol_count;

  // npart * (nphase + 1)
  int *recvcounts;
  int *sendcounts;
  int *rdispls;
  int *sdispls;

  // nphase + 1
  int *rcount;
  int *mcount;
  int *scount;
  int *rbuf_offsets;
  int *mbuf_offsets;
  int *sbuf_offsets;
  int *mptr_offsets;
  int *mcol_offsets;

  long *idx_buf; // Important note in iterator() function comments
  long *idx_sbuf;
  long *mptr_buf;
  long *mcol_buf;

  double *vv_buf;
  double *vv_sbuf;
  double *mval_buf;
} buffers_t;

/// @brief Check the number of arguments.
///
/// @details Most of the programs work on a single directory and
/// obtain the parameters from the directory name, so most of the
/// files need to check if the a single argument was provided.
void check_args(int argc, char *argv0);

/// @brief Allocate a `buffers_t` `struct`.
///
/// @param[in] cd is a communication data structure, from the `driver`
/// and `comp` programs.
///
/// @return Pointer to newly allocated `buffers_t`.
///
/// @details Fills the basic data from @p cd such as `n`, `npart`
/// etc. and allocate buffers/arrays from the information available
/// from @p cd (mostly pointer arrays of approximately `nphase` size).
/// Deallocate with `del_bufs()`.
buffers_t *new_bufs(comm_data_t *);

/// @brief Allocate the `vv_buf`, `vv_sbuf` arrays in @p bufs.
///
/// @param[in,out] bufs the pointer to `buffers_t` object.
///
/// @details The `vv_buf` is allocated based on the `buf_count`, and
/// `vv_sbuf` based on `buf_scount`.
void alloc_val_bufs(buffers_t *);

/// @brief Deallocate a `buffers_t` allocated with `new_bufs`.
///
/// @param[in,out] bufs to deallocate.
///
/// @details Needs to be called. Considers the case when `vv_buf` and
/// `vv_sbuf` are not allocated.
void del_bufs(buffers_t *);

/// @brief The main procedure which fills all the buffers with the
/// communication and computation patterns.
///
/// @param[in] cd communication data.
///
/// @param[in,out] bufs buffers with allocate arrays.
///
/// @details The main steps:
/// - call `fill_bufsize_rscount_displs` and fill the basic MPI related arrays.
///
/// - call `alloc_bufs` to allocate `idx_buf`, `idx_sbuf`, `mptr_buf`,
/// fill `buf_count`, `buf_scount`, `mptr_count`, `rbuf_offsets`,
/// `mbuf_offsets` and `mptr_offsets`.
///
/// - call `fill_bufs` to fill index `rbus` and `sbufs`, `mptr`.
///
/// - fill `fill_mcol_sbuf` to fill `mcol`.
///
/// It uses `comm_table` and `store_part` to store inter-phase and
/// inter-partition communication.
void fill_buffers(comm_data_t *, buffers_t *);

/// @brief Write out the buffers to a file (read with `read_buffers`).
///
/// @param[in] bufs is the data structure which holds all the buffers
/// needed to be saved.
///
/// @param[in] dir the directory where the `rankN.bufs` will be
/// written.
///
/// @details Write all the important data which is used by the program
/// that preforms the calculations/computations. It treats the case
/// when `mval_buf` as a special case.
void write_buffers(buffers_t *, char *);

/// @brief Read the buffers to a file (write with `write_buffers`).
///
/// @param[in] dir the directory where the `rankN.bufs` will be read
/// from.
///
/// @param[in] the `N` in `rankN.bufs`.
///
/// @return A newly allocated `buffers_t` object filled with data from
///
/// @details Read the data written by `write_buffers`.
buffers_t *read_buffers(char *, int);

#ifdef __cplusplus
}
#endif

#endif
