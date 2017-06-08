#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include "lib.h"

crs0_t *new_crs(int nv, int ne) {
  crs0_t *g = (crs0_t*) malloc(sizeof(crs0_t));
  assert(g != NULL);

  g->n = nv;
  g->ptr = (int*) malloc(sizeof(int) * (nv + 1));
  g->col = (int*) malloc(sizeof(int) * 2 * ne);
  assert(g->ptr != NULL);
  assert(g->col != NULL);

  return g;
}

void del_crs(crs0_t *g) {
  assert(g != NULL);

  assert(g->ptr != NULL);
  free(g->ptr);
  g->ptr = NULL;

  assert(g->col != NULL);
  free(g->col);
  g->col = NULL;

  free(g);
}

crs0_t* read_crs(FILE *f) {
  char line[10240];

  fgets(line, 10240, f);

  int nv, ne, p, q, i;
  i = sscanf(line, "%d %d %d %d", &nv, &ne, &p, &q);
  if (i != 2) {
    fprintf(stderr, "read_crs: format %d not implemented yet\n", i);
    exit(1);
  }

  if (nv <= 0 || ne <= 0) {
    fprintf(stderr, "read_crs: nv=%d ne=%d\n", nv, ne);
    exit(1);
  }

  crs0_t *g = new_crs(nv, ne);
  assert(g != NULL);

  i = 0; 			/* row index */
  p = 0;			/* pointer index */
  g->ptr[0] = 0;
  while (! feof(f)) {
    line[0] = '\0';
    fgets(line, 10240, f);

    if (feof(f) && line[0] == '\0')
      break;

    char *s = line;
    while (s != NULL) {
      while (*s <= ' ' && *s != '\0')
	s ++;
      if (*s == '\0')
	break;
      if (p >= ne * 2) {
	fprintf(stderr, "too many edges in crs (%d %d)\n", i, p);
	exit(1);
      }
      if (sscanf(s, "%d ", &g->col[p]) == 0)
	break;
      g->col[p] --;
      p ++;
      while (isdigit(*s))
	s ++;
    }

    g->ptr[++i] = p;
    if (i == nv)
      break;
  }

  fclose(f);

  return g;
}

void write_crs(FILE *f, crs0_t *g) {
  assert(f != NULL);
  assert(g != NULL);

  int n = g->n;
  fprintf(f, "%d %d\n", n, g->ptr[n]/2);

  int i, j;
  for (i=0; i< n; i++) {
    for (j= g->ptr[i]; j< g->ptr[i+1]; j++)
      fprintf(f, "%d ", g->col[j] + 1);
    fprintf(f, "\n");
  }

  fclose(f);
}

int minplus(crs0_t *g, int *x, int *y) {
  int i, j, k=0;
  for (i=0; i< g->n; i++) {
    int t = x[i];
    for (j= g->ptr[i]; j< g->ptr[i+1]; j++) {
      int s = x[g->col[j]] + 1;
      if (s < t) {
	t = s;
	k++;
      }
    }
    y[i] = t;
  }

  return k;
}

int minplus_r(crs0_t *g, int *x, int *y, int r, int *part) {
  int i, j, k=0;
  for (i=0; i< g->n; i++)
    if (part[i] == r) { // r is the partition number
      int t = x[i];
      for (j= g->ptr[i]; j< g->ptr[i+1]; j++) { // for neighbors
	int s = x[g->col[j]] + 1; // level of neighbor +1
	if (s < t) {
	  t = s;
	  k++;
	}
      }
      y[i] = t; // y the old level
    }

  return k;
}

int minplus_x(crs0_t *g, int *x, int *y, int r, int *part) {
  int i, j, k=0;
  for (i=0; i< g->n; i++)
    if (part[i] != r) {
      int t = x[i];
      for (j= g->ptr[i]; j< g->ptr[i+1]; j++) {
	int s = x[g->col[j]] + 1;
	if (s < t) {
	  t = s;
	  k ++;
	}
      }
      y[i] = t;
    }

  return k;
}

