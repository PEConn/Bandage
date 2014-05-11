// RUN: ./pointerAnalysis.sh %s 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int main(){
  int ***x;
  int **y;
  int *z;

  x = malloc(sizeof(int ***));

  *x = y;   
  *y = z;   
  z = **x;  

  int p = 123;
  y = (int **)p;

  int *a;
  int *b;
  a = b;    
  *a = *b;  

  int *c;
  int d;
  *c = d;  
  c = &d;  

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
