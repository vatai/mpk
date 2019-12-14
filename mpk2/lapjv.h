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

/// Input: `sums` points to an array of `npart * npart` number of
/// integers.  The value at `sums[i * npart + j]` is the number of
/// communication from old partition `i` to new partition `j`.
///
/// Output: `perm`, pointer to an array of `npart` integers, which are
/// filled with the values `0, 1, ... npart-1` (i.e.  a permuation)
/// which results in the trace of `sums` (as a matrix) being maximal,
/// i.e. resulting in optimal communication.
void lapjv(int *sums, int npart, int *perm);

#ifdef __cplusplus
}
#endif

#endif
