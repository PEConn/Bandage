// RUN: ./pointerAnalysis.sh %s 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int main(){
  long int a = 123;
	int *x;

  x = (int *)a;
  // CHECK: Dynamic: x

	return 0;
}
