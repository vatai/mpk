/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-12-13
///
/// Jonker-Volgenant algorithm - buggy for now.
///
/// Paper: https://link.springer.com/article/10.1007%2FBF02278710

#ifndef _LAPJV_H_
#define _LAPJV_H_

#ifdef __cplusplus
extern "C" {
#endif

/// Implement the algorithm described in this paper:
/// https://link.springer.com/article/10.1007%2FBF02278710
///
/// @param[in] sums Problem as an `npart` by `npart` matrix (row major).
///
/// @param[in] npart Size of the problem.
///
/// @param[out] perm Result in the form of a permutation.
void lapjv(int *sums, int npart, int *perm);

#ifdef __cplusplus
}
#endif

#endif