coord_t *new_coord(crs0_t *g) {
  assert(g != NULL);

  coord_t *cg = (coord_t*) malloc(sizeof(coord_t));
  assert(cg != NULL);

  cg->g = g;
  cg->x = (double*) malloc(sizeof(double) * g->n);
  cg->y = (double*) malloc(sizeof(double) * g->n);
  assert(cg->x != NULL);
  assert(cg->y != NULL);

  return cg;
}

void read_coord(FILE *f, coord_t *cg) {
  assert(f != NULL);
  assert(cg != NULL);

  int i, n = cg->g->n;
  for (i=0; i< n; i++)
    fscanf(f, "%lf %lf\n", &cg->x[i], &cg->y[i]);
  fclose(f);
}

part_t *new_part(crs0_t *g) {
  assert(g != NULL);

  part_t *pg = (part_t*) malloc(sizeof(part_t));
  assert(pg != NULL);

  pg->g = g;
  pg->n_part = -1;
  pg->part = (int*) malloc(sizeof(int) * g->n);
  assert(pg->part != NULL);

  return pg;
}

void del_part(part_t *pg) {
  assert(pg != NULL);

  assert(pg->part != NULL);
  free(pg->part);
  pg->part = NULL;

  pg->n_part = -1;
  free(pg);
}

void read_part(FILE *f, part_t *pg) {
  int i, max=0;
  for (i=0; i< pg->g->n; i++) {
    int p;
    fscanf(f, "%d", &p);
    if (max < p)
      max = p;
    pg->part[i] = p;
  }

  pg->n_part = max + 1;
}

void write_part_c(FILE *f, part_t *pg, coord_t *cg) {
  assert(f != NULL);
  assert(pg != NULL);
  assert(cg != NULL);

  int n = pg->g->n;

  int j;
  for (j=0; j< n; j++) {
    if (j > 0 && cg->x[j] != cg->x[j-1])
      fprintf(f, "\n");
    fprintf(f, "%e %e %d\n", cg->x[j], cg->y[j], pg->part[j]);
  }
  fclose(f);
}

level_t *new_level(part_t *pg) {
  assert(pg != NULL);

  level_t *lg = (level_t*) malloc(sizeof(level_t));
  assert(lg != NULL);

  lg->pg = pg;
  lg->level = calloc(pg->g->n, sizeof(*lg->level));
  assert(lg->level != NULL);

  lg->partial = calloc(pg->g->n, sizeof(*lg->partial));
  assert(lg->partial != NULL);

  return lg;
}

void del_level(level_t* lg) {
  assert(lg != NULL);

  assert(lg->level != NULL);
  free(lg->level);
  lg->level = NULL;

  assert(lg->partial != NULL);
  free(lg->partial);
  lg->partial = NULL;
  free(lg);
}

void comp_level(level_t *lg) {
  assert(lg != NULL);
  int n = lg->pg->g->n;
  int np = lg->pg->n_part;
  int *p = lg->pg->part;
  int *l = lg->level;

  // two levels
  int *t0 = (int*) malloc(sizeof(int) * n);
  int *t1 = (int*) malloc(sizeof(int) * n);
  assert(t0 != NULL && t1 != NULL);

  int i;
  for (i = 0; i< np; i++) {
    int j;
    for (j=0; j< n; j++)
      if (p[j] == i)
	t0[j] = t1[j] = n+1;		/* big enough */
      else
	t0[j] = t1[j] = -1;

    do {
      j = minplus_r(lg->pg->g, t0, t1, i, p);
      int *tt = t0;  t0 = t1;  t1 = tt;
    } while (j > 0);

    for (j=0; j< n; j++)
      if (p[j] == i)
	l[j] = t1[j];
  }

  free(t1);  free(t0);
}

