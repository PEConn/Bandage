// RUN: ./pointerAnalysis.sh %s 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int main(){
  int *a;
  int *b;

  b = a + 4;

  int *c;
  c++;

  // Seq can be converted into Safe
  int *d;
  d = c;

  // Safe cannot be converted into Safe
  int *e;
  c = e;

	return 0;
}

// CHECK: (a, 0): Sequential
// CHECK: (b, 0): Safe
// CHECK: (c, 0): Sequential
// CHECK: (d, 0): Safe
// CHECK: (e, 0): Sequential
