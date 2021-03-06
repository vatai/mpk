/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-12-07

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "lapjv.h"

const int kUnassigned = -1;

void check_alloc(void *ptr) {
  if (ptr == NULL) {
    fprintf(stderr, "%s::check_alloc(): Not enough memory!\n", __FILE__);
    exit(-1);
  }
}

/// Dense matrix to store the problem.
struct mat {
  int *data; ///< Data stored in row major order.
  int n;     ///< Size of a row/col of the matrix.
};

// struct mat //

struct mat *new_mat(const int n) {
  struct mat *m;
  m = (struct mat *)malloc(sizeof(struct mat));
  check_alloc(m);
  m->n = n;
  m->data = (int *)malloc(n * n * sizeof(int));
  check_alloc(m->data);
  return m;
}

void del_mat(struct mat *m) {
  free(m->data);
  free(m);
}

int elem(struct mat *m, int i, int j) { return m->data[i * m->n + j]; }

// struct assign //

/// Assignment of rows and columns (a representation of the solution).
struct assign {
  int *col_at; ///< Column assigned to rows (x in the paper).
  int *row_at; ///< Row assigned to columns (y in the paper).
};

struct assign *new_assign(const int n) {
  struct assign *a = (struct assign *)malloc(sizeof(struct assign));
  check_alloc(a);
  a->col_at = (int *)malloc(n * sizeof(int));
  check_alloc(a->col_at);
  a->row_at = (int *)malloc(n * sizeof(int));
  check_alloc(a->row_at);
  return a;
}

void del_assign(struct assign *a) {
  free(a->row_at);
  free(a->col_at);
  free(a);
}

// struct dual //

/// Dual problem.
struct dual {
  int *row; ///< Row variable of the dual.
  int *col; ///< Column variable of the dual.
};

struct dual *new_dual(const int n) {
  struct dual *dual = (struct dual *)malloc(sizeof(struct dual));
  check_alloc(dual);
  dual->row = (int *)malloc(n * sizeof(int));
  check_alloc(dual->row);
  dual->col = (int *)malloc(n * sizeof(int));
  check_alloc(dual->col);
  return dual;
}

void del_dual(struct dual *dual) {
  free(dual->col);
  free(dual->row);
  free(dual);
}

// struct free //

/// Unassigned rows.
struct free {
  int *data; ///< Data (allocated npart elements).
  int size;  ///< Actual size.
};

struct free *new_free(const int n) {
  struct free *f = (struct free *)malloc(sizeof(struct free));
  check_alloc(f);
  f->size = 0;
  f->data = (int *)malloc(n * sizeof(int));
  check_alloc(f);
  return f;
}

void del_free(struct free *f) {
  free(f->data);
  free(f);
}

void insert(struct free *f, const int val) {
  f->data[f->size] = val;
  f->size++;
}

// struct col //

/// Array of columns: scanned are `data[0..low-1]`; labeled and
/// unscanned `data[low..up-1]`; unlabeled `data[up..npart-1]`.
struct col {
  int *data; ///< The actual data.
  int low;   ///< Low boundary.
  int up;    ///< Up boundary.
};

struct col *new_col(const int n) {
  struct col *col = (struct col *)malloc(sizeof(struct col));
  check_alloc(col);
  col->low = 0;
  col->up = 0;
  col->data = (int *)malloc(n * sizeof(int));
  check_alloc(col->data);
  return col;
}

void del_col(struct col *col) {
  free(col->data);
  free(col);
}

// // // // // //

static int lapjv_mark(int val) { return val < kUnassigned ? val : -val - 2; }

static int lapjv_unmark(int val) { return -val - 2; }

static void lapjv_prep(int *sums, struct mat *m, struct assign *a,
                       struct dual *d) {
  const int n = m->n;
  const int size = n * n;

  // Find max
  int max_sum = sums[0];
  for (int i = 1; i < size; i++)
    if (sums[i] > max_sum)
      max_sum = sums[i];

  // Create mat
  for (int i = 0; i < size; i++)
    m->data[i] = max_sum - sums[i];

  // init assignment and dual
  for (int i = 0; i < n; i++) {
    a->col_at[i] = kUnassigned;
    a->row_at[i] = kUnassigned;
    d->row[i] = 0;
    d->col[i] = 0;
  }
}

/// Input:
///   `d->row[i] = d->col[j] = 0;`
///   `a->col_at[i] = a->row_ar[j] = 0;`
///
/// Output:
///   `col->data[j] = j;`
///   For a column `j`
static void lapjv_colred(struct mat *m, struct assign *a, struct dual *d,
                         struct col *col) {
  const int n = m->n;
  for (int j = n - 1; j >= 0; j--) {
    col->data[j] = j; // Init `col`
    int min_val = elem(m, 0, j);
    int i1 = 0;
    for (int i = 1; i < n; i++) {
      if (elem(m, i, j) < min_val) {
        min_val = elem(m, i, j);
        i1 = i;
      }
    }
    d->col[j] = min_val;
    if (a->col_at[i1] == kUnassigned) {
      a->col_at[i1] = j;
      a->row_at[j] = i1;
    } else {
      a->col_at[i1] = lapjv_mark(a->col_at[i1]);
      a->row_at[j] = kUnassigned;
    }
  }
}

