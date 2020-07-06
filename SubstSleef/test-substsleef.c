#include <math.h>

#define N 64

double a[N], b[N];

void foo() {
  for(int i=0;i<N;i++) a[i] = sin(b[i]);
}
