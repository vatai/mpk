#ifndef _UTIL_H_
#define _UTIL_H_

#include "lib.h"

void prn_k(level_t *lg, fp_t *b, int k);
void prn_lvl(level_t *lg, fp_t *b, idx_t lvl);
void prn_prt(part_t *pg);
void octave_check(char*fn, fp_t* b, int N, int k);
void print_levels(level_t *lg);
void debug(level_t* lg, fp_t* bb, idx_t k_steps, int t);


#endif // _UTIL_H_
