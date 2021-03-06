//*************** (Main computational functions)
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "lib.h"

void print_levels(level_t *lg) {
  int n = lg->pg->g->n;
  int i, max, min;
  int *l = lg->level;

  max = min = l[0];
  for (i=1; i< n; i++) {
    if (max < l[i])
      max = l[i];
    if (min > l[i])
      min = l[i];
  }

  printf("level min=%d max=%d\n", min, max);
}

int main(int argc, char **argv) {

  if (argc != 6 && argc != 7) {
    fprintf(stderr, "usage: %s mode graph coord part [level] out\n",
	    argv[0]); // The section to ensure correct format to call out a function
    fprintf(stderr, "valid mode: part_c, level_c, level and weight\n"); 
    exit(1);//graph is loaction of graph file. coord is x. Part is Partitioned graph. Out will be output file. [level] is optional 
  }

  FILE *fi = fopen(argv[2], "r"); // open the graph file if not then exit the program
  if (fi == NULL) {
    fprintf(stderr, "cannot open %s\n", argv[2]);
    exit(1);
  }
  
  crs0_t *g = read_crs(fi); // Read the graph and make a crs data structure.//*****************************

  coord_t *cg = NULL;

  if (strcmp(argv[1], "level_c") == 0 || // For now this part is ignored since we are not using level_c and weight_c
      strcmp(argv[1], "part_c") == 0) {
    FILE *fc = fopen(argv[3], "r");
    if (fc == NULL) {
      fprintf(stderr, "cannot open %s\n", argv[3]);
      exit(1);
    }  
    cg = new_coord(g);
    read_coord(fc, cg); 
  }

  FILE *fp = fopen(argv[4], "r");
  if (fp == NULL) { // we will store partitioned graph here
    fprintf(stderr, "cannot open %s\n", argv[4]);
    exit(1);
  }
  part_t *pg = new_part(g);
  read_part(fp, pg);// partitioned graph read function*********************************

  level_t *lg_org = NULL;

  if (argc == 7) { // If phase is >0
    FILE *fl = fopen(argv[5], "r");
    if (fl == NULL) {
      fprintf(stderr, "cannot open %s\n", argv[5]);
      exit(1);
    }
    lg_org = new_level(pg);
    read_level(fl, lg_org); // ********************************Will be used to update level in function below.
  }

  FILE *fo = fopen(argv[argc - 1], "w");
  if (fo == NULL) {
    fprintf(stderr, "cannot open %s\n", argv[argc - 1]);
    exit(1);
  }

  level_t *lg = new_level(pg);
  if (lg_org == NULL)
    comp_level(lg); // If phase 0 then we compute the newely made base level // ************************
  else
    update_level(lg, lg_org); // Else we will update the new level lg with the help lg_org // ****************************

  wcrs_t *wg = new_wcrs(g);
  level2wcrs(lg, wg); //************************ Used to calculate edge weights with the help of level

  if (strcmp(argv[1], "part_c") == 0)
    write_part_c(fo, pg, cg);// Not used right now
  else if (strcmp(argv[1], "level_c") == 0) {
    print_levels(lg); // Not used yet
    write_level_c(fo, lg, cg);
  } else if (strcmp(argv[1], "level") == 0)
    write_level(fo, lg); // ************************
  else if (strcmp(argv[1], "weight") == 0)
    write_wcrs(fo, wg); // *************************
  else {
    fprintf(stderr, "unknown mode %s\n", argv[1]);
    fprintf(stderr, "valid mode: part_c, level_c, level and weight\n");
    exit(1);
  }

  return 0;
}
