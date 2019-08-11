#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#include "lib.h"
#include "mpi2_lib.h"

#define DETAIL 0
#define ONEVEC 0
#define ONEENT 0
#define TRANS 0

void test_allltoall_inputs(comm_data_t *cd) {
  printf("testing all inputs and printing out_mpi_alltoall:\n");
  int n = cd->n;
  int npart = cd->npart;

  char name[100];
  sprintf(name,"%d_out_mpi_alltoall.log", cd->rank);
  FILE *f = fopen(name, "w");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", name);
    exit(1);
  }

  int *sendcount = cd->sendcounts + npart;
  int *recvcount = cd->recvcounts + npart;
  int *sdispls = cd->sdispls + npart;
  int *rdispls = cd->rdispls + npart;
  for (int phase  = 1; phase < cd->nphase; ++phase){
    fprintf(f, "Phase:%d\n",phase);
    fprintf(f, "sbufs-\n");
    for(int i = 0; i< sendcount[npart-1]+sdispls[npart-1];i++){
      fprintf(f, " %f", cd->vv_sbufs[phase][i]);
    }
    fprintf(f, "\n");
    fprintf(f, "idx_sbufs-\n");
    for(int i = 0; i< sendcount[npart-1]+sdispls[npart-1];i++){
      fprintf(f, " %d", cd->idx_sbufs[phase][i]);
    }
    fprintf(f, "\n");
    fprintf(f, "sendcount-\n");
    for(int i = 0; i< npart;i++){
      fprintf(f, " %d", sendcount[i]);
    }
    fprintf(f, "\n");
    fprintf(f, "recvcount-\n");
    for(int i = 0; i< npart;i++){
      fprintf(f, " %d", recvcount[i]);
    }
    fprintf(f, "\n");
    fprintf(f, "sdispls-\n");
    for(int i = 0; i< npart;i++){
      fprintf(f, " %d", sdispls[i]);
    }
    fprintf(f, "\n");
    fprintf(f, "rdispls-\n");
    for(int i = 0; i< npart;i++){
      fprintf(f, " %d", rdispls[i]);
    }
    fprintf(f, "\n");

    sendcount += npart;
    recvcount += npart;
    sdispls += npart;
    rdispls += npart;
  }
  fclose(f);
}

void show_exinfo(mpk_t *mg) {
  assert(mg != NULL);

#if DETAIL == 0
  return;
#endif

  int npart = mg->npart;
  int nphase = mg->nphase;

  int i;
  double tsum = 0.0;
  int maxth = 0;
  for (i=0; i<= nphase; i++) {
    int j;
    for (j=0; j< npart; j++) {
      task_t *tl = mg->tlist + i * npart + j;
#if DETAIL
      printf("%d %d %d %d %e %e\n", i, j, tl->th,
	     tl->n, tl->t1 - tl->t0, (tl->t1 - tl->t0) / tl->n);
#endif
      tsum += tl->t1 - tl->t0;
      if (maxth < tl->th)
	maxth = tl->th;
    }
  }
  printf("total work %e, avrg %e\n", tsum, tsum/(maxth+1.0));
}

void check_error(double *vv, int n, int nlevel) {
  int i;
  double e = 0.0;
  for (i=0; i< n; i++) {
    double v = vv[n * nlevel + i];
    e += (v - 1.0) * (v - 1.0);
  }
  printf("error %e\n", sqrt(e/n));
}

static int *alloc_fill_count(mpk_t *mg) {
  int *count = malloc(sizeof(*count) * mg->npart);
  assert(count != NULL);
  for (int part = 0; part < mg->npart; part++)
    count[part] = 0;

  for (int phase = 0; phase <= mg->nphase; phase++) {
    for (int part = 0; part < mg->npart; part++) {
      task_t *tl = mg->tlist + phase * mg->npart + part;
      count[part] += tl->n;
    }
  }
  return count;
}

