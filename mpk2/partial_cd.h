#ifndef _PARTIAL_H_
#define _PARTIAL_H_

struct partial_cd {
  const char *dir;
  int rank;
};

struct partial_cd *
new_partial_cd(char *dir, int rank);

void read_pcd(struct partial_cd* pcd);

#endif
