/*  Author: Emil VATAI(emil.vatai@gmail.com) */
/*  Date: 2019-08-18 */

#include <stdio.h>

int main(int argc, char *argv[])
{
  for (int n = 1; n < 20; n++) {
    double a = 1.0 / n;
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
      sum += a;
    }
    if (sum != 1.0) printf("broke at %d with sum=%le\n", n, sum);
  }
  return 0;
}
