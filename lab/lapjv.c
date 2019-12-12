/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-12-07
///
/// Jonker-Volgenant algorithm - buggy for now.
///
/// Paper: https://link.springer.com/article/10.1007%2FBF02278710
///
/// Run using ./lapjv csum_xenon2....txt

// elem(mat, i, j) is c[i, j]
//
// assign->col_at[i] is x[i]
// assign->row_at[j] is y[j]
//
// dual->row[i] is u[i]
// dual->col[j] is v[j]

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
const int kUnassigned = -1;

void check_alloc(void *ptr) {
  if (ptr == NULL) {
    fprintf(stderr, "Not enough memory!\n");
    exit(-1);
  }
}

static void print_arr(char *name, int n, int *vec) {
  printf("%s: ", name);
  for (int i = 0; i < n; i++)
    printf("%d, ", vec[i]);
  printf("\n");
}

struct mat {
  int *data;
  int n;
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

static void print_mat(char *name, struct mat *m) {
  printf("%s:\n", name);
  for (int i = 0; i < m->n; i++) {
    for (int j = 0; j < m->n; j++)
      printf("%d, ", elem(m, i, j));
    printf("\n");
  }
  printf("\n");
}

// struct assign //

struct assign {
  int *col_at;
  int *row_at;
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

void print_assign(const int n, struct assign *a) {
  print_arr("col_at: ", n, a->col_at);
  print_arr("row_at: ", n, a->row_at);
}

// struct dual //

struct dual {
  int *row;
  int *col;
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

void print_dual(const int n, struct dual *dual) {
  print_arr("dual_row: ", n, dual->row);
  print_arr("dual_col: ", n, dual->col);
}

void check_dual(struct mat *m, struct dual *dual) {
  const int n = m->n;
  for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++) {
      if (0 > elem(m, i, j) - dual->row[i] - dual->col[j]) {
        printf("check_dual():: %d and %d,%d at %d,%d\n", elem(m, i, j),
               dual->row[i], dual->col[j], i, j);
      }
      assert(0 <= elem(m, i, j) - dual->row[i] - dual->col[j]);
    }
}

// struct free //

struct free {
  int *data;
  int size;
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

struct col {
  int *data;
  int low;
  int up;
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

static void visit_perm(int npart, int *sums, int *tperm, int *perm, int *_max) {
  int sum = 0;
  for (int i = 0; i < npart; i++) {
    int j = tperm[i];
    sum += sums[j * npart + i];
  }
  if (sum > *_max) {
    *_max = sum;
    memcpy(perm, tperm, sizeof(*tperm) * npart);
  }
}

static void make_perm(int *perm, int *sums, int npart) {
  int tmp;
  int *tmpperm = (int *)malloc(sizeof(*tmpperm) * npart);
  // init perm a1 <= a2 <= .. <= an
  for (int i = 0; i < npart; i++) {
    tmpperm[i] = i;
    perm[i] = i;
  }
  int maxsum = 0;
  while (1) {
    visit_perm(npart, sums, tmpperm, perm, &maxsum);
    // Set j.
    int j = npart - 2;
    while (j >= 0 && tmpperm[j] >= tmpperm[j + 1])
      j--;
    if (j < 0)
      break;
    // Set i. (ell in TAOCP)
    int i = npart - 1;
    while (i >= 0 && tmpperm[j] >= tmpperm[i])
      i--;
    // Swap tmpperm[i] and tmpperm[j].
    tmp = tmpperm[j];
    tmpperm[j] = tmpperm[i];
    tmpperm[i] = tmp;
    assert(j >= 0);
    j++;
    i = npart - 1;
    while (j < i) {
      // Swap tmpperm[i] == tmpperm[j].
      tmp = tmpperm[j];
      tmpperm[j] = tmpperm[i];
      tmpperm[i] = tmp;
      j++;
      i--;
    }
  }
  free(tmpperm);
}

static int *alloc_read_comm_sums(char *fname, int *npart) {
  FILE *file = fopen(fname, "r");
  fscanf(file, "%d\n", npart);
  int size = (*npart) * (*npart);
  int *comm_sums = (int *)malloc(size * sizeof(*comm_sums));
  assert(comm_sums != NULL);
  for (int i = 0; i < size; i++)
    fscanf(file, "%d\n", comm_sums + i);
  fclose(file);
  return comm_sums;
}

// /// /// //

static int lapjv_mark(int val) {
  return val < kUnassigned ? val: -val - 2;
}

static int lapjv_unmark(int val) {
  return -val - 2;
}

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

static void lapjv_colred(struct mat *m, struct assign *a, struct dual *d,
                         struct col *col) {
  const int n = m->n;
  for (int j = n - 1; j >= 0; j--) {
    col->data[j] = j;
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

static void lapjv_redtransf(struct mat *m, struct assign *a,
                            struct dual *d, struct col *col,
                            struct free *free) {
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

static void lapjv_augrowred(struct mat *m, struct assign *a,
                            struct dual *d, struct free *free) {
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
                          struct col *col, struct free *fr) {
  const int n = m->n;
  const int f_size = fr->size;
  int *d_arr = (int *)malloc(n * sizeof(*d_arr));
  int *pred = (int *)malloc(n * sizeof(*pred));
  // For every free ertex.
  for (fr->size = 0; fr->size < f_size; fr->size++) {
    int i1 = fr->data[fr->size];
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
  free(pred);
  free(d_arr);
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

static void lapjv(int *sums, int npart, int *perm) {
  // "local" arrays
  struct mat *msums = new_mat(npart);
  struct assign *asgn = new_assign(npart);
  struct dual *dual = new_dual(npart);
  struct col *col = new_col(npart);
  struct free *free = new_free(npart);

  lapjv_prep(sums, msums, asgn, dual);
  printf("=== after init ===\n");
  print_mat("msums", msums);
  print_dual(npart, dual);
  print_assign(npart, asgn);
  check_dual(msums, dual);

  lapjv_colred(msums, asgn, dual, col);
  printf("=== after colred ===\n");
  printf("free->size: %d\n", free->size);
  print_dual(npart, dual);
  print_assign(npart, asgn);
  check_dual(msums, dual);

  lapjv_redtransf(msums, asgn, dual, col, free);
  printf("=== after redtransf ===\n");
  printf("free->size: %d\n", free->size);
  print_dual(npart, dual);
  print_assign(npart, asgn);
  check_dual(msums, dual);

  lapjv_augrowred(msums, asgn, dual, free);
  printf("=== after lapjv_augrowred ===\n");
  printf("free->size: %d\n", free->size);
  print_dual(npart, dual);
  print_assign(npart, asgn);
  check_dual(msums, dual);

  lapjv_augrowred(msums, asgn, dual, free);
  printf("=== after lapjv_augrowred ===\n");
  printf("free->size: %d\n", free->size);
  print_dual(npart, dual);
  print_assign(npart, asgn);
  check_dual(msums, dual);

  lapjv_augment(msums, asgn, dual, col, free);
  printf("=== after lapjv_augment ===\n");
  print_mat("msums", msums);
  printf("free->size: %d\n", free->size);
  print_dual(npart, dual);
  print_assign(npart, asgn);
  /* check_dual(msums, dual); */

  lapjv_finalize(msums, asgn, dual);
  printf("=== after lapjv_finalize ===\n");
  printf("free->size: %d\n", free->size);
  print_mat("msums", msums);
  print_dual(npart, dual);
  print_assign(npart, asgn);
  /* check_dual(msums, dual); */


  for (int i = 0; i < npart; i++)
    perm[i] = asgn->col_at[i];

  /* print_mat("lapjv::msums", msums); */

  del_free(free);
  del_col(col);
  del_assign(asgn);
  del_dual(dual);
  del_mat(msums);

  printf("LAPJV::END\n");
}

void test_mark_unmark() {
  for (int i = -1; i < 4; i++) {
    printf("i: %d, mark(i): %d, mark(mark(i)): %d, unmark: %d\n", i, lapjv_mark(i),
           lapjv_mark(lapjv_mark(i)), lapjv_unmark(lapjv_mark(i)));
    assert(lapjv_mark(i) == lapjv_mark(lapjv_mark(i)));
    if (i != kUnassigned) assert(lapjv_mark(i) < kUnassigned);
    else assert(i == lapjv_mark(i));
    assert(lapjv_unmark(lapjv_mark(i)) == i);
  }
}

int main(int argc, char *argv[]) {
  printf("hi\n");

  test_mark_unmark();

  assert(argc == 2);
  int npart;
  int *sums = alloc_read_comm_sums(argv[1], &npart);

  int *perm = (int *)malloc(npart * sizeof(*perm));
  assert(perm != NULL);

  make_perm(perm, sums, npart);
  print_arr("1perm: ", npart, perm);

  lapjv(sums, npart, perm);
  print_arr("2perm: ", npart, perm);

  printf("npart: %d\n", npart);
  free(perm);
  free(sums);
  return 0;
}
