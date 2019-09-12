#include <stdlib.h>

#include "partial_cd.h"

struct partial_cd *
new_partial_cd(char *dir, int rank)
{
  struct partial_cd *pcd = (struct partial_cd *) malloc(sizeof(*pcd));
  pcd->dir = dir;
  pcd->rank = rank;
  return pcd;
}
