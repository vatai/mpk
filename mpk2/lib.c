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

// Delete (free the memory of) the matrix/graph allocated by `new_crs`
// (or `read_crs`).
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

// Read a matrix/graph written to disk by `write_crs`.
crs0_t* read_crs(FILE *f) {
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

// Write a matrix/graph to the disc.
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
    y[i] = t;  // min(x[i], x[j] + 1)
  }

  return k;
}

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

void comp_level(level_t *lg) {
  assert(lg != NULL);
  int n = lg->pg->g->n;
  int np = lg->pg->n_part;
  int *p = lg->pg->part;
  int *l = lg->level;

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

  wcrs_t *wg = (wcrs_t*) malloc(sizeof(wcrs_t));
  assert(wg != NULL);

  wg->g = g;
  wg->wv = (int*) malloc(sizeof(int) * g->n);
  wg->we = (int*) malloc(sizeof(int) * 2 * g->ptr[g->n]);

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
	wg->we[j] = w;
    }
  }
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

    if (lg != NULL) {
      int j;
      for (j=0; j< n; j++)
	if (lg->level[j] < level - skirt[j])
	  fprintf(f, "%d %d\n", j, level - skirt[j]);
    } else {
      int j;
      for (j=0; j< n; j++)
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

    while (1) {
      line[0] = '\0';
      fgets(line, 10240, f);
      if ((feof(f) && line[0] == '\0') || (line[0] == 'p'))
	break;

      int k, m;
      sscanf(line, "%d %d", &k, &m);
      if (k < 0 || n <= k || m < 0 || level < m) {
	fprintf(stderr, "error in reading skirt: %s", line);
	exit(1);
      }
      skirt[k] = level - m;
    }
  }

  fclose(f);
}
    

void print_skirt_cost(skirt_t *sg, level_t *lg, int level) {
  assert(sg != NULL);

  int n = sg->pg->g->n;
  int np = sg->pg->n_part;
  int n_comp = 0;
  int n_comm = 0;

  int i;
  for (i = 0; i < np; i ++) {
    int *skirt = sg->levels + i * n;

    if (lg != NULL) {
      int t[n], dom[n];

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

    } else {

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
      level_comp += (lg->level[i] > level ? level : lg->level[i]);

  printf("skirt cost: node %d level %d comp %f (%f) comm %f\n",
	 n, level, n_comp / (double) n, 
	 level_comp / (double) n, n_comm / (double) n);
}
