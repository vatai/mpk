#ifndef _LIB_H_
#define _LIB_H_

#include <stdio.h>
#include <metis.h>

typedef float fp_t;

typedef struct {
  idx_t n;			/* number of nodes */
  idx_t *ptr;			/* pointer */
  idx_t *col;			/* column index */
  fp_t diag;                    /* element on the diagonal */
  fp_t other;                   /* other elements */
} crs0_t;

crs0_t *new_crs(int, int);
void del_crs(crs0_t*);
crs0_t *read_crs(FILE*);
void write_crs(FILE*, crs0_t*);

typedef struct {
  crs0_t *g;
  double *x;
  double *y;
} coord_t;

coord_t *new_coord(crs0_t*);
void read_coord(FILE*, coord_t*);

typedef struct {
  crs0_t *g;			/* graph */
  idx_t n_part;			/* number of parts */
  idx_t *part;			/* partition */
} part_t;

part_t *new_part(crs0_t*);
void del_part(part_t*);
void read_part(FILE*, part_t*);
void write_part_c(FILE*, part_t*, coord_t*);

// min plus

typedef struct {
  part_t *pg;
  idx_t *level;
  unsigned char *partial;
} level_t;

level_t *new_level(part_t*);
void del_level(level_t*);
void comp_level(level_t*);
void update_level(level_t*,level_t*);
void write_level(FILE*, level_t*);
void write_level_c(FILE*, level_t*, coord_t*);

typedef struct {
  crs0_t *g;
  idx_t *wv;
  idx_t *we;
} wcrs_t;

wcrs_t *new_wcrs(crs0_t*);
void del_wcrs(wcrs_t*);
void write_wcrs(FILE*, wcrs_t*);

void partition(part_t*,idx_t);
void wpartition(part_t*,wcrs_t*);
void level2wcrs(level_t*, wcrs_t*);

typedef struct {
  level_t *lg;
  idx_t *part_start;
  idx_t *perm; // Global permutation
  idx_t *this_perm;
  idx_t *inv_perm;
} perm_t;

perm_t *new_perm(level_t *lg);
void del_perm(perm_t *pr);
void update_perm(perm_t* pr);
void permutation(perm_t* pr, fp_t** bb, idx_t k_step);
void inverse_permutation(perm_t* pr, fp_t** bb, idx_t k_steps);

typedef struct {
  part_t *pg;
  int *levels;
} skirt_t;

skirt_t *new_skirt(part_t*);
void del_skirt(skirt_t*);
void comp_skirt(skirt_t*);
void print_skirt_cost(skirt_t*, level_t*, int);

void read_level(FILE *f, level_t *lg);

void iwrite(char*, char*, int, void*);
fp_t *new_bvect(int nn);
void mul(part_t *pg, fp_t *b);
void mpk1(int level, level_t *lg, fp_t *b);
void mpk2(int level, perm_t *pr, fp_t *bb);

#endif // _LIB_H_