static void lapjv_redtransf(struct mat *m, struct assign *a, struct dual *d,
                            struct col *col, struct free *free) {
  free->size = 0;
  const int n = m->n;
  for (int i = 0; i < n; i++) {
    if (a->col_at[i] == kUnassigned)
      insert(free, i);
    else if (a->col_at[i] < kUnassigned)
      a->col_at[i] = lapjv_unmark(a->col_at[i]);
    else {
      int j1 = a->col_at[i];
      int min = INT_MAX;
      for (int j = 0; j < n; j++) {
        if (j != j1) {
          if (elem(m, i, j) - d->col[j] < min)
            min = elem(m, i, j) - d->col[j];
        }
      }
      d->col[j1] -= min;
    }
  }
}

static void lapjv_augrowred(struct mat *m, struct assign *a, struct dual *d,
                            struct free *free) {
  const int n = m->n;
  const int f_size = free->size;
  int k_size = 0;
  free->size = 0;
  while (k_size < f_size) {
    int i = free->data[k_size++];
    int u1 = elem(m, i, 0) - d->col[0];
    int u2 = INT_MAX;
    int j1 = 0;
    int j2;
    for (int j = 1; j < n; j++) {
      int h = elem(m, i, j) - d->col[j];
      if (h < u2) {
        if (h >= u1) {
          u2 = h;
          j2 = j;
        } else {
          u2 = u1;
          u1 = h;
          j2 = j1;
          j1 = j;
        }
      }
    }
    int i1 = a->row_at[j1];
    if (u1 < u2)
      d->col[j1] = d->col[j1] - u2 + u1;
    else if (i1 > kUnassigned) {
      j1 = j2;
      i1 = a->row_at[j1];
    }
    if (i1 > kUnassigned) {
      if (u1 < u2)
        free->data[--k_size] = i1;
      else
        insert(free, i1);
    }
    a->col_at[i] = j1;
    a->row_at[j1] = i;
  }
}

static int lapjv_auginner(struct mat *m, struct assign *a, struct dual *d,
                          struct col *col, int *pred, int *d_arr, int *last,
                          int *min) {
  const int n = m->n;
  int h, j;
  while (1) {
    if (col->up == col->low) {
      *last = col->low;
      *min = d_arr[col->data[col->up++]];
      for (int k = col->up; k < n; k++) {
        j = col->data[k];
        h = d_arr[j];
        if (h <= *min) {
          if (h < *min) {
            col->up = col->low;
            *min = h;
          }
          col->data[k] = col->data[col->up];
          col->data[col->up++] = j;
        }
      }
      for (h = col->low; h < col->up; h++) {
        j = col->data[h];
        if (a->row_at[j] == kUnassigned)
          return j; // GOTO
      }
    } // up == low

    // scan row
    int j1 = col->data[col->low++];
    int i = a->row_at[j1];
    int u1 = elem(m, i, j1) - d->col[j1] - *min;
    for (int k = col->up; k < n; k++) {
      j = col->data[k];
      h = elem(m, i, j) - d->col[j] - u1;
      if (h < d_arr[j]) {
        d_arr[j] = h;
        pred[j] = i;
        if (h == *min) {
          if (a->row_at[j] == kUnassigned)
            return j; // GOTO
          else {
            col->data[k] = col->data[col->up];
            col->data[col->up++] = j;
          }
        }
      }
    }
  } // end while(1)
}

static void lapjv_augment(struct mat *m, struct assign *a, struct dual *d,
                          struct col *col, struct free *free) {
  const int n = m->n;
  const int f_size = free->size;
  int d_arr[n];
  int pred[n];
  // For every free ertex.
  for (free->size = 0; free->size < f_size; free->size++) {
    int i1 = free->data[free->size];
    int min;
    col->low = 0;
    col->up = 0;
    for (int j = 0; j < n; j++) {
      d_arr[j] = elem(m, i1, j) - d->col[j];
      pred[j] = i1;
    }
    int j1, last;
    // Inner start.
    int j = lapjv_auginner(m, a, d, col, pred, d_arr, &last, &min);
    // Inner end.
    for (int k = 0; k < last; k++) {
      j1 = col->data[k];
      d->col[j1] = d->col[j1] + d_arr[j1] - min;
    }
    int i;
    do {
      i = pred[j];
      a->row_at[j] = i;
      int k = j;
      j = a->col_at[i];
      a->col_at[i] = k;
    } while (i != i1);
  }
}

static int lapjv_finalize(struct mat *m, struct assign *a, struct dual *d) {
  const int n = m->n;
  int cost = 0;
  for (int i = 0; i < n; i++) {
    int j = a->col_at[i];
    d->row[i] = elem(m, i, j) - d->col[j];
    cost += d->row[i] + d->col[j];
  }
  return cost;
}

void lapjv(int *sums, int npart, int *perm) {
  // "local" arrays
  struct mat *msums = new_mat(npart);
  struct assign *asgn = new_assign(npart);
  struct dual *dual = new_dual(npart);
  struct col *col = new_col(npart);
  struct free *free = new_free(npart);

  lapjv_prep(sums, msums, asgn, dual);
  lapjv_colred(msums, asgn, dual, col);
  lapjv_redtransf(msums, asgn, dual, col, free);
  lapjv_augrowred(msums, asgn, dual, free);
  lapjv_augrowred(msums, asgn, dual, free);
  lapjv_augment(msums, asgn, dual, col, free);
  lapjv_finalize(msums, asgn, dual);

  for (int j = 0; j < npart; j++)
    perm[j] = asgn->row_at[j];

  del_free(free);
  del_col(col);
  del_assign(asgn);
  del_dual(dual);
  del_mat(msums);
}
