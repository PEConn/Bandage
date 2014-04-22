// RUN: ./pointerAnalysis.sh %s 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int *X(){
  int *a;
  return a;
}

int *Y(){
  int **b;
  return *b;
}

int *Z(){
  int c;
  return &c;
}

int main(){
  int *d = X();
	return 0;
}

// CHECK: X returns (a, 0)
// CHECK: Y returns (b, 1)
// CHECK: Z returns (c, -1)
// CHECK: Added (d, 0) set to (a, 0)
