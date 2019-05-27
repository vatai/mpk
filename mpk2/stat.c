#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lib.h"

int main(int argc, char **argv) {

  if (argc != 3 && argc != 4) {
  usage:
    fprintf(stderr, "usage: %s levfile [skfile] level\n", argv[0]);
    fprintf(stderr, "  levfile can be \"none\"\n");
    exit(1);
  }

  int level = 0;
  sscanf(argv[argc-1], "%d", &level);
  if (level < 1) {
    fprintf(stderr, "level must be positive\n");
    exit(1);
  }

  if (argc == 3) {

    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
      fprintf(stderr, "cannot open %s\n", argv[1]);
      exit(1);
    }

    int min = -1, max = -1;
    int n = 0, sum = 0;
    char line[10];
    while (1) {
      line[0] = '\0';
      fgets(line, 10, f);

      if (feof(f))
	break;

      n ++;
      int lev;
      sscanf(line, "%d", &lev);

      if (lev > level)
	lev = level;

      if (min < 0 || lev < min)
	min = lev;
      if (max < 0 || max < lev)
	max = lev;
      sum += lev;
    }

    printf("%d nodes, min %d, max %d, average %f\n",
	   n, min, max, sum / (double) n);

  } else if (argc == 4) {

    /* first get n and np from skirt file */

    FILE *fs = fopen(argv[2], "r");
    if (fs == NULL) {
      fprintf(stderr, "cannot open %s\n", argv[2]);
      exit(1);
    }

    char line[100];
    fgets(line, 100, fs);

    int n, np;
    sscanf(line, "skirt %d nodes, %d parts", &n, &np);

    /* then read levels */

    int lev[n];
    int nc = 0;

    if (strcmp(argv[1], "none") != 0) {

      FILE *f = fopen(argv[1], "r");
      if (f == NULL) {
	fprintf(stderr, "cannot open %s\n", argv[1]);
	exit(1);
      }

      int i;
      for (i=0; i< n; i++) {
	line[0] = '\0';
	fgets(line, 100, f);
	sscanf(line, "%d", &lev[i]);
	
	if (lev[i] > level)
	  lev[i] = level;
	nc += level - lev[i];
      }
    } else {

      int i;
      for (i=0; i< n; i++)
	lev[i] = 0;

    }

    /* then read skirts */

    int ns = 0;
    while (1) {
      line[0] = '\0';
      fgets(line, 100, fs);
      if (feof(fs) || line[0] == '\0') break;

      if (line[0] == 'p' || line[0] == 'S')
	;			/* partition line */
      else {
	int j, d;
	sscanf(line, "%d %d", &j, &d);
	assert(0 <= j && j < n);
	assert(0 <= d && d <= level);

	if (d <= lev[j]) {
	  fprintf(stderr, "error? node %d, lev %d, dep %d\n", j, lev[j], d);
	}
	ns += d - lev[j];
      }
    }

    printf("%d nodes, avr remain %f, avr comp %f, avr ovhd %f\n", 
	   n, nc / (double)n, ns / (double)n, (ns-nc)/(double)n);
  }

  return 0;
}