void update_level(level_t *lg, level_t *lg_org) {
  assert(lg != NULL);
  assert(lg_org != NULL);
  assert(lg->pg->g == lg_org->pg->g);
  int n = lg->pg->g->n;
  int np = lg->pg->n_part;
  int *p = lg->pg->part;
  int *l = lg->level;
  int *l_org = lg_org->level;

  int *t0 = (int*) malloc(sizeof(int) * n);
  int *t1 = (int*) malloc(sizeof(int) * n);
  assert(t0 != NULL && t1 != NULL);

  int max = l_org[0];
  int i;
  for (i = 1; i< n; i++)
    if (max < l_org[i])
      max = l_org[i];

  for (i = 0; i < np; i++) {
    int j;
    for (j = 0; j < n; j++)
      if (p[j] == i)
	t0[j] = t1[j] = max + n + 1;	/* big enough */
      else
	t0[j] = t1[j] = l_org[j];
    
    do {
      j = minplus_r(lg->pg->g, t0, t1, i, p);
      int *tt = t0;  t0 = t1;  t1 = tt;
    } while (j > 0);

    for (j=0; j< n; j++)
      if (p[j] == i)
	l[j] = t1[j];
  }

  free(t1);  free(t0);
}

void read_level(FILE *f, level_t *lg) {
  assert(f != NULL);
  assert(lg != NULL);

  int n = lg->pg->g->n;
  int i;
  for (i=0; i< n; i++)
    fscanf(f, "%d\n", &lg->level[i]);
  fclose(f);
}

void write_level(FILE *f, level_t *lg) {
  assert(f != NULL);
  assert(lg != NULL);

  int n = lg->pg->g->n;
  int i;
  for (i=0; i< n; i++)
    fprintf(f, "%d\n", lg->level[i]);
  fclose(f);
}

void write_level_c(FILE *f, level_t *lg, coord_t *cg) {
  assert(f != NULL);
  assert(lg != NULL);

  int n = lg->pg->g->n;
  int i;
  for (i=0; i< n; i++) {
    if (i > 0 && cg->x[i] != cg->x[i-1])
      fprintf(f, "\n");
    fprintf(f, "%e %e %d\n", cg->x[i], cg->y[i], lg->level[i]);
  }
  fclose(f);
}

wcrs_t *new_wcrs(crs0_t *g) {
  assert(g != NULL);

  wcrs_t *wg = (wcrs_t*) malloc(sizeof(*wg));
  assert(wg != NULL);

  wg->g = g;
  wg->wv = (int*) malloc(sizeof(*wg->wv) * g->n);
  wg->we = (int*) malloc(sizeof(*wg->we) * 2 * g->ptr[g->n]);

  assert(wg->wv != NULL);
  assert(wg->we != NULL);

  return wg;
}

void del_wcrs(wcrs_t *wg) {
  assert(wg != NULL);

  assert(wg->we != NULL);
  free(wg->we);
  wg->we = NULL;

  assert(wg->wv != NULL);
  free(wg->wv);
  wg->wv = NULL;

  free(wg);
}

void write_wcrs(FILE *f, wcrs_t *wg) {
  assert(f != NULL);
  assert(wg != NULL);

  crs0_t *g = wg->g;
  int n = g->n;

  fprintf(f, "%d %d 011\n", n, g->ptr[n]/2);
  int i, j;
  for (i=0; i< n; i++) {
    fprintf(f, "1 "); // ("1 %d", wg->wv[i]);
    for (j= g->ptr[i]; j< g->ptr[i+1]; j++) {
      fprintf(f, "%d ", g->col[j] + 1);
      fprintf(f, "%d ", wg->we[j]);
    }
    fprintf(f, "\n");
  }

  fclose(f);
}

