#include <stdio.h>
#include "metis/include/metis.h"

int main()
{
  int i,j;
  printf("Hello metis\n");
  idx_t ov[1];
  idx_t xadj[] = {0, 2, 5, 8, 11, 13, 16, 20, 24, 28, 31, 33, 36, 39, 42, 44};
  idx_t adjncy[] = {
    /* 0 */  1, 5, /* 2 */  0, 2, 6, /* 5 */  1, 3, 7,
    /* 8 */ 2, 4, 8, /* 11 */ 3, 9, /* 13 */ 0, 6, 10,
    /* 16 */  1, 5, 7, 11, /* 20 */ 2, 6, 8, 12,
    /* 24 */ 3, 7, 9, 13, /* 28 */ 4, 8, 14, /* 31 */  5, 11,
    /* 33 */ 6, 10, 12, /* 36 */ 7, 11, 13, /* 39 */ 8, 12, 14,
    /* 42 */  9, 13};
  idx_t N = sizeof(xadj)/sizeof(*xadj)-1;
  idx_t nc = 1;
  idx_t part[N];
  idx_t k = 2;
  printf("nv %d\n",N);
  printf("ne %d\n",nc);
  METIS_PartGraphRecursive(&N, &nc, xadj, adjncy, NULL,
		      NULL, NULL, &k, NULL, NULL,
		      NULL, ov, part);
  for (i = 0; i < N; i++){
    //for (j = xadj[i]; j < xadj[i+1]; j++)
    printf("[%d] = %d, ", i, part[i]);
    printf("\n");
  }
  return 0;
}
