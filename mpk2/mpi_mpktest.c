#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#include "lib.h"
#include "mpi_lib.h"

#define DETAIL 0
#define ONEVEC 0
#define ONEENT 0
#define TRANS 0
#define LOGFILE 1

void print_time(char *dir, double mpi_exectime, double spmvmintime) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  char name[100];
  sprintf(name,"%s/%d_time.log",dir,rank);
  FILE *f = fopen(name, "w");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", name);
    exit(1);
  }
  fprintf(f, "spmvmintime = %lf and mpi_exectime = %lf\n",spmvmintime, mpi_exectime );
  fclose(f);
}

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

int main(int argc, char* argv[]) {
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

  // verify if the input data has correct structure and report error
  // if not and prepare communication matrix.
  prep_mpk(mg, vv);

  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  printf("world_size: %d, nphase: %d\n", world_size, mg->npart);
  assert(world_size == mg->npart);

  // Every phase needs its own buffer.
  // It can be overwritten over the same memory later.
  // (Utsav) Do we need to change the name ?

  // int MPI_Alltoallv(const void *sendbuf, const int *sendcounts,
  //                   const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
  //                   const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
  //                   MPI_Comm comm)

  // Each phase communicates a different amount, hence pointer to
  // pointer.  E.g. `vv_sbufs[phase][i]` is the `i`-th element of the
  // send buffer in phase `p`

  comm_data_t cd;
  mpi_prep_mpk(mg, &cd);

  #if LOGFILE
    test_allltoall_inputs(&cd);
  #endif

  int i;
  for (i = 0; i < n; i++) vv[i] = 1.0;
  for (i = 0; i < n * nlevel; i++) vv[n + i] = -1.0;		/* dummy */

  // SpMV multiplication (sequential) is carried out and minimum time
  // is reported.
  double start, end;
  double spmvmintime = 0;
  // TODO(Utsav): Do we need barrier ? //MPI_Barrier(MPI_COMM_WORLD);

  for (int i = 0; i < 5; ++i) {
    start = MPI_Wtime();
    spmv_exec_seq(mg->g0, vv, nlevel);
    end = MPI_Wtime();
    if (i==0)
      spmvmintime = end-start;
    else if (spmvmintime < end-start)
      spmvmintime = end-start;
  }

  // TODO(vatai): double t1 = omp_get_wtime();
  // if (i == 0 || min > t1 - t0)
  //   min = t1 - t0;

  // printf("seq spmv time= %e\n", min);
  check_error(vv, n, nlevel);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  for (i = 0; i < n; i++)
    if (mg->plist[0]->part[i] == rank)
      vv[i] = 1.0;
    else
      vv[i] = -100.0;
  for (i = 0; i < n * nlevel; i++)
    vv[n + i] = -1.0; /* dummy */

  double mpi_exectime = 0;
  for (int i = 0; i < 5; ++i) {
    start = MPI_Wtime();
    mpi_exec_mpk(mg, vv, &cd, argv[1]);
    end = MPI_Wtime();
    if (i == 0)
      mpi_exectime = end - start;
    else if (mpi_exectime < end - start)
      mpi_exectime = end - start;
  }

  print_time(argv[1], mpi_exectime, spmvmintime);

#if LOGFILE
  char fname[1024];
  sprintf(fname, "%s/vv_after_mpi_exec_rank%d.log", argv[1], rank);
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
#endif
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
