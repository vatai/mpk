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

void test_allltoall_inputs(mpk_t *mg, double *vv, double **sbufs, double **rbufs,
                  int **idx_sbufs, int **idx_rbufs,
                  int *sendcount, int *recvcount,
                  int *sdispls, int* rdispls){
  
  printf("testing all inputs and printing out_mpi_alltoall:\n");
  int n = mg -> n;
  int npart = mg->npart;
  int rank;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  char name[100];
  sprintf(name,"%d_out_mpi_alltoall",rank);
  FILE *f = fopen(name, "w");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", name);
    exit(1);
  }

  fprintf(f, "vv:\n");
  for (int i = 0; i < (mg->n)*(mg->nlevel); ++i){
    fprintf(f, " %f",vv[i] );
    if ((i+1)%n==0){
      fprintf(f, "\n");
    }
  }

  sendcount += npart;
  recvcount += npart;
  sdispls += npart;
  rdispls += npart;
  for (int phase  = 1; phase < mg->nphase; ++phase){
    fprintf(f, "Phase:%d\n",phase);
    fprintf(f, "sbufs-\n");    
    for(int i = 0; i< sendcount[npart-1]+sdispls[npart-2];i++){
      fprintf(f, " %f",sbufs[phase][i]);
    }
    fprintf(f, "\n");
    fprintf(f, "idx_sbufs-\n");
    for(int i = 0; i< sendcount[npart-1]+sdispls[npart-2];i++){
      fprintf(f, " %d",idx_sbufs[phase][i]);
    }
    fprintf(f, "\n");
    fprintf(f, "sendcount-\n");
    for(int i = 0; i< npart;i++){
      fprintf(f, " %d",sendcount[i]);
    }
    fprintf(f, "\n");
    fprintf(f, "recvcount-\n");
    for(int i = 0; i< npart;i++){
      fprintf(f, " %d",recvcount[i]);
    }
    fprintf(f, "\n");
    fprintf(f, "sdispls-\n");
    for(int i = 0; i< npart;i++){
      fprintf(f, " %d",sdispls[i]);
    }
    fprintf(f, "\n");
    fprintf(f, "rdispls-\n");
    for(int i = 0; i< npart;i++){
      fprintf(f, " %d",rdispls[i]);
    }
    fprintf(f, "\n");
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
  int world_rank, world_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  mpk_t *mg = read_mpk(argv[1]); // *******

  int n = mg->n;
  int nlevel = mg->nlevel;

  // vv stores the vector (vertices) at different levels
  double *vv = (double*) malloc(sizeof(double) * n * (nlevel+1));
  assert(vv != NULL);

  prep_mpk(mg, vv); //****** // Verify if the input data has correct structure and report error if not and prepare communication matrix

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

  // Send and receive buffers for vertex values (from vv).
  double **vv_sbufs = malloc(sizeof(*vv_sbufs) * mg->nphase);
  double **vv_rbufs = malloc(sizeof(*vv_rbufs) * mg->nphase);
  // Send and receive buffers for (vv) indices.
  int **idx_sbufs = malloc(sizeof(*idx_sbufs) * mg->nphase);
  int **idx_rbufs = malloc(sizeof(*idx_rbufs) * mg->nphase);

  // `sendcounts[phase * npart + p]` and `recvcounts[phase * npart +
  // p]` are is the number of elements sent/received to/from partition
  // `p`.
  int *sendcounts = malloc(sizeof(*sendcounts) * mg->npart * mg->nphase);
  int *recvcounts = malloc(sizeof(*recvcounts) * mg->npart * mg->nphase);

  // `sdispls[phase * npart + p]` and `rsdispls[phase * npart + p]` is
  // the displacement (index) in the send/receive buffers where the
  // elements sent to partition/process `p` start.
  int *sdispls = malloc(sizeof(*sdispls) * mg->npart * mg->nphase);
  int *rdispls = malloc(sizeof(*rdispls) * mg->npart * mg->nphase);

  mpi_prep_mpk(mg, vv, vv_sbufs, vv_rbufs, idx_sbufs, idx_rbufs,
               sendcounts, recvcounts, sdispls, rdispls);

  //test_allltoall_inputs(mg, vv, vv_sbufs, vv_rbufs, idx_sbufs, idx_rbufs,
  //             sendcounts, recvcounts, sdispls, rdispls);

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

  int nth;
  for (nth = 1; nth <= mg->npart; nth ++) { // spmv multiplication (parallel) is done and minnimum time is reported
    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      spmv_exec_par(mg->g0, vv, nlevel, nth);
      double t1 = omp_get_wtime();

      //      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("spmv nth= %d time= %e\n", nth, min);
    check_error(vv, n, nlevel);
  }

  /********************************************************************/
#if ONEVEC
  printf("force to refer only one element\n");
  for (i=0; i< mg->g0->ptr[n+1]; i++)
    mg->g0->col[i] = 0;
#endif

#if ONEENT
  printf("force to compute only one element\n");
  for (i=0; i< mg->npart * (mg->nphase+1); i++) {
    int j;
    for (j=0; j< mg->tlist[i].n; j++)
      mg->tlist[i].idx[j] = n;
  }
#endif
  /********************************************************************/


  for (nth = 1; nth <= mg->npart; nth ++) {
#if 1
    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      /* exec_mpk_mpi(mg, vv, nth,vv_sbufs, vv_rbufs, idx_sbufs, idx_rbufs, */
      /*          sendcounts, recvcounts, sdispls, rdispls ); */
      double t1 = omp_get_wtime();

      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("mpk nth= %d time= %e\n", nth, min);
    check_error(vv, n, nlevel);
#else
    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      exec_mpk_xs(mg, vv, nth);
      double t1 = omp_get_wtime();

      //      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("xs nth= %d time= %e\n", nth, min);

    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      exec_mpk_xd(mg, vv, nth);
      double t1 = omp_get_wtime();

      //      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("xd nth= %d time= %e\n", nth, min);

    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      exec_mpk_is(mg, vv, nth);
      double t1 = omp_get_wtime();

      //      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("is nth= %d time= %e\n", nth, min);

    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      exec_mpk_id(mg, vv, nth);
      double t1 = omp_get_wtime();

      //      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("id nth= %d time= %e\n", nth, min);
#endif
  }

#if TRANS
  for (i=0; i< n * (nlevel+1); i++)
    vv[i] = -1.0;		/* dummy */
  for (i=0; i< n; i++)
    vv[i * (nlevel+1)] = 1.0;

  for (nth = 1; nth <= mg->npart; nth ++) {
    for (i=0; i< 5; i++) {
      double t0 = omp_get_wtime();
      exec_mpkt(mg, vv, nth);
      double t1 = omp_get_wtime();

      //      show_exinfo(mg);
      if (i == 0 || min > t1 - t0)
	min = t1 - t0;
    }
    printf("mpkt nth= %d time= %e\n", nth, min);
  }
#endif

  MPI_Finalize();
  return 0;
}
