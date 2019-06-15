#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define UFACTOR 100

int main(int argc, char **argv) {

  if (argc != 5) {
  usage:
    fprintf(stderr, "usage: %s ghead npart nlevel nphase\n", argv[0]);
    exit(1);  // To check if current input format to implement else return the format
  }

  char *ghead = argv[1]; /* ghead ~= name */

  int npart = 0;
  sscanf(argv[2], "%d", &npart);
  if (npart <= 1) {
    fprintf(stderr, "npart = %d must be more than one\n", npart);
    goto usage;
  }

  int nlevel = 0;
  sscanf(argv[3], "%d", &nlevel);
  if (nlevel <= 1) {
    fprintf(stderr, "nlevel = %d must be more than one\n", nlevel);
    goto usage;
  }

  int nphase = 0;
  sscanf(argv[4], "%d", &nphase);
  if (nphase < 0) {
    fprintf(stderr, "nphase = %d cannot be nevative\n", nphase);
    goto usage; // 0 Phase is PA1
  }

  char line[100 + 5 * strlen(ghead)];
  char dir[100 + strlen(ghead)];
  int res;

  sprintf(dir, "%s_%d_%d_%d", ghead, npart, nlevel, nphase);
  sprintf(line, "mkdir %s", dir);
  printf("%s\n", line); /* To make a directory with name as ghead_npart_nlevel_nphase*/
  res = system(line); /* printing the line and executing it on terminal*/
  if (res != 0) exit(res);

  sprintf(line, "cp %s.g0 %s/g0", ghead, dir);
  printf("%s\n", line);
  res = system(line); /* Copying the ghead.g0 to the newely made folder */
  if (res != 0) exit(res);

  sprintf(line, "echo graph %s, npart %d, nlevel %d, "
	  "nphase %d > %s/log", ghead, npart, nlevel, nphase, dir);
  printf("%s\n", line);
  res = system(line);
  if (res != 0) exit(res);

  int phase;
  for (phase = 0; phase < nphase; phase ++) {  // Implementing all nphases

    sprintf(line, "./gpmetis -ufactor=%d %s/g%d %d > %s/metis.log%d",
	    UFACTOR, dir, phase, npart, dir, phase);
    printf("%s\n", line);  // ufactor denotes the value of maximum imbalance factor among partitions.
    res = system(line);    /* using gpmetis to do the partition*/
    if (res != 0) exit(res);

    if (phase == 0)
      sprintf(line, "./comp level %s/g0 x %s/g%d.part.%d %s/l%d",
	      dir, dir, phase, npart, dir, phase); // Calling comp.c with required string inputs,level is the mode for comp. Separate for phase = 0.
    else
      sprintf(line, "./comp level %s/g0 x %s/g%d.part.%d %s/l%d %s/l%d",
	      dir, dir, phase, npart, dir, phase-1, dir, phase);
    printf("%s\n", line);
    res = system(line);
    if (res != 0) exit(res);

    sprintf(line, "./stat %s/l%d %d >> %s/log; tail -1 %s/log",
	    dir, phase, nlevel, dir, dir);
    printf("%s\n", line);
    res = system(line);
    if (res != 0) exit(res);

    if (phase == 0)
      sprintf(line, "./comp weight %s/g0 x %s/g%d.part.%d %s/g%d",
	      dir, dir, phase, npart, dir, phase+1); // Calling comp.c again but this time to do computations for weight
    else
      sprintf(line, "./comp weight %s/g0 x %s/g%d.part.%d %s/l%d %s/g%d",
	      dir, dir, phase, npart, dir, phase-1, dir, phase+1);
    printf("%s\n", line);
    res = system(line);
    if (res != 0) exit(res);
  }

  if (nphase > 0) {

    sprintf(line, "./skirt %d %s/g0 x %s/g0.part.%d %s/l%d %s/s%d",
	    nlevel, dir, dir, npart, dir, phase-1, dir, phase);
    printf("%s\n", line);
    res = system(line); // Skirt used to fill the leftover gap.
    if (res != 0) exit(res);

    sprintf(line, "./stat %s/l%d %s/s%d %d >> %s/log; tail -1 %s/log",
	    dir, phase-1, dir, phase, nlevel, dir, dir);
    printf("%s\n", line);
    res = system(line);
    if (res != 0) exit(res);

  } else {			/* nphase = 0, i.e. PA1 */

    phase = 0;
    sprintf(line, "./gpmetis -ufactor=%d %s/g%d %d > %s/metis.log%d",
	    UFACTOR, dir, phase, npart, dir, phase);
    printf("%s\n", line);
    res = system(line);
    if (res != 0) exit(res);

    sprintf(line, "./skirt %d %s/g0 x %s/g0.part.%d %s/s%d",
	    nlevel, dir, dir, npart, dir, phase);
    printf("%s\n", line);
    res = system(line);
    if (res != 0) exit(res);

    sprintf(line, "./stat none %s/s%d %d >> %s/log; tail -1 %s/log",
	    dir, phase, nlevel, dir, dir);
    printf("%s\n", line);
    res = system(line);
    if (res != 0) exit(res);

  }

  return 0;
}
