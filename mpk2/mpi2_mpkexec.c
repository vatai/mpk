#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mpi.h>
#include <math.h>

#include "lib.h"
#include "mpi2_lib.h"

static void print_values_of_vv(int rank, int phase, int n, int nlevel, double *vv, char *dir) {
  char fname[1024];
  sprintf(fname,"%s/vv_after_phase_%d_with_rank%d", dir, phase, rank);
  FILE *vv_log_file = fopen(fname, "w");
  int ns = sqrt(n);
  for (int level = 0; level < nlevel + 1; level++) {
    fprintf(vv_log_file, "> level(%3d): \n", level);
    for (int i = 0; i < n; i++) {
      if (i % ns == 0) {
        fprintf(vv_log_file, "\n");
      }
      fprintf(vv_log_file, " %8.3f", vv[level * n + i]);
    }
    fprintf(vv_log_file, "\n\n");
  }
  fclose(vv_log_file);

  return;
}

void log_tlist(comm_data_t *cd, int phase, FILE *log_file) {
  task_t *tl = cd->mg->tlist + phase * cd->npart + cd->rank;
  int n = cd->n;
  for (int i = 0; i < tl->n; i++) {
    int idx = tl->idx[i];
    int level = idx / n;
    int j = idx % n;
    fprintf(log_file, "%d (level:%3d, i: %3d)\n", idx, level, j);
  }
}

void log_cd(double *vv_bufs, long *idx_bufs, int *count, int *displs, int n,
            FILE *log_file) {
  int npart;
  MPI_Comm_size(MPI_COMM_WORLD, &npart);

  int total = 0;
  for (int p = 0; p < npart; p++) {
    total += count[p];
  }
  assert(total == count[npart - 1] + displs[npart - 1]);

  fprintf(log_file, "[i<%3d] %8s, %8s\n", total, "vv_buf", "idx_buf");
  for (int i = 0; i < total; i++) {
    long idx = idx_bufs[i];
    int level = idx / n;
    int j = idx % n;
    fprintf(log_file, "[%5d] %8.3f, %8d (level: %d, j: %d)\n", i, vv_bufs[i],
            idx, level, j);
  }

  fprintf(log_file, "[p<%3d] %8s, %8s\n", npart, "count", "disp");
  for (int p = 0; p < npart; p++) {
    fprintf(log_file, "[%5d] %8d, %8d\n", p, count[p], displs[p]);
  }
}

// TODO(vatai): delete this, see trivial optimisation below
int find_idx(long *ptr, int size, long target);

static void do_comm(int phase, comm_data_t *cd, double *vv, FILE *log_file) {
  int npart = cd->npart;

  // Log tlist.
  fprintf(log_file, "\n\n==== TLIST: part/rank %d : phase %d ====)\n", cd->rank,
          phase);
  log_tlist(cd, phase, log_file);

  // Copy data to send buffers.
  int stotal = cd->sendcounts[npart * phase + npart - 1] +
               cd->sdispls[npart * phase + npart - 1];
  for (int i = 0; i < stotal; ++i)
    cd->vv_sbufs[phase][i] = vv[cd->idx_sbufs[phase][i]];

  // Log send buffers.
  fprintf(log_file, "\n\n<<<< SEND: part/rank %d : phase %d >>>>\n\n", cd->rank,
          phase);
  log_cd(cd->vv_sbufs[phase], cd->idx_sbufs[phase],
         cd->sendcounts + npart * phase, cd->sdispls + npart * phase, cd->n,
         log_file);

  // Do communication.
  MPI_Alltoallv(cd->vv_sbufs[phase], cd->sendcounts + npart * phase,
                cd->sdispls + npart * phase, MPI_DOUBLE, cd->vv_rbufs[phase],
                cd->recvcounts + npart * phase, cd->rdispls + npart * phase,
                MPI_DOUBLE, MPI_COMM_WORLD);

  // Log receive buffers.
  fprintf(log_file, "\n\n>>>> RECV: part/rank %d : phase %d <<<<\n\n", cd->rank,
          phase);
  log_cd(cd->vv_rbufs[phase], cd->idx_rbufs[phase],
         cd->recvcounts + npart * phase, cd->rdispls + npart * phase, cd->n,
         log_file);

  // Copy from receive buffers.
  int rtotal = cd->recvcounts[npart * phase + npart - 1] +
               cd->rdispls[npart * phase + npart - 1];
  for (int i = 0; i < rtotal; ++i)
    vv[cd->idx_rbufs[phase][i]] = cd->vv_rbufs[phase][i];
}

static void do_task(comm_data_t *cd, double *vv, int phase, int part) {
  assert(cd != NULL && vv != NULL);

  int n = cd->n;

  task_t *tl = cd->mg->tlist + phase * cd->npart + part;
  int *ptr = cd->mg->g0->ptr;
  int *col = cd->mg->g0->col;

  // TODO(vatai): tl->th = omp_get_thread_num();
  // TODO(vatai): tl->t0 = omp_get_wtime();

  int t;
  for (t=0; t< tl->n; t++) {
    int tidx = tl->idx[t];
    int level = tidx / n;
    int l1n = (level - 1) * n;
    int i = tidx - level * n; /*tidx % n*/
    double a = 1.0 / (ptr[i+1] - ptr[i]); /* simply... */

    double s = 0.0;
    int j;
    for (j = ptr[i]; j < ptr[i+1]; j++) {
      s += a * vv[l1n + col[j]];
    }
    vv[level * n + i] = s;
  }

  // TODO(vatai): tl->t1 = omp_get_wtime();
}

void mpi_exec_mpk(mpk_t *mg, double *vv, comm_data_t *cd, char *dir) {
  assert(mg != NULL && vv != NULL && cd != NULL);

  int n = mg->n;
  int npart = mg->npart;
  int nlevel = mg->nlevel;
  int nphase = mg->nphase;

  char fname[1024];
  sprintf(fname, "mpi_comm_in_exec_rank-%d.log", cd->rank);
  FILE *log_file = fopen(fname, "w");

  int phase;
  for (phase = 0; phase <= nphase; phase++) {
    if (phase > 0 || nphase == 0) {
      do_comm(phase, cd, vv, log_file);
    }

    do_task(cd, vv, phase, cd->rank);

    print_values_of_vv(cd->rank, phase, n, nlevel, vv, dir);
  }
  fclose(log_file);
}
