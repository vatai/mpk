#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "partial_cd.h"

struct partial_cd *new_partial_cd(char *dir, int rank)
{
  struct partial_cd *pcd = (struct partial_cd *) malloc(sizeof(*pcd));
  assert(pcd != NULL);
  pcd->dir = dir;
  pcd->rank = rank;
  return pcd;
}

// This will be called in read_pcd()
void fileread_pcd(FILE *, struct partial_cd*);

// Get the filename, then open, read & close the file.
void read_pcd(struct partial_cd *pcd)
{
  char fname[1024];
  sprintf(fname, "%s/g0", pcd->dir);
  FILE *file = fopen(fname, "r");
  assert(file != NULL);
  fileread_pcd(file, pcd);
  fclose(file);
}

void fileread_pcd(FILE *file, struct partial_cd *pcd)
{

}
