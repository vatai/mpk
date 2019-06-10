// The lib.c implements most of the data-structures and it's
// operations/methods defined in lib.h.  Most of the data-structures
// have a new_*, del_*, read_*, write_* functions.
//
// new_* :: Usually only the new_* function is documented, which
// allocates the data-structure.
//
// del_* :: The del_* functions free/delete the data-structure.
//
// write_* :: The write_* functions write out the data-structures to
// the disc.
//
// read_* :: The read_* functions read the data-structure from the
// disc.  Only `read_crs()` calls `new_crs()`, all the other read
// functions assume the matrix/graph is already allocated with the
// appropriate new_* function.
//
// Some functions have *_c suffix, which indicates that the function
// deals with coordinates (i.e. visual representation) and for now
// they are not documented.
//
// There are several other functions with special purposes.  They are
// usually important, and documented.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lib.h"

// Creates (allocate) a new sparse matrix in compressed row storage
// format, with `nv` number of rows (and columns) and `ne` number of
// non-zero values.  The connection to graphs should be noted: in the
// adjacency matrix of a graph, the rows and columns both represent
// the vertices of a graph (hence the `nv` refers to the number of
// vertices), and there is an entry in the matrix iff there is an edge
// in the graph between the vertices represented by the row and column
// of the matrix.
crs0_t *new_crs(int nv, int ne) {
  crs0_t *g = (crs0_t*) malloc(sizeof(crs0_t));
  assert(g != NULL);

  g->n = nv;
  g->ptr = (int*) malloc(sizeof(int) * (nv + 1));
  // The 2 multiplies comes from the assumption that the graph is
  // undirected.
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

crs0_t* read_crs(FILE *f) { // Called out function from comp.c to read the crs or graph file 
  char line[10240];

  fgets(line, 10240, f);

  int nv, ne, p, q, i;
  i = sscanf(line, "%d %d %d %d", &nv, &ne, &p, &q);
  if (i != 2) {
    fprintf(stderr, "read_crs: format %d %d not implemented yet\n", p, q);
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
      if (g->col[p] < 0 || nv <= g->col[p]) {
	fprintf(stderr, "read_crs: error edge %d %d\n", i, g->col[p]);
	exit(1);
      }
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

// For each `i`, `y[i]` is the minimum of `x[i]` and `x[j] + 1` for
// all vertices `j` adjacent to `i`.
//
// If `x[]` holds "levels" of MPK, than `minplus()` will calculate the
// next level for all `x[]`.
//
// It should be noted, that although `y[]` is the output of the
// function, if we want to calculate it for certain vertices
// (i.e. some values of `i`) those `x[i]` should be initialised to a
// "big enough" value.  So it will not work for arbitrary `x[]`.
//
// TODO(vatai): refactor the minplus functions or specify exact
// precondition for input values.
//
// Here returned value k is the number of modifications made. If it
// returns 0 then it means that we have reached the end of phase.
int minplus(crs0_t *g, int *x, int *y) {
  int i, j, k = 0;
  for (i=0; i < g->n; i++) {
    int t = x[i];
    for (j= g->ptr[i]; j < g->ptr[i + 1]; j++) {
      int s = x[g->col[j]] + 1;
      if (s < t) {
	t = s;
	k++;
      }
    }
    y[i] = t;  // min(x[i], x[j] + 1 for all j adjacent i)
  }

  return k;
}

// Similar to `minplus()` but do the calculations only for the
// vertices in partition `r`.
int minplus_r(crs0_t *g, int *x, int *y, int r, int *part) {
  int i, j, k=0;
  for (i=0; i< g->n; i++)
    if (part[i] == r) {
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

// Similar to `minplus()` but do the calculations only for the
// vertices in not partition `r`.
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

// Create a partitioned graph/matrix from a CRS graph/matrix: a graph
// which stores the number of partitions and the partition index for
// each vertex.
part_t *new_part(crs0_t *g) { // to create a empty partition according to the features of graph.
  assert(g != NULL);

  part_t *pg = (part_t*) malloc(sizeof(part_t));
  assert(pg != NULL);

  pg->g = g; 
  pg->n_part = -1; 
  pg->part = (int*) malloc(sizeof(int) * g->n);

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
  for (i=0; i< pg->g->n; i++) { // according to size of graph we will store the value of partition to which ith index belong
    int p;
    fscanf(f, "%d", &p);
    if (max < p)
      max = p;
    pg->part[i] = p;
  }

  pg->n_part = max + 1; // total partitions
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

// Create a graph/matrix with levels from a partitioned graph: a
// partitioned graph which stores the level for each node.
level_t *new_level(part_t *pg) {
  assert(pg != NULL);

  level_t *lg = (level_t*) malloc(sizeof(level_t));
  assert(lg != NULL);

  lg->pg = pg;
  lg->level = (int*) malloc(sizeof(int) * pg->g->n);
  assert(lg->level != NULL);

  return lg;
}

void del_level(level_t* lg) {
  assert(lg != NULL);

  assert(lg->level != NULL);
  free(lg->level);
  lg->level = NULL;

  free(lg);
}

// Compute a levels of a partitioned graph in the initial phase.  This
// method should be called in the first phase and update_level()
// should be called in the i-th stage, for i > 2.
void comp_level(level_t *lg) {
  assert(lg != NULL);
  int n = lg->pg->g->n;
  int np = lg->pg->n_part;
  int *p = lg->pg->part;
  int *l = lg->level;

  // Temporary arrays.
  int *t0 = (int*) malloc(sizeof(int) * n);
  int *t1 = (int*) malloc(sizeof(int) * n);
  assert(t0 != NULL && t1 != NULL);

  int i;
  for (i = 0; i < np; i++) {
    // For each partition: `i` is the current partition.
    int j;
    for (j=0; j < n; j++)
      if (p[j] == i)
        // For vertices in the current partition.
	t0[j] = t1[j] = n+1;		// a big enough value
      else
        // For vertices NOT in the current partition.
	t0[j] = t1[j] = -1;

    // Calculate the levels as long as possible (after a finite number
    // of steps, the levels will stop changing).  Input is `t0`;
    // output is `t1`; after computation the `t0` and `t1` are
    // swapped.
    do {
      j = minplus_r(lg->pg->g, t0, t1, i, p);
      int *tt = t0;  t0 = t1;  t1 = tt;
    } while (j > 0);

    // The levels are calculated in the `t1[]` array, and are copied
    // to the `l` array storing the final result.
    for (j=0; j< n; j++)
      if (p[j] == i)
	l[j] = t1[j];
  }

  free(t1);  free(t0);
}
// update level is almost same as comp_level the only difference is that there are values already assigned and we will update over that
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

// Create a graph/matrix with weighted edges.  The edge weights are
// calculated by the `level2wcrs()` function.
wcrs_t *new_wcrs(crs0_t *g) {
  assert(g != NULL);

  wcrs_t *wg = (wcrs_t*) malloc(sizeof(wcrs_t));
  assert(wg != NULL);

  wg->g = g;
  wg->wv = (int*) malloc(sizeof(int) * g->n);
  wg->we = (int*) malloc(sizeof(int) * g->ptr[g->n]); // Removed a multiplicand of 2 from here (Utsav)

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
    fprintf(f, "1 "/*, wg->wv[i]*/);
    for (j= g->ptr[i]; j< g->ptr[i+1]; j++) {
      fprintf(f, "%d ", g->col[j] + 1);
      fprintf(f, "%d ", wg->we[j]);
    }
    fprintf(f, "\n");
  }

  fclose(f);
}

// Calculate the edge weights based on the levels.
void level2wcrs(level_t *lg, wcrs_t *wg) {
  assert(lg != NULL);
  assert(wg != NULL);
  assert(lg->pg->g == wg->g);

  int max, min;
  int n = wg->g->n;

  max = min = lg->level[0];

  int i;
  for (i=1; i< n; i++) {
    if (max < lg->level[i])
      max = lg->level[i];
    if (min > lg->level[i])
      min = lg->level[i];
  }

  for (i=0; i< n; i++) {
    wg->wv[i] = max + 1 - lg->level[i];

    int l0 = lg->level[i];
    int j;
    for (j = wg->g->ptr[i]; j< wg->g->ptr[i+1]; j++) {
      int l1 = lg->level[wg->g->col[j]];
      double w = l0 + l1 - 2 * min;
      w = 1e+6 / (w + 1);
      if (w < 1.0)
	wg->we[j] = 1;
      else
	wg->we[j] = w; // Here we are converting the double to integers
    }
  }
}

// `skirt_t` is like `level_t` but with each partition having its own
// levels stored i.e. `n_part * n` number of `levels` (while `level_t`
// has only `n`).
//
// TODO(vatai): [maybe] Move these skirt functions to skirt.c because
// they are only used there.
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

// Calculate the level achievable/reachable in each partition is
// calculated.  For each partition `i`, the input of `minplus_x`
// (`t0`) is initialised to be 0 in the current partition, and a "big
// enough" number everywhere else.  `minplus_x()` updates only the
// vertices NOT in the current partition.
//
// Example: After initialisation (before `minplus_x()`):
//
// 999000999  (the current partition is the middle three digits)
//
// And after calling `minplus_x()`:
//
// 321000123
//
// As a result `sg->levels[i * n + j]` will have the level of vertex
// `j` available/reachable using only elements of partition `i`.
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

void write_skirt(FILE *f, skirt_t *sg, level_t *lg, int level) {
  assert(f != NULL);
  assert(sg != NULL);

  int n = sg->pg->g->n;
  int np = sg->pg->n_part;

  fprintf(f, "skirt %d nodes, %d parts\n", n, np);

  int i;
  for (i=0; i< np; i++) {
    fprintf(f, "partition %d\n", i);

    int *skirt = sg->levels + i * n;

    // TODO(vatai): this could (should?) be refactored!!!
    //
    // Conditionally store `j` and `level - skirt[j]`
    if (lg != NULL) { // lg is null in the case of phase = 0. So this part is only for dmpk
      int j;
      for (j=0; j< n; j++)
        // Condition with `lg`:
        // `skirt[j] + lg->level[j] < level`
	if (lg->level[j] < level - skirt[j])
	  fprintf(f, "%d %d\n", j, level - skirt[j]);
    } else {// This proceeds as pa1
      int j;
      for (j=0; j< n; j++)
        // Condition without `lg`:
        // `skirt[j]                < level`
	if (skirt[j] < level)
	  fprintf(f, "%d %d\n", j, level - skirt[j]);
    }
  }

  fclose(f);
}

void read_skirt(FILE *f, skirt_t *sg, int level) {
  assert(f != NULL);
  assert(sg != NULL);
  assert(level > 0);

  int n = sg->pg->g->n;
  int np = sg->pg->n_part;

  char line[10240];
  fgets(line, 10240, f);

  int i, j;
  sscanf(line, "skirt %d nodes, %d parts", &i, &j);
  if (i != n || j != np) {
    fprintf(stderr, "error in reading skirt: %d/%d %d/%d\n", i, n, j, np);
    exit(1);
  }

  fgets(line, 10240, f);
  for (i=0; i< np; i++) {

    sscanf(line, "partition %d", &j);
    if (j != i) {
      fprintf(stderr, "error in reading skirt: %d/%d\n", i, j);
      exit(1);
    }

    int *skirt = sg->levels + i * n;
    for (j=0; j< n; j++)
      skirt[j] = -1;		/* no data */

    while (1) { // Read the skirt of one partition.
      line[0] = '\0';
      fgets(line, 10240, f);
      // Read until the end of the file or a line starting with 'p'
      if ((feof(f) && line[0] == '\0') || (line[0] == 'p'))
	break;

      // Read `k` and `m`.
      int k, m;
      sscanf(line, "%d %d", &k, &m);
      if (k < 0 || n <= k || m < 0 || level < m) {
	fprintf(stderr, "error in reading skirt: %s", line);
	exit(1);
      }
      // Store `k` and `m`.
      skirt[k] = level - m;
    }
  }

  fclose(f);
}

// TODO(vatai): Not entirely sure about this function.  It is
// commented out in skirt.c, but it should print computation and
// communication costs.
//
// Calculate the total cost, to get all vertices to `level` assuming
// the `skirt` and all vertices calculated to `level`.
//
// lg is optional (i.e. can be NULL).
void print_skirt_cost(skirt_t *sg, level_t *lg, int level) {
  assert(sg != NULL);

  int n = sg->pg->g->n;
  int np = sg->pg->n_part;
  int n_comp = 0;
  int n_comm = 0;

  int i;
  for (i = 0; i < np; i ++) {
    // For each partition.

    // `skirt` of the current partition
    int *skirt = sg->levels + i * n;

    // The easier version is in the other branch below.
    if (lg != NULL) {
      // Do the same as `lg == NULL` (see below) but instead of
      // `skirt[j]` use `skirt[j] + lg->level[j]`?? (vatai)

      int t[n], dom[n];

      // TODO(vatai): 
      int j;
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

    } else {  // if lg == NULL
      // For each vertex, `n_comp` (i.e. number of computations) is
      // the number of "level jumps" needed to reach `level` assuming
      // the vertices already reached level `skirt[j]` (which is
      // calculated by `level - skirt[j]`); `n_comm` (or number of
      // communications) is the number of vertices below `level`.

      int j;
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
      // `level_comp` is: sum over i for min(level, lg->level[i])
      level_comp += (lg->level[i] > level ? level : lg->level[i]);

  // output:
  // - n_comp/n,
  // - level_comp/n,
  // - n_comm/n
  printf("skirt cost: node %d level %d comp %f (%f) comm %f\n",
	 n, level, n_comp / (double) n, 
	 level_comp / (double) n, n_comm / (double) n);
}
