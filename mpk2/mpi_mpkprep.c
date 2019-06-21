#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mpi.h>

#include "lib.h"

int get_ct_idx(int n, int nlevel, int npart, int src_part, int tgt_part, int vv_idx){
  int from_part_to_part = npart * src_part + tgt_part;
  return nlevel * n * from_part_to_part + vv_idx;
}

void mpi_prep_mpk(mpk_t *mg, double *vv, double **sbufs, double **rbufs,
                  int **idx_sbufs, int **idx_rbufs,
                  int *sendcount, int *recvcount,
                  int *sdispls, int* rdispls) {
  assert(mg != NULL && vv != NULL);

  printf("preparing mpi buffers for communication...");  fflush(stdout);

  int n = mg->n;
  int npart = mg->npart;
  int nlevel = mg->nlevel;
  int nphase = mg->nphase;
  int phase;


  crs0_t *g0 = mg->g0;
  assert(g0 != NULL);

  int i;
  for (i=0; i< n * (nlevel+1); i++)
    vv[i] = -1.0;

  assert(mg->plist[0] != NULL);
  int *pl = mg->plist[0]->part;

  for (i=0; i< n; i++)
    vv[i] = pl[i] * 100.0;

  int l0[n];
  for (i=0; i< n; i++)
    l0[i] = 0;

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int tcount = 0;
  int *prevl = l0;
  int prevlmin = 0;
  // rcount and scount are pointers to the first element of the
  // recvcount and sendcount for the current phase.
  int *rcount = recvcount;
  int *scount = sendcount;
  // Similar to s/rcount.
  int *rdisp = rdispls;
  int *sdisp = sdispls;

  int *comm_table = malloc(sizeof(*comm_table) * nlevel * n * npart * npart);
  //______________________________________________
  for (phase = 0; phase < nphase; phase ++) {
    assert(mg->plist[phase] != NULL);
    pl = mg->plist[phase]->part;

    assert(mg->llist[phase] != NULL);
    int *ll = mg->llist[phase]->level;

    int lmin, lmax;
    lmin = lmax = ll[0];
    for (i=1; i< n; i++) { // give max and min value to lmax and lmin resp.
      if (lmin > ll[i])
	lmin = ll[i];
      if (lmax < ll[i])
	lmax = ll[i];
    }

    if (lmax > nlevel)
      lmax = nlevel;

    // Communication of which vertex (n) from which level (nlevel)
    // from which process/partition (npart) to which process/partition
    // (npart).
    for (i = 0; i < nlevel * n * npart * npart; i++) comm_table[i] = 0;

    int l;
    for (l = prevlmin + 1; l <= lmax; l++) { // initially prevlmin =0
      for (i=0; i< n; i++)
      	if (pl[i] == rank && prevl[i] < l && l <= ll[i]) { //according to rank

         int j;
      	  for (j= g0->ptr[i]; j< g0->ptr[i+1]; j++) { // All neighbours
           int k = g0->col[j];


            // MPI code to complete comm_table

           if (phase == 0) {
              // No communication occurs in the initial phase.
              // TODO(vatai): what to do here?
           } else {
              // Communicate after the initial phase, if the following
              // 2 conditions are met for the adjacent vertex
              // (i.e. the "source" vertex needed to compute vv[n * l
              // + i])
              // The source vertex is in a different partition than
              // the target vertex
            int is_diff_part = (mg->plist[phase - 1]->part[k] != pl[i]);
              // The vertex is computed, i.e. just before the target
              // level.
            int is_computed = (prevl[k] >= l - 1);
            if (is_diff_part && is_computed) {
                int vv_idx = n * (l - 1) + k; //source vv index
                int src_part = mg->plist[phase - 1]->part[k]; // source partition
                int tgt_part = pl[i]; // target partition
                int idx = get_ct_idx(n, nlevel, npart, src_part, tgt_part, vv_idx);
                comm_table[idx]++; // setting it to 1 implying the communication for the corresponding index
              }
            }
          }
        }
      }
    int numb_of_send = 0;
    int numb_of_rec = 0;
    // For all "other" partitions `p` (other = other partitions we are
    // sending to, and other partitions we are receiving from).
    for (i = 0; i < npart; i++) {
      rcount[i] = 0;
      scount[i] = 0;
    }
    for (int p = 0; p < npart; ++p) {
      // For all vv indices.
      for (int i = 0; i < nlevel*n; ++i){
        // Here p is the source (from) partition.
        int idx = get_ct_idx(n, nlevel, npart, p, rank, i);
        if (comm_table[idx])
        {
          // So we increment:
          numb_of_rec++;
          rcount[p]++;
        }
        // Here `p` is the target (to) partition.
        idx = get_ct_idx(n, nlevel, npart, rank, p, i);
        if (comm_table[idx])
        {
          // So we increment:
          numb_of_send++;
          scount[p]++;
        }
      }
    }
    sbufs[phase] = malloc(sizeof(*sbufs[phase]) * numb_of_send);
    rbufs[phase] = malloc(sizeof(*rbufs[phase]) * numb_of_rec);
    idx_sbufs[phase] = malloc(sizeof(*idx_sbufs[phase]) * numb_of_send);
    idx_rbufs[phase] = malloc(sizeof(*idx_rbufs[phase]) * numb_of_rec);

    // Do a scan on rdisp/sdisp.


    // Prepare for the next phase.
    prevl = ll;
    prevlmin = lmin;
    rcount += npart;
    scount += npart;
    rdisp += npart;
    sdisp += npart;
  }
  //_____________________________________________


  // Following loop is to initialize sbufs 
  // for phase = 0 there is no communication so will remain NULL
  for (phase = 1; i < nphase; ++i)
  {

    for (int i = 0; i < npart; ++i)
    {
    /*   if (i==0) */
    /*   { */
    /*     *(send_displ[phase])=*(send_counts[phase]); */
    /*     *(recv_displ[phase])=*(recv_counts[phase]); */
    /*   } */
    /*   else{ */
    /*   *(send_displ[phase])=*(send_displ[phase-1])+ *(send_counts[phase]); */
    /*   *(recv_displ[phase])=*(recv_displ[phase-1])+ *(recv_counts[phase]); */
    /* } */
    }
    //(Utsav) sbufs to be made here.
    int count = 0;
    for (int i = nlevel*n*npart*rank; i < nlevel*n*npart*(rank+1); ++i)
    { 
      if (comm_table[i])
      {
        // sbufs[count] = vv[(i-nlevel*n*npart*rank)%(n*nlevel)];
        count++;
      }

    }
  }


  pl = mg->plist[0]->part;
  int *sl = mg->sg->levels;


  assert(tcount <= mg->idxallocsize && nlevel * n <= tcount);
  printf(" task %f", tcount / (double)(nlevel * n));



  printf(" done\n");

#if 0
  printf("size of tasks ...\n");
  for (i = 0; i <= nphase; i++) {
    int p;
    for (p=0; p< npart; p++)
      printf("%5d", mg->tlist[i*npart+p].n);
    printf("\n");
  }
#endif
}
