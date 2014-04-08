// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int main(){
	printf("Start\n");
	// CHECK: Start

	int *x;
	x = malloc(sizeof(int)*2);


  x[0] = 1;
  x[1] = 1;

  printf("%d\n", x[0]);

  printf("Dirty\n");
  // CHECK-NOT: OutOfBounds
  // CHECK: Dirty

  x[2] = 1;
  // CHECK: OutOfBounds

  printf("Finished\n");
  // CHECK: Finished

	return 0;
}
