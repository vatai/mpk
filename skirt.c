#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "lib.h"

int main(int argc, char **argv) {

  if (argc != 5 && argc != 6) {
    fprintf(stderr, "usage: %s level graph coord part [level]\n",
	    argv[0]);
    exit(1);
  }

  int level = -1;
  sscanf(argv[1], "%d", &level);
  if (level <= 0) {
    fprintf(stderr, "level should be positive\n");
    exit(1);
  }

  FILE *fi = fopen(argv[2], "r");
  if (fi == NULL) {
    fprintf(stderr, "cannot open %s\n", argv[2]);
    exit(1);
  }
  crs0_t *g = read_crs(fi);

#if 0
  FILE *fc = fopen(argv[3], "r");
  if (fc == NULL) {
    fprintf(stderr, "cannot open %s\n", argv[3]);
    exit(1);
  }  
  coord_t *cg = new_coord(g);
  read_coord(fc, cg);
#endif

  FILE *fp = fopen(argv[4], "r");
  if (fp == NULL) {
    fprintf(stderr, "cannot open %s\n", argv[4]);
    exit(1);
  }
  part_t *pg = new_part(g);
  read_part(fp, pg);

  level_t *lg_org = NULL;
  if (argc == 6) {
    FILE *fl = fopen(argv[5], "r");
    if (fl == NULL) {
      fprintf(stderr, "cannot open %s\n", argv[5]);
      exit(1);
    }
    lg_org = new_level(pg);
    read_level(fl, lg_org);
  }

  skirt_t *sg = new_skirt(pg);
  comp_skirt(sg);
  print_skirt_cost(sg, lg_org, level);

  return 0;
}