void level2wcrs(level_t *lg, wcrs_t *wg) {
  assert(lg != NULL);
  assert(wg != NULL);
  assert(lg->pg->g == wg->g);

  int max, min;
  int n = wg->g->n;
  int i, j;

  max = min = lg->level[0];
  for (i=1; i< n; i++) {
    if (max < lg->level[i])
      max = lg->level[i];
    if (min > lg->level[i])
      min = lg->level[i];
  }

  for (i=0; i< n; i++) {
    wg->wv[i] = max + 1 - lg->level[i];

    int l0 = lg->level[i];
    for (j = wg->g->ptr[i]; j< wg->g->ptr[i+1]; j++) {
      int l1 = lg->level[wg->g->col[j]];
      double w = l0 + l1 - 2 * min;
      w = 1e+6 / (w + 1);
      if (w < 1.0)
	wg->we[j] = 1;
      else
	wg->we[j] = w;
    }
  }
}

perm_t *new_perm(level_t *lg)
{
  perm_t *pr = (perm_t*) malloc(sizeof(*pr));
  assert(pr != NULL);

  pr->lg = lg;

  pr->part_start = (idx_t*) malloc((pr->lg->pg->n_part+1) *
				   sizeof(*pr->part_start));
  assert(pr->part_start != NULL);

  int n = pr->lg->pg->g->n;
  pr->perm = (idx_t*) malloc(n * sizeof(*pr->perm));
  assert(pr->perm);

  pr->this_perm = (idx_t*) malloc(n * sizeof(*pr->this_perm));
  assert(pr->this_perm);

  pr->inv_perm = (idx_t*) malloc(n * sizeof(*pr->inv_perm));
  assert(pr->inv_perm);

  for (int i = 0; i < n; i++) pr->perm[i] = i;
  
  return pr;
};

void del_perm(perm_t *pr)
{
  assert(pr != NULL);

  assert(pr->part_start != NULL);
  free(pr->part_start);
  pr->part_start = NULL;

  assert(pr->perm != NULL);
  free(pr->perm);
  pr->perm = NULL;
  free(pr->this_perm);
  pr->this_perm = NULL;
  free(pr->inv_perm);
  pr->inv_perm = NULL;
  
  free(pr);
}

void update_perm(perm_t* pr)
{

  int i = 0;
  int n = pr->lg->pg->g->n;
  int n_part = pr->lg->pg->n_part;
  idx_t *part_start = pr->part_start; 
  idx_t *part = pr->lg->pg->part;
  idx_t *this_perm = pr->this_perm;
  idx_t *inv_perm = pr->inv_perm;

  // local arrays.
  idx_t *iter = (idx_t*) calloc(n_part, sizeof(*iter));

  // prepare part_start
  // 1. init to 0
  // 2. calc numbíer of each part (one off, for easier scan op)
  // 3. prefix sum (scan op)
  for (i = 0; i < n_part+1; i++) part_start[i] = 0;
  for (i = 0; i < n; i++) part_start[part[i] + 1]++;
  for (i = 0; i < n_part; i++) part_start[i+1] += part_start[i]; 

  for (i = 0; i < n; i++) {
    idx_t p = part[i];
    idx_t j = part_start[p] + iter[p]; // new index
    this_perm[j] = i;
    inv_perm[i] = j;
    iter[p]++; // adjust for next iteration
  }

  free(iter);
}

