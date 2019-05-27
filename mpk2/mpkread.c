#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lib.h"

mpk_t* new_mpk(crs0_t *g0, int npart, int nlevel, int nphase) {
  assert(g0 != NULL);

  mpk_t *mg = (mpk_t*) malloc(sizeof(mpk_t));
  assert(mg != NULL);

  mg->n = g0->n;
  mg->npart = npart;
  mg->nlevel = nlevel;
  mg->nphase = nphase;

  mg->g0 = g0;
  if (nphase == 0) {
    mg->plist = (part_t**) malloc(sizeof(part_t*));
    mg->llist = NULL;
  } else {
    mg->plist = (part_t**) malloc(sizeof(part_t*) * nphase);
    mg->llist = (level_t**) malloc(sizeof(level_t*) * nphase);
    int i;
    for (i=0; i< nphase; i++) {
      mg->plist[i] = NULL;
      mg->llist[i] = NULL;
    }
  }
  mg->sg = NULL;

  mg->tlist = (task_t*) malloc(sizeof(task_t) * npart * (nphase+1));
  assert(mg->tlist != NULL);
  int i;
  for (i=0; i< npart * (nphase+1); i++) {
    mg->tlist[i].n = -1;
    mg->tlist[i].idx = NULL;
  }

  mg->idxsrc = NULL;

  return mg;
}

mpk_t* read_mpk(char *dir) {
  printf("Reading MPK data from %s...\n", dir);

  char ghead[100];
  int npart, nlevel, nphase;
  int i, j;
  for (i=0, j= strlen(dir); i < 3 && j >= 0; j--)
    if (dir[j] == '_') {
      i ++;
      if (i == 3)
	break;
    }

  if (!(i == 3 && j >= 0 && dir[j] == '_')) {
    fprintf(stderr, "  unknwon directory naming %s\n", dir);
    fprintf(stderr, "  expecting ghead_npart_nlevel_nphase\n");
    exit(1);
  }

  npart = nlevel = nphase = -1;	/* guard */
  i = sscanf(dir+j, "_%d_%d_%d", &npart, &nlevel, &nphase);
  if (i != 3) {
    fprintf(stderr, "unknwon directory naming %s\n", dir);
    fprintf(stderr, "  expecting ghead_npart_nlevel_nphase\n");
    exit(1);
  }
  assert(npart > 0 && nlevel > 0 && nphase >= 0);

  char fname[100+strlen(dir)];
  sprintf(fname, "%s/g0", dir);
  printf("  reading %s...\n", fname);

  FILE *f;
  f = fopen(fname, "r");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }

  crs0_t *g0 = read_crs(f);

  // fclose(f) ... in read_crs

  mpk_t *mg = new_mpk(g0, npart, nlevel, nphase);

  int phase;
  for (phase = 0; phase < nphase; phase ++) {

    sprintf(fname, "%s/g%d.part.%d", dir, phase, npart);
    printf("  reading %s...\n", fname);

    f = fopen(fname, "r");
    if (f == NULL) {
      fprintf(stderr, "cannot open %s\n", fname);
      exit(1);
    }

    part_t *pg = new_part(g0);
    read_part(f, pg);

    fclose(f);

    mg->plist[phase] = pg;

    sprintf(fname, "%s/l%d", dir, phase);
    printf("  reading %s...\n", fname);

    f = fopen(fname, "r");
    if (f == NULL) {
      fprintf(stderr, "cannot open %s\n", fname);
      exit(1);
    }

    level_t *lg = new_level(pg);
    read_level(f, lg);

    //    fclose(f); ... in read_level

    mg->llist[phase] = lg;

    int nc, i;
    for (i=nc=0; i< mg->n; i++)
      nc += lg->level[i];
    printf("average level %f\n", nc/(double)mg->n);
  }

  if (nphase == 0) {

    sprintf(fname, "%s/g0.part.%d", dir, npart);
    printf("  reading %s...\n", fname);

    f = fopen(fname, "r");
    if (f == NULL) {
      fprintf(stderr, "cannot open %s\n", fname);
      exit(1);
    }

    part_t *pg = new_part(g0);
    read_part(f, pg);

    fclose(f);

    mg->plist[0] = pg;

  }

  sprintf(fname, "%s/s%d", dir, phase);
  printf("  reading %s...\n", fname);

  f = fopen(fname, "r");
  if (f == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    exit(1);
  }

  skirt_t *sk = new_skirt(mg->plist[0]);
  read_skirt(f, sk, nlevel);

  // fclose(f); ... in read_skirt

  mg->sg = sk;

  int cnt = mg->n * nlevel;
  for (i=0; i< npart * mg->n; i++)
    if (sk->levels[i] >= 0)
      cnt += nlevel - sk->levels[i] + 1;

  mg->idxsrc = (long*) malloc(sizeof(long) * cnt);
  mg->idxallocsize = cnt;
  assert(mg->idxsrc != NULL);

  printf("Reading MPK data %s done\n", dir);

  return mg;
}
