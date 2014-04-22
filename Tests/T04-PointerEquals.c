// RUN: ./pointerAnalysis.sh %s 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int main(){
  int ***x;
  int **y;
  int *z;

  x = malloc(sizeof(int ***));

  *x = y;   // CHECK: (x, 1) set to (y, 0)
  *y = z;   // CHECK: (y, 1) set to (z, 0)
  z = **x;  // CHECK: (z, 0) set to (x, 2)

  y = (int **)123;

  int *a;
  int *b;
  a = b;    // CHECK: (a, 0) set to (b, 0)
  *a = *b;  // CHECK: (a, 1) set to (b, 1)

  int *c;
  int d;
  *c = d;   // CHECK: (c, 1) set to (d, 0)
  c = &d;   // CHECK: (c, 0) set to (d, -1)

	return 0;
}

// CHECK: (x, 0): Safe
// CHECK: (x, 1): Dynamic
// CHECK: (x, 2): Dynamic
// CHECK: (y, 0): Dynamic
// CHECK: (y, 1): Dynamic
// CHECK: (z, 0): Dynamic
// CHECK: (a, 0): Safe
// CHECK: (a, 1): Safe
// CHECK: (b, 0): Safe
// CHECK: (b, 1): Safe
