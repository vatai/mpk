#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#include "comm_data.h"
#include "buffers.h"

#define DETAIL 0
#define ONEVEC 0
#define ONEENT 0
#define TRANS 0

/*
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
*/

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

static void recv_copy_results(buffers_t *bufs, double *vv) {
  for (int part = 1; part < bufs->npart; part++){
    int buf_count;
    MPI_Recv(&buf_count, 1, MPI_INT, part, 0, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
    long *idx_buf = malloc(sizeof(*idx_buf) * buf_count);
    double *vv_buf = malloc(sizeof(*idx_buf) * buf_count);
    MPI_Recv(idx_buf, buf_count, MPI_LONG, part, 0, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
    MPI_Recv(vv_buf, buf_count, MPI_DOUBLE, part, 0, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
    for (int i = 0; i < buf_count; i++)
      vv[idx_buf[i]] = vv_buf[i];
    free(idx_buf);
    free(vv_buf);
  }
}

static void send_results(buffers_t *bufs, double *vv) {
  MPI_Send(&bufs->buf_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  MPI_Send(bufs->idx_buf, bufs->buf_count, MPI_LONG, 0, 0, MPI_COMM_WORLD);
  MPI_Send(bufs->vv_buf, bufs->buf_count, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
}

static int check_results(buffers_t *bufs, double *vv) {
  if (bufs->rank == 0) {
    recv_copy_results(bufs, vv);
    int size = bufs->n * bufs->nlevel;
    for (int i = 0; i < size; i++)
      if (vv[bufs->n + i] != 1.0)
        return -1;
    return 0;
  } else {
    send_results(bufs, vv);
    return 0;
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s dirname\n", argv[0]);
    exit(1);
  }

  // Init MPI
  MPI_Init(&argc, &argv);

  comm_data_t *cd = new_comm_data(argv[1]);
  buffers_t *bufs = new_bufs(cd);
  fill_buffers(cd, bufs);

  double *vv = (double*) malloc(sizeof(double) * cd->n * (cd->nlevel+1));
  assert(vv != NULL);

  // NEW CODE
  for (int i = 0; i < bufs->buf_count; i++) {
    bufs->vv_buf[i] = -42;
  }
  for (int i = 0; i < bufs->rcount[0]; i++) {
    bufs->vv_rbufs[0][i] = 1.0;
  }

  mpi_exec_mpk(bufs);

  // Copy from buf to vv[]
  for (int i = 0; i < bufs->buf_count; i++) {
    vv[bufs->idx_buf[i]] = bufs->vv_buf[i];
  }

  int result = check_results(bufs, vv);

  del_comm_data(cd);
  del_bufs(bufs);

  free(vv);
  MPI_Finalize();
  return result;
}