void permutation(perm_t* pr, fp_t** bb, idx_t k_steps)
{
  // checklist to permute
  // - perm[], part[], levels[], partials[]
  // - matrix col[] and ptr[]
  // - bb[] vector (k_step times)

  int i = 0;
  int n = pr->lg->pg->g->n;
  int ne2 = pr->lg->pg->g->ptr[n];

  // perm[], part[], level[], partal[] arrays.
  idx_t *perm = pr->perm;
  idx_t *part = pr->lg->pg->part;
  idx_t *level = pr->lg->level;
  unsigned char *partial = pr->lg->partial;
  idx_t *tmp_perm = malloc(n * sizeof(*tmp_perm));
  idx_t *tmp_part = malloc(n * sizeof(*tmp_part));
  idx_t *tmp_level = malloc(n * sizeof(*tmp_level));
  unsigned char *tmp_partial = malloc(n * sizeof(*tmp_partial));

  // matrix is ptr[] and col[].
  idx_t *ptr = pr->lg->pg->g->ptr;
  idx_t *col = pr->lg->pg->g->col;
  idx_t *tmp_ptr = malloc((n+1) * sizeof(*tmp_ptr));
  idx_t *tmp_col = malloc(ne2 * sizeof(*tmp_col));

  // memory for bb[] permutation.
  fp_t *tmp_bb = malloc(n * k_steps * sizeof(*tmp_bb));
  
  // local arrays.
  idx_t *this_perm = pr->this_perm;
  idx_t *inv_perm = pr->inv_perm;


  //// DEBUG 1 ////
  // printf("D1: 1. perm[], 2. part[i],  3. this_perm[], 4. partial[] and base\n");
  // for (i = 0; i < n; i++) printf("%3d ", perm[i]); printf("\n");
  // for (i = 0; i < n; i++) printf("%3d ", part[i]); printf("\n");
  // for (i = 0; i < n; i++) printf("%3d ", this_perm[i]); printf("\n");
  // for (i = 0; i < n; i++) printf("%3d ", partial[i]); printf("\n");
  // for (i = 0; i < n; i++) printf("%3d ", i); printf("\n");

  // printf("CSR:::\n");
  // for (i = 0; i < n+1; i++) printf("%3d", ptr[i]); printf("\n");
  // for (i = 0; i < ne2; i++) printf("%3d", col[i]); printf("\n");

  // This is the main permutation loop: 
  
  idx_t k = 0;
  for (i = 0; i < n; i++) {
    idx_t j = this_perm[i];

    // perm[], part[], level[] and partial[] array swaps.
    tmp_perm[i] = perm[j];
    tmp_part[i] = part[j];
    tmp_level[i] = level[j];
    tmp_partial[i] = partial[j];
    
    // matrix ptr and col arrays
    tmp_ptr[i+1] = ptr[j+1] - ptr[j];
    for (idx_t t = ptr[j]; t < ptr[j+1]; t++)
      tmp_col[k++] = inv_perm[col[t]];

    // bb[] array permutation (for each level)
    fp_t *bptr = *bb;
    fp_t *new_bptr = tmp_bb;
    for (idx_t t = 0; t < k_steps; t++) {
      new_bptr[i] = bptr[j];
      new_bptr += n;
      bptr += n;
    }
  }

  // prefix sum of ptr[] and col[] renaming
  tmp_ptr[0] = 0;
  for (i = 0; i < n; i++)
    tmp_ptr[i+1] += tmp_ptr[i];
  
  //// DEBUG 2 ////
  // printf("D2: 1. new_perm[i], 2. new_part[i],  3. this_perm[], 4. new_partial[]\n");
  // for (i = 0; i < n; i++) printf("%3d ", new_perm[i]); printf("\n");
  // for (i = 0; i < n; i++) printf("%3d ", new_part[i]); printf("\n");
  // for (i = 0; i < n; i++) printf("%3d ", this_perm[i]); printf("\n");
  // for (i = 0; i < n; i++) printf("%3d ", new_partial[i]); printf("\n");
  // for (i = 0; i < n; i++) printf("%3d ", i); printf("\n");

  // printf("CSR:::\n");
  // for (i = 0; i < n+1; i++) printf("%3d", new_ptr[i]); printf("\n");
  // for (i = 0; i < ne2; i++) printf("%3d", new_col[i]); printf("\n");

  // perm[], part[], level[] and partial[] array cleanup.
  pr->perm = tmp_perm;
  pr->lg->pg->part = tmp_part;
  pr->lg->level = tmp_level;
  pr->lg->partial = tmp_partial;
  free(perm);
  free(part);
  free(level);
  free(partial);

  // Matrix ptr[] and col[] array cleanup.
  pr->lg->pg->g->ptr = tmp_ptr;
  pr->lg->pg->g->col = tmp_col;
  free(ptr);
  free(col);

  // bb[] array cleanup! CAUTION!
  // CAUTION: reverse deallocation, because pointer to pointer
  free(*bb);
  *bb = tmp_bb;
}