static void recv_copy(comm_data_t *cd, int *count, double *vv) {
  for (int part = 1; part < cd->mg->npart; part++) {
    double *buf = malloc(sizeof(*buf) * count[part]);
    assert(buf != NULL);
    MPI_Recv(buf, count[part], MPI_DOUBLE, part, 0, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
    // PHASE LOOP
    for (int phase = 0; phase <= cd->nphase; phase++) {
      task_t *tl = cd->mg->tlist + phase * cd->npart + part;
      for (int i = 0; i < tl->n; i++) {
        long val = tl->idx[i];
        if (val >= cd->n * (cd->nlevel + 1))
          printf("tl->idx[i]: %ld\n", tl->idx[i]);
        vv[tl->idx[i]] = buf[i];
      }
    }
    free(buf);
  }
}

static void copy_send(comm_data_t *cd, int *count, double *vv) {
  double *buf = malloc(sizeof(*buf) * count[cd->rank]);
  assert(buf != NULL);

  // PHASE LOOP
  for (int phase = 0; phase <= cd->mg->nphase; phase++) {
    task_t *tl = cd->mg->tlist + phase * cd->mg->npart + cd->rank;
    for (int i = 0; i < tl->n; i++)
      buf[i] = vv[tl->idx[i]];
  }

  MPI_Send(buf, count[cd->rank], MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
  free(buf);
}

static void collect_results(comm_data_t *cd, double *vv) {
  int *count = alloc_fill_count(cd->mg);

  if (cd->rank == 0)
    recv_copy(cd, count, vv);
  else
    copy_send(cd, count, vv);

  free(count);
}

static int check_results(comm_data_t *cd, double *vv) {
  int size = cd->n * cd->nlevel;
  for (int i = 0; i < size; i++)
    if (vv[cd->n + i] != 1.0)
      return -1;
  return 0;
};

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s dirname\n", argv[0]);
    exit(1);
  }
  // TODO(vatai): NEXT LIST below
  // NEXT: read_cd
  // NEXT: del mpk_t *mg;

  // Init MPI
  MPI_Init(&argc, &argv);

  mpk_t *mg = read_mpk(argv[1]);
  int n = mg->n;
  int nlevel = mg->nlevel;

  double *vv = (double*) malloc(sizeof(double) * n * (nlevel+1));
  assert(vv != NULL);

  prep_mpk(mg, vv);

  comm_data_t *cd = new_comm_data(mg, argv[1]);

  mpi_prep_mpk(cd);

  int i;
  for (i = 0; i < n; i++)
    vv[i] = 1.0;
  for (i = 0; i < n * nlevel; i++)
    vv[n + i] = -1.0;		/* dummy */
  double min;

  for (i=0; i< n; i++)
    if (mg->plist[0]->part[i] == cd->rank)
      vv[i] = 1.0;
    else
      vv[i] = -100.0;
  for (i=0; i< n * nlevel; i++)
    vv[n + i] = -1.0;		/* dummy */

  // NEW CODE
  for (int i = 0; i < cd->buf_count; i++) {
    cd->vv_buf[i] = -42;
  }
  for (int i = 0; i < cd->rcount[0]; i++) {
    cd->vv_rbufs[0][i] = 1.0;
  }

  mpi_exec_mpk(cd);

  // Copy from cd to vv[]
  for (int i = 0; i < cd->buf_count; i++) {
    vv[cd->idx_buf[i]] = cd->vv_buf[i];
  }

  char fname[1024];
  sprintf(fname, "%s/vv_after_mpi_exec_rank%d.log", argv[1], cd->rank);
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

  int result = 0;
  collect_results(cd, vv);
  if (cd->rank == 0)
    result = check_results(cd, vv);
  del_comm_data(cd);
  free(vv);
  // TODO(vatai): del_mpk(mg); // todos added to lib.h and readmpk.c
  MPI_Finalize();
  return result;
}
