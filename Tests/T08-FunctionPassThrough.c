// RUN: ./pointerAnalysis.sh %s 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int *Id(int *x){
  return x;
}

int main(){
  int *a;
  int *b;

  b = Id(a);

  b++;
	return 0;
}

// CHECK: (x, 0) set to (a, 0)
// CHECK: Added (b, 0) set to (x, 0)
// CHECK: (x, 0): Sequential
// CHECK: (a, 0): Sequential
// CHECK: (b, 0): Sequential