void inverse_permutation(perm_t* pr, fp_t** bb, idx_t k_steps)
{
  int i = 0;
  int n = pr->lg->pg->g->n;
  int ne2 = pr->lg->pg->g->ptr[n];

  // perm[], part[], level[], partal[] arrays.
  idx_t *perm = pr->perm;
  idx_t *part = pr->lg->pg->part;
  idx_t *level = pr->lg->level;
  unsigned char *partial = pr->lg->partial;
  idx_t *new_perm = malloc(n * sizeof(*new_perm));
  idx_t *new_part = malloc(n * sizeof(*new_part));
  idx_t *new_level = malloc(n * sizeof(*new_level));
  unsigned char *new_partial = malloc(n * sizeof(*new_partial));

  // matrix is ptr[] and col[].
  idx_t *ptr = pr->lg->pg->g->ptr;
  idx_t *col = pr->lg->pg->g->col;
  idx_t *new_ptr = malloc((n+1) * sizeof(*new_ptr));
  idx_t *new_col = malloc(ne2 * sizeof(*new_col));

  // memory for bb[] permutation.
  fp_t *new_bb = malloc(n * k_steps * sizeof(*new_bb));
  
  // This is the main permutation loop: 
  
  idx_t k = 0;
  for (i = 0; i < n; i++) {
    idx_t j = perm[i];

    // perm[], part[], level[] and partial[] array swaps.
    new_perm[j] = perm[i];
    new_part[j] = part[i];
    new_level[j] = level[i];
    new_partial[j] = partial[i];
    
    // matrix ptr and col arrays
    // probably wrong! check!!!
    new_ptr[j+1] = ptr[i+1] - ptr[i];
    for (idx_t t = ptr[i]; t < ptr[i+1]; t++)
      new_col[k++] = perm[col[t]]; // probably wrong!!!

    // bb[] array permutation (for each level)
    fp_t *bptr = *bb;
    fp_t *new_bptr = new_bb;
    for (idx_t t = 0; t < k_steps; t++) {
      new_bptr[j] = bptr[i];
      new_bptr += n;
      bptr += n;
    }
  }

  // prefix sum of new_ptr[]
  new_ptr[0] = 0;
  for (i = 0; i < n; i++)
    new_ptr[i+1] += new_ptr[i] ;

  // perm[], part[], level[] and partial[] array cleanup.
  pr->perm = new_perm;
  pr->lg->pg->part = new_part;
  pr->lg->level = new_level;
  pr->lg->partial = new_partial;
  free(perm);
  free(part);
  free(level);
  free(partial);

  // Matrix ptr[] and col[] array cleanup.
  pr->lg->pg->g->ptr = new_ptr;
  pr->lg->pg->g->col = new_col;
  free(ptr);
  free(col);

  // bb[] array cleanup! CAUTION!
  // CAUTION: reverse deallocation, because pointer to pointer
  free(*bb);
  *bb = new_bb;
}

skirt_t *new_skirt(part_t *pg) {
  assert(pg != NULL);

  skirt_t *sg = (skirt_t*) malloc(sizeof(skirt_t));
  assert(sg != NULL);

  sg->pg = pg;
  sg->levels = (int*) malloc(sizeof(int*) * pg->n_part * pg->g->n);
  assert(sg->levels != NULL);

  return sg;
}

void del_skirt(skirt_t *sg) {
  assert(sg != NULL);

  assert(sg->levels != NULL);
  free(sg->levels);
  sg->levels = NULL;

  free(sg);
}

