// RUN: ./pointerAnalysis.sh %s 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int main(){
  int ***x;
  int **y;
  int *z;

  *x = y;
  // CHECK: *x = y;
  *y = z;
  // CHECK: *y = z;
  z = **x;
  // CHECK: z = **x;

  int *a;
  int *b;
  a = b;
  // CHECK: a = b;
  *a = *b;
  // CHECK: *a = *b;

  int *c;
  int d;
  *c = d;
  // CHECK: *c = d;
  c = &d;
  // CHECK: c = &d;

	return 0;
}
