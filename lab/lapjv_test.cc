#include "../mpk2/lapjv.h"
#include <assert.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// /// /// //

static void print_arr(const char *name, int n, int *vec) {
  printf("%s: ", name);
  for (int i = 0; i < n; i++)
    printf("%d, ", vec[i]);
  printf("\n");
}

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

// /// Tests ///

/* void test_mark_unmark() { */
/*   for (int i = -1; i < 4; i++) { */
/*     printf("i: %d, mark(i): %d, mark(mark(i)): %d, unmark: %d\n", i,
 * lapjv_mark(i), */
/*            lapjv_mark(lapjv_mark(i)), lapjv_unmark(lapjv_mark(i))); */
/*     assert(lapjv_mark(i) == lapjv_mark(lapjv_mark(i))); */
/*     if (i != kUnassigned) assert(lapjv_mark(i) < kUnassigned); */
/*     else assert(i == lapjv_mark(i)); */
/*     assert(lapjv_unmark(lapjv_mark(i)) == i); */
/*   } */
/* } */

int calc_cost(const int n, int *m, int *row_at) {
  int result = 0;
  for (int j = 0; j < n; j++) {
    const int i = row_at[j];
    result += m[i * n + j];
  }
  return result;
}

/// Return 1 if arrays are equal, 0 if arrays differ.
int arr_eq(const int n, int *arr1, int *arr2) {
  for (int i = 0; i < n; i++)
    if (arr1[i] != arr2[i])
      return 0;
  return 1;
}

void make_input(const int n, int *input) {
  for (int i = 0; i < n; i++)
    input[i] = rand() % 10000;
}

int test_core(const int n, int *input, int *buf1, int *buf2) {
  make_input(n * n, input);
  make_perm(buf1, input, n);
  lapjv(input, n, buf2);
  return calc_cost(n, input, buf1) == calc_cost(n, input, buf2);
}

void test_loop(const int count, const int n) {
  int size = n * n;
  int *input = (int *)malloc(size * sizeof(int));
  int *buf1 = (int *)malloc(n * sizeof(int));
  int *buf2 = (int *)malloc(n * sizeof(int));

  for (int i = 0; i < count; i++) {
    int result = test_core(n, input, buf1, buf2);
    if (!result) {
      print_arr("buf1", n, buf1);
      printf("cost1: %d\n", calc_cost(n, input, buf1));
      print_arr("buf2", n, buf2);
      printf("cost2: %d\n", calc_cost(n, input, buf2));
      // print_mat("input: ", &m);
      printf("Error!\n");
    } else
      printf("OK!\n");
    assert(result);
  }

  free(buf2);
  free(buf1);
  free(input);
}

TEST(LapjvTest, FirstTest) {
  const int count = 10000;
  const int n = 5;
  // srand(time);
  test_loop(count, n);
}