void comp_skirt(skirt_t *sg) {
  assert(sg != NULL);

  int n = sg->pg->g->n;
  int np = sg->pg->n_part;
  int *p = sg->pg->part;
  int *l = sg->levels;

  int *t0 = (int*) malloc(sizeof(int) * n);
  int *t1 = (int*) malloc(sizeof(int) * n);
  assert(t0 != NULL && t1 != NULL);

  int i;
  for (i = 0; i < np; i++) {
    int j;
    for (j = 0; j < n; j ++)
      if (p[j] == i)
	t0[j] = t1[j] = 0;
      else
	t0[j] = t1[j] = n + 1;	/* big enough */

    do {
      j = minplus_x(sg->pg->g, t0, t1, i, p);
      int *tt = t0;  t0 = t1;  t1 = tt;
    } while (j > 0);

    for (j=0; j < n; j++)
      l[i * n + j] = t1[j];
  }

  free(t1);  free(t0);
}

void print_skirt_cost(skirt_t *sg, level_t *lg, int level) {
  assert(sg != NULL);

  int n = sg->pg->g->n;
  int np = sg->pg->n_part;
  int i;
  int n_comp = 0;
  int n_comm = 0;

  for (i = 0; i < np; i ++) {
    int *skirt = sg->levels + i * n;
    int j;

    if (lg != NULL) {
      int t[n], dom[n];

      for (j=0; j< n; j++)
	if (lg->level[j] + skirt[j] < level)
	  t[j] = 0;
	else
	  t[j] = 2;

      minplus(sg->pg->g, t, dom);

      for (j=0; j< n; j++)
	if (dom[j] == 0) {
	  n_comp += level - lg->level[j] - skirt[j];
	  n_comm ++;
	} else if (dom[j] == 1)
	  n_comm += 2;

    } else {

      for (j=0; j< n; j++)
	if (skirt[j] < level) {
	  n_comp += level - skirt[j];
	  n_comm ++;
	} else if (skirt[j] == level)
	  n_comm ++;
    }
  }

  int level_comp = 0;
  if (lg != NULL)
    for (i=0; i< n; i++)
      level_comp += (lg->level[i] > level ? level : lg->level[i]);

  printf("skirt cost: node %d level %d comp %f (%f) comm %f\n",
	 n, level, n_comp / (double) n, 
	 level_comp / (double) n, n_comm / (double) n);
}

void partition(part_t* pg, idx_t num_part){
  idx_t num_const = 1;
  idx_t dummy = 0;
  pg->n_part = num_part;
  METIS_PartGraphKway(&pg->g->n, &num_const, pg->g->ptr, pg->g->col,
		      NULL, NULL, NULL, &num_part,
		      NULL, NULL, NULL, &dummy, pg->part);
}

void wpartition(part_t* pg, wcrs_t* wg){
  idx_t num_const = 1;
  idx_t dummy = 0;

  idx_t opt[METIS_NOPTIONS];
  METIS_SetDefaultOptions(opt);
  opt[METIS_OPTION_UFACTOR]=1000;
  printf("check n_part: %d\n", pg->n_part);
  METIS_PartGraphKway(&pg->g->n, &num_const, pg->g->ptr, pg->g->col,
		      NULL, NULL, wg->we, &pg->n_part,
		      NULL, NULL, opt, &dummy, pg->part);
};

void iwrite(char* cmd, char* fn, int i, void* g){
  char tmp[80];
  sprintf(tmp, "%s.iter%d.%s",fn,i,cmd);
  FILE *f = fopen(tmp,"w");
  if( f == NULL ) {
    fprintf(stderr, "cannot open %s\n", tmp);
    exit(1);
  }
  if( strcmp(cmd, "level") == 0 ){
    write_level(f, (level_t*)g);
  } else if (strcmp(cmd,"weight") == 0){
    write_wcrs(f, (wcrs_t *)g);
  } else if (strcmp(cmd,"part") == 0){
    part_t *pg = (part_t*)g;
    for (int i = 0; i < pg->g->n; i++) {
      fprintf(f,"%d\n", pg->part[i]);
    }
    fclose(f);
  } else {
    fprintf(stderr, "This shouldn't happen, there is a typo in the code, "
	    "probably in %s", __FILE__);
  }
}

