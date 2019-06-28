#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>
#include <omp.h>

#include "lib.h"

#define DETAIL 0
#define ONEVEC 0
#define ONEENT 0
#define TRANS 0

void test_allltoall_inputs(comm_data_t *cd) {
  printf("testing all inputs and printing out_mpi_alltoall:\n");
  int n = cd->n;
  int npart = cd->npart;

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  char name[100];
  sprintf(name,"%d_out_mpi_alltoall.log",rank);
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

int main(int argc, char* argv[]){
  if (argc != 2) {
    fprintf(stderr, "usage: %s dirname\n", argv[0]);
    exit(1);
  }

  // Init MPI
  MPI_Init(&argc, &argv);

  mpk_t *mg = read_mpk(argv[1]); // *******

  int n = mg->n;
  int nlevel = mg->nlevel;

  // vv stores the vector (vertices) at different levels
  double *vv = (double*) malloc(sizeof(double) * n * (nlevel+1));
  assert(vv != NULL);

  prep_mpk(mg, vv); //****** // Verify if the input data has correct structure and report error if not and prepare communication matrix

  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  printf("world_size: %d, nphase: %d\n", world_size, mg->npart);
  assert(world_size == mg->npart);

  //Every phase needs its own buffer.
  // It can be overwritten over the same memory later.
  //(Utsav) Do we need to change the name ?

  // int MPI_Alltoallv(const void *sendbuf, const int *sendcounts,
  //                   const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
  //                   const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
  //                   MPI_Comm comm)

  // Each phase communicates a different amount, hence pointer to
  // pointer.  E.g. `vv_sbufs[phase][i]` is the `i`-th element of the
  // send buffer in phase `p`

  comm_data_t cd;
  mpi_prep_mpk(mg, vv, &cd);

  test_allltoall_inputs(&cd);

  int i;
  for (i=0; i< n; i++)
    vv[i] = 1.0;
  for (i=0; i< n * nlevel; i++)
    vv[n + i] = -1.0;		/* dummy */

  double min;
  for (i=0; i< 5; i++) { // spmv multipliaction(sequential) is carried out and minnimum time is reported
    double t0 = omp_get_wtime();
    spmv_exec_seq(mg->g0, vv, nlevel);
    double t1 = omp_get_wtime();
    if (i == 0 || min > t1 - t0)
      min = t1 - t0;
  }
  printf("seq spmv time= %e\n", min);
  check_error(vv, n, nlevel);

  // for (i = 0; i < 5; i++) {
  // double t0 = omp_get_wtime();
  mpi_exec_mpk(mg, vv, &cd);
  // double t1 = omp_get_wtime();

  // show_exinfo(mg);
  // if (i == 0 || min > t1 - t0)
  // min = t1 - t0;
  // }
  // printf("spmv nth= %d time= %e\n", nth, min);
  // check_error(vv, n, nlevel);

  mpi_del_cd(&cd);
  free(vv);
  // TODO(vatai): del_mpk(mg); // todos added to lib.h and readmpk.c
  MPI_Finalize();
  return 0;
}
