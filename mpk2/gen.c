// This is a matrix generator file. No compuation is performed here.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(int argc, char **argv) {
  if (argc != 4) {
  usage:
    fprintf(stderr, "usage: %s type size ghead\n", argv[0]);
    fprintf(stderr, "type: m5p m7p m9p t5p t7p t9p cube\n");
    exit(1);
  }

  int size = -1;
  sscanf(argv[2], "%d", &size);
  if (size <= 0) {
    fprintf(stderr, "cannot get size (%d)\n", size);
    goto usage;
  }

  char s[10 + strlen(argv[3])];
  sprintf(s, "%s.g0", argv[3]);
  FILE *f = fopen(s, "w");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", s);
    goto usage;
  }

  sprintf(s, "%s.co", argv[3]);
  FILE *fp = fopen(s, "w");
  if (f == NULL) {
    fprintf(stderr, "Cannot open %s\n", s);
    goto usage;
  }

  int n = size;

  if (strcmp(argv[1], "m5p") == 0) {
    fprintf(f, "%d %d\n", n * n, 2 * (n-1) * n);

    int i, j;
    for (i=0; i< n; i++)
      for (j=0; j< n; j++) {
	if (i > 0)
	  fprintf(f, "%d ", (i-1) * n + j + 1);
	if (i < n-1)
	  fprintf(f, "%d ", (i+1) * n + j + 1);
	if (j > 0)
	  fprintf(f, "%d ", i * n + j-1 + 1);
	if (j < n-1)
	  fprintf(f, "%d ", i * n + j+1 + 1);
	fprintf(f, "\n");
	
	fprintf(fp, "%d %d\n", i, j);
      }

  } else if (strcmp(argv[1], "t5p") == 0) {
    fprintf(f, "%d %d\n", n * n, 2 * n * n);

    int i, j;
    for (i=0; i< n; i++)
      for (j=0; j< n; j++) {
	if (i > 0)
	  fprintf(f, "%d ", (i-1) * n + j + 1);
	else
	  fprintf(f, "%d ", (n-1) * n + j + 1);
	if (i < n-1)
	  fprintf(f, "%d ", (i+1) * n + j + 1);
	else
	  fprintf(f, "%d ", 0 * n + j + 1);
	if (j > 0)
	  fprintf(f, "%d ", i * n + j-1 + 1);
	else
	  fprintf(f, "%d ", i * n + n-1 + 1);
	if (j < n-1)
	  fprintf(f, "%d ", i * n + j+1 + 1);
	else
	  fprintf(f, "%d ", i * n + 0 + 1);
	fprintf(f, "\n");
	
	fprintf(fp, "%d %d\n", i, j);
      }

  } else if (strcmp(argv[1], "m7p") == 0) {
    fprintf(f, "%d %d\n", n * n, 2 * (n-1) * n + (n-1) * (n-1));

    int i, j;
    for (i=0; i< n; i++)
      for (j=0; j< n; j++) {
	if (i > 0)
	  fprintf(f, "%d ", (i-1) * n + j + 1);
	if (i < n-1)
	  fprintf(f, "%d ", (i+1) * n + j + 1);
	if (j > 0)
	  fprintf(f, "%d ", i * n + j-1 + 1);
	if (j < n-1)
	  fprintf(f, "%d ", i * n + j+1 + 1);

	if (i > 0 && j > 0)
	  fprintf(f, "%d ", (i-1) * n + j-1 + 1);
	if (i < n-1 && j < n-1)
	  fprintf(f, "%d ", (i+1) * n + j+1 + 1);
	fprintf(f, "\n");
	
	fprintf(fp, "%d %d\n", i, j);
      }

  } else if (strcmp(argv[1], "m9p") == 0) {
    fprintf(f, "%d %d\n", n * n, 2 * (n-1) * n + 2 * (n-1) * (n-1));

    int i, j;
    for (i=0; i< n; i++)
      for (j=0; j< n; j++) {
	if (i > 0)
	  fprintf(f, "%d ", (i-1) * n + j + 1);
	if (i < n-1)
	  fprintf(f, "%d ", (i+1) * n + j + 1);
	if (j > 0)
	  fprintf(f, "%d ", i * n + j-1 + 1);
	if (j < n-1)
	  fprintf(f, "%d ", i * n + j+1 + 1);

	if (i > 0 && j > 0)
	  fprintf(f, "%d ", (i-1) * n + j-1 + 1);
	if (i < n-1 && j < n-1)
	  fprintf(f, "%d ", (i+1) * n + j+1 + 1);
	if (i > 0 && j < n-1)
	  fprintf(f, "%d ", (i-1) * n + j+1 + 1);
	if (i < n-1 && j > 0)
	  fprintf(f, "%d ", (i+1) * n + j-1 + 1);
	fprintf(f, "\n");
	
	fprintf(fp, "%d %d\n", i, j);
      }

  } else if (strcmp(argv[1], "cube") == 0) {
    fprintf(f, "%d %d\n", n * n * n, 3 * (n-1) * n * n);

    int i, j, k;
    for (i=0; i< n; i++)
      for (j=0; j< n; j++)
	for (k=0; k< n; k++) {
	  if (i > 0)
	    fprintf(f, "%d ", ((i-1) * n + j) * n + k + 1);
	  if (i < n-1)
	    fprintf(f, "%d ", ((i+1) * n + j) * n + k + 1);
	  if (j > 0)
	    fprintf(f, "%d ", (i * n + j-1) * n + k + 1);
	  if (j < n-1)
	    fprintf(f, "%d ", (i * n + j+1) * n + k + 1);
	  if (k > 0)
	    fprintf(f, "%d ", (i * n + j) * n + k-1 + 1);
	  if (k < n-1)
	    fprintf(f, "%d ", (i * n + j) * n + k+1 + 1);
	  fprintf(f, "\n");
	
	}

  } else {
    fprintf(stderr, "unknown graph type %s\n", argv[1]);
    exit(1);
  }

  fclose(f);
  fclose(fp);

  return 0;
}
