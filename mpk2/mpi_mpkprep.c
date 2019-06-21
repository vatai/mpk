#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mpi.h>

#include "lib.h"

void mpi_prep_mpk(mpk_t *mg, double *vv, double **sbufs, double **rbufs) {
  assert(mg != NULL && vv != NULL);

  printf("checking mpk...");  fflush(stdout);

  int erc = 0;

  int n = mg->n;
  int npart = mg->npart;
  int nlevel = mg->nlevel;
  int nphase = mg->nphase;

  int phase;


  // Loop below to check the error in calculations of levels.
  for (phase = 0; phase < nphase; phase ++) {

    if (phase == 0) {
      level_t *l1 = mg->llist[phase];
      assert(l1 != NULL);

      int i;
      for (i=0; i< n; i++)
	if (0 > l1->level[i]) { // Levels should be >=0
	  fprintf(stderr, "level error at i=%d phase=%d: %d\n",
		  i, phase, l1->level[i]);
	  erc ++;
	}

    } else {
      level_t *l0 = mg->llist[phase-1];
      level_t *l1 = mg->llist[phase];

      assert(l0 != NULL && l1 != NULL);

      int i;
      for (i=0; i< n; i++)
	if (l0->level[i] > l1->level[i]) { // levels should never decrease.
	  fprintf(stderr, "level error at i=%d phase=%d: %d %d\n",
		  i, phase, l0->level[i], l1->level[i]);
	  erc ++;
	}
    }
  }

  // Loop to calculate errors in the levels of skirt
  if (nphase == 0) { // PA1

    int p;
    for (p=0; p< npart; p++) {

      int *sl = mg->sg->levels + p * n; // Levels of skirt of partition p

      int i;
      for (i=0; i< n; i++)
	if (sl[i] >= 0 && 0 > nlevel - sl[i]) { // Cannot have skirt level more than nlevel
	  fprintf(stderr, "skirt level error at i=%d p=%d: %d\n",
		  i, p, nlevel - sl[i]);
	  erc ++;
	}
    }

  } else {

    int p;
    for (p=0; p< npart; p++) {

      int *ll = mg->llist[nphase-1]->level;
      int *sl = mg->sg->levels + p * n;

      int i;
      for (i=0; i< n; i++)
	if (sl[i] >= 0 && ll[i] > nlevel - sl[i]) {
	  fprintf(stderr, "skirt level error at i=%d p=%d: %d %d\n",
		  i, p, ll[i], nlevel - sl[i]);
	  erc ++;
	}
    }
  }

  if (erc > 0) // Proceed only if no errors
    exit(1);

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

  int tcount = 0;
  int *prevl = l0;
  int prevlmin = 0;
  //______________________________________________

  int *numb_of_send = malloc(sizeof(int)*nphase);
  int *numb_of_rec = malloc(sizeof(int)*nphase);
  int **send_counts = malloc(sizeof(int*)*nphase);
  int **recv_counts = malloc(sizeof(int*)*nphase);
  int **send_displ = malloc(sizeof(int*)*nphase);
  int **recv_displ = malloc(sizeof(int*)*nphase);

  for (phase = 0; phase < nphase; phase ++) {
    assert(mg->plist[phase] != NULL);
    pl = mg->plist[phase]->part;

    assert(mg->llist[phase] != NULL);
    int *ll = mg->llist[phase]->level;
    if (phase>0)
    {
    numb_of_send[phase]=0;
    numb_of_rec[phase] = 0;
    //(Utsav) initialize the values to 0
    send_counts[phase] = malloc(sizeof(int)*npart);
    recv_counts[phase] = malloc(sizeof(int)*npart);
    for (int i = 0; i < npart; ++i)
    {
    (*(send_counts[phase]+i))=0;
    (*(recv_counts[phase]+i))=0;
    }

    }
    int tsize[npart];
    for (i = 0; i< npart; i++)
      tsize[i] = 0;

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

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Communication of which vertex (n) from which level (nlevel)
    // from which process/partition (npart) to which process/partition
    // (npart).

    if (phase>0) // No communication for phase = 0
    {
      int *comm_table = malloc(sizeof(*comm_table) * nlevel * n * npart * npart);
    for (i = 0; i < nlevel * n * npart * npart; i++) comm_table[i] = 0;
    }
    

    int l;
    for (l = prevlmin + 1; l <= lmax; l++) { // initially prevlmin =0
      for (i=0; i< n; i++)
	if (pl[i] == rank && prevl[i] < l && l <= ll[i]) { //according to rank
	  if (vv[l*n+i] >= 0) {
	    fprintf(stderr, "already computed: level %d i %d\n", l, i);
	    erc ++;
	  }

	  int j;
	  for (j= g0->ptr[i]; j< g0->ptr[i+1]; j++) { // All neighbours
	    int k = g0->col[j];

	    if (vv[(l-1)*n + k] < 0.0 ||
		(pl[i] != pl[k] && ((int)vv[(l-1)*n+k])%100 == phase)) {

	      fprintf(stderr, "structure error phase %d level %d i %d k %d\n",
		      phase, l, i, k);
	      int r;
	      for (r=0; r< l; r++)
		fprintf(stderr, "%6.0f", vv[r*n+k]);
	      fprintf(stderr, " prevl=%d\n", prevl[k]);
	      erc ++;
	    }

            // MPI code to complete comm_table

            if (phase == 0) {
              // No communication occurs in the initial phase.
              // TODO(vatai): what to do here?
              continue;
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
                //int vv_idx = n * (l - 1) + k; //source vv index
                int src_part = mg->plist[phase - 1]->part[k]; // source partition
                int tgt_part = pl[i]; // target partition
                int from_part_to_part = npart * src_part + tgt_part;
                int idx = nlevel * n * (from_part_to_part)+ n*l + i;
                comm_table[idx]++; // setting it to 1 implying the communication for the corresponding index
                if (src_part==rank)
                {
                  int numb_of_send[phase]++;
                  (*(send_counts[phase]+tgt_part))++;
                }
                if (tgt_part==rank)
                {
                  int numb_of_rec[phase]++;
                  (*(recv_counts[phase]+src_part))++;
                }

              }
            }
	  }

	  vv[l*n + i] = pl[i] * 100.0 + phase;
	  tsize[pl[i]] ++;  // Most important in loop1
	}
    }

    task_t *tl = mg->tlist + phase * npart;
    for (i=0; i< npart; i++) {
      tl[i].n = tsize[i];
      tl[i].idx = mg->idxsrc + tcount;
      tcount += tsize[i];
      assert(tcount <= n * nlevel);
      tsize[i] = 0;		/* re-initialize */
    }

    for (l = prevlmin + 1; l <= lmax; l++)
      for (i=0; i< n; i++)
	if (prevl[i] < l && l <= ll[i])
	  tl[pl[i]].idx[tsize[pl[i]]++] = l*n + i;

    prevl = ll;
    prevlmin = lmin;
  }
  //___________________________________________

  // Following loop is to initialize sbufs 
  // for phase = 0 there is no communication so will remain NULL
  for (phase = 1; i < nphase; ++i)
  {
    sbufs[phase] = malloc(sizeof(int)*numb_of_send[phase]);
    rbufs[phase] = malloc(sizeof(int) *numb_of_rec[phase]);

    send_displ[phase] = malloc(sizeof(int)*npart);
    recv_displ[phase] = malloc(sizeof(int)*npart);
    for (int i = 0; i < npart; ++i)
    {
      if (i==0)
      {
        *(send_displ[phase])=*(send_counts[phase]);
        *(recv_displ[phase])=*(recv_counts[phase]);
        continue;
      }
      *(send_displ[phase])=*(send_displ[phase-1])+ *(send_counts[phase]);
      *(recv_displ[phase])=*(recv_displ[phase-1])+ *(recv_counts[phase]);
    }
    //(Utsav) sbufs to be made here.
    int count = 0;
    for (int i = nlevel*n*npart*rank; i < nlevel*n*npart*(rank+1); ++i)
    { 
      if (comm_table[i]==1)
      {
        sbufs[count]=vv[(i-nlevel*n*npart*rank)%(n*nlevel)];
        count++;
        continue;
      }

    }
  }  

  if (erc > 0)
    exit(1);

  pl = mg->plist[0]->part;
  int *sl = mg->sg->levels;

  int p;
  for (p = 0; p < npart; p++) {
    int cnt = 0;
    task_t *tl = mg->tlist + nphase * npart + p;
    tl->idx = mg->idxsrc + tcount;

    int l;
    for (l = prevlmin + 1; l <= nlevel; l++) {
      for (i=0; i< n; i++)
	if (prevl[i] < l && sl[p*n+i] >= 0 && l <= nlevel - sl[p*n+i]) {
	  if (vv[l*n+i] > 0.0) {
	    fprintf(stderr, "skirt %d %d %d already computed\n", l, i, p);
	    erc ++;
	  }

	  int j;
	  for (j= g0->ptr[i]; j< g0->ptr[i+1]; j++) {
	    int k = g0->col[j];

	    if (vv[(l-1)*n + k] < 0.0) {
	      fprintf(stderr, "skirt error level %d i %d k %d\n",
		      l, i, k);
	      erc ++;
	    }
	  }

	  vv[l*n+i] = pl[i] * 100.0 + nphase;
	  tl->idx[cnt++] = l*n+i;
	}
    }

    for (l = prevlmin + 1; l <= nlevel; l++) {
      for (i=0; i< n; i++)
	if (prevl[i] < l && sl[p*n+i] >= 0 && l <= nlevel - sl[p*n+i])
	  vv[l*n+i] = -0.1;	/* clean up */
    }

    tl->n = cnt;
    tcount += cnt;
  }

  assert(tcount <= mg->idxallocsize && nlevel * n <= tcount);
  printf(" task %f", tcount / (double)(nlevel * n));

  for (i=0; i< n; i++)
    if (vv[nlevel * n + i] < -0.5) {
      fprintf(stderr, "element %d of final level not computed\n", i);
      erc ++;
    }

  if (erc > 0)
    exit(1);

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
