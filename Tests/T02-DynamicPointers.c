// RUN: ./pointerAnalysis.sh %s 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int main(){
  long int a = 123;
	int *x;

  x = (int *)a;

	return 0;
}

// CHECK: (x, 0): Dynamic
