#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "lib.h"
#include "util.h"


/**
 * Example ./komp data 4 4 10
 */
 

int main(int argc, char **argv) {
  
  // process argv
  if (argc != 5 ) {
    fprintf(stderr, "usage: %s graph num_part num_iter k_steps\n",
	    argv[0]);
    exit(1);
  }

  idx_t num_part;
  sscanf(argv[2], "%d", &num_part);

  idx_t num_iter;
  sscanf(argv[3], "%d", &num_iter);

  idx_t k_steps;
  sscanf(argv[4], "%d", &k_steps);
  
  // read graph
  FILE *fi = fopen(argv[1], "r");
  if (fi == NULL) {
    fprintf(stderr, "cannot open %s\n", argv[2]);
    exit(1);
  }

  crs0_t *g = read_crs(fi);
  g->diag = 4; g->other = -1;

  part_t *pg = new_part(g);
  level_t *lg = new_level(pg);
  wcrs_t *wg = new_wcrs(g);

  fp_t *b = new_bvect(g->n);
  fp_t *bb = calloc(g->n * k_steps, sizeof(*bb)); // done
  memcpy(bb, b, g->n * sizeof(*bb) );

  /* Partition and write ITER0 */
  partition(pg,num_part); /// !!!!
  perm_t *pr = new_perm(lg);
  
  // prn_lvl(lg, bb, 0);
  for (int t = 0; t < num_iter; t++) {
    iwrite("part", argv[1], t, (void*)pg);
    permutation(pr, &bb, k_steps);
    mpk2(k_steps, lg, bb); /* MPK !!!!  */
    //misc_info(lg, bb, k_steps, t);
    level2wcrs(lg, wg); //// !!!!
    wpartition(pg, wg); //// !!!!
  }

  inverse_permutation(pr, &bb, k_steps);
  
  octave_check("check.m", bb, g->n, k_steps);
  del_wcrs(wg);
  del_level(lg);
  del_part(pg);
  del_crs(g);
  del_perm(pr);
  free(bb);
  free(b);

  return 0;
}