fp_t *new_bvect(int nn){
  int n = sqrt(nn);
  assert(n*n == nn);
  assert (n > 3);

  fp_t *bb = calloc(nn,  sizeof(fp_t));
  int i=0;
  while (i < n/3) {bb[i] = 1; bb[n*i] += 1; i++; }
  while (i < 2*(n/3)) {fp_t t= bb[i-1] + 1; bb[i] = t; bb[n*i] += t; i++; }
  fp_t end = bb[2*(n/3)-1]+1; 
  while (i < n) { bb[i] = end; bb[n*i] += end; i++; }
  for (i = 0; i < n; i++) {
    bb[n*i+n-1] += end;
    bb[(n-1)*n + i] += end;
  }
  // for(i=0;i<nn;i++)printf("%2.0f %s",bb[i],(i+1)%n?"":"\n");printf("\n");
  return bb;
}

void mul(part_t *pg, fp_t *b){
  int i,j,k,n,nn;
  nn = pg->g->n; n = sqrt(nn);
  idx_t* ptr = pg->g->ptr; idx_t* col = pg->g->col;

  fp_t* bb = calloc(nn, sizeof(fp_t));
  for (i = 0; i < nn; i++) {
    bb[i] = b[i] * pg->g->diag;
    idx_t end = ptr[i+1];
    for (j = ptr[i]; j < end; j++) {
      k = col[j];
      bb[i] += b[k]*pg->g->other;
    }
  }
  // for(i=0;i<nn;i++)printf("%4.0f %s",bb[i],(i+1)%n?"":"\n");printf("\n");
  memcpy(b, bb, nn*sizeof(fp_t));
  free(bb);
}


void mpk2(int level, level_t *lg, fp_t *bb) {
  int i,j,k,nn;
  nn = lg->pg->g->n;
  //int n = sqrt(nn);
  idx_t* ptr = lg->pg->g->ptr;
  idx_t* col = lg->pg->g->col;
  idx_t* part = lg->pg->part;

  fp_t* b = bb;
  bb += nn;
  idx_t* ll = malloc(nn * sizeof(*ll));
  for (i = 0; i < nn; i++) ll[i] = lg->level[i];
  int lvl = 0;
  int contp = 1;
  
  for(lvl = 0; contp && /**/ lvl < level-1; lvl++) { // for each level
    contp = 0;
    for (i = 0; i < nn; i++) { // for each node
      if (lvl == lg->level[i]){
	int i_is_complete = 1;
	if (lg->partial[i] == 0)
	  bb[i] = b[i] * lg->pg->g->diag;
	idx_t end = ptr[i+1];
	for (j = ptr[i]; j < end; j++) { // for each adj node
	  int diff = end - j;
	  assert(0 <= diff && diff<8);
	  k = col[j];
	  // if needed == not done
	  if (((1 << diff) & lg->partial[i]) == 0) {
	    if (part[i] == part[k] && lvl <= ll[k] ){ 
	      bb[i] += b[k] * lg->pg->g->other;
	      lg->partial[i] |= (1 << diff);
	    } else i_is_complete = 0;
	  } 
	} // end for each ajd node
	if(i_is_complete){
	  ll[i]++; // level up
	  lg->partial[i]=0; // no partials in new level
	  contp = 1;
	}
      } else contp = 1;
    }
    for (i = 0; i < nn; i++) lg->level[i] = ll[i];
    b = bb;
    bb += nn;
  }
  free(ll);
}
