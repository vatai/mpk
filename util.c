#include <stdio.h>
#include <math.h>
#include "lib.h"


void prn_k(level_t *lg, fp_t *b, int k){
  int i,j,n,nn;
  nn = lg->pg->g->n; n = sqrt(nn);
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) 
      printf("%6.1f ", b[nn*k+i*n+ j]);
    printf("\n");
  }
  printf("\n--------\n");
};

void prn_lvl(level_t *lg, fp_t *b, idx_t lvl){
  int i,j,n,nn;
  printf("level: %d\n",lvl);
  nn = lg->pg->g->n; n = sqrt(nn);
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      idx_t ii = i*n+ j;
      if( lg->level[ii] == lvl){
	printf("%6.1f ", b[ii]);
      } else { printf("   *** "); }
    }
    printf("\n");
  }
  printf("\n--------\n");
};

void prn_prt(part_t *pg){
  int i,j,n,nn;
  nn = pg->g->n;
  n = sqrt(nn);
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) 
      printf("%4d ", pg->part [i*n + j]);
    printf("\n");
  }
  printf("\n--------\n");
};

void octave_check(char*fn, fp_t* b, int N, int k){
  int n = sqrt(N);
  FILE* f = fopen(fn,"w");
  fprintf(f,"1;\nn = %d;pde;\n", n);
  //b += N;
  for (int t = 0; t < k; t++) {
    fprintf(f,"rv%d = reshape(A^%d*bb,%d,%d) - [", t, t, n,n);
    for (int i = 0; i < N; i++) {
      if (i % n == 0)     fprintf(f,"\n");
      fprintf(f, "%8.1f,", b[i]);
    }
    fprintf(f,"]; norm(rv%d)\n",t);
    b += N;
  }
  fclose(f);
}

void print_levels(level_t *lg) {
  int n = lg->pg->g->n;
  int i, max, min;
  int *l = lg->level;

  max = min = l[0];
  for (i=1; i< n; i++) {
    if (max < l[i])
      max = l[i];
    if (min > l[i])
      min = l[i];
  }
  
  printf("level min=%d max=%d\n", min, max);
}

void misc_info(level_t* lg, fp_t* bb, idx_t k_steps, int t)
{
  part_t *pg = lg->pg;
  int n = (int) sqrt(pg->g->n);

  for (int i = 1; i < k_steps; i++) prn_k(lg, bb, i);

  printf("\nRound %d\n", t);
  printf("--- Partition ---\n");
  for (int i = 0; i < n; i++){
    for (int j = 0; j < n; j++) printf("%5d, ",pg->part[i*n+j]);
    printf("\n");
  }
  for (int i = 0; i < n; i++){
    for (int j = 0; j < n; j++) printf("%5x, ",lg->partial[i*n+j]);
    printf("\n");
  }
  printf("--- LEVEL ---\n");
  for (int i = 0; i < n; i++){
    for (int j = 0; j < n; j++) printf("%d",lg->level[i*n+j]);
    printf("\n");
  }


}

void prn_part_size(perm_t* pr)
{
  int i;
  int n_part = pr->pg->n_part;
  idx_t *part_size = pr->part_start;
  printf("part_size[]");
  for (i = 0; i < n_part; i++) printf("%d ", part_size[i]);
  printf("\n");
}
