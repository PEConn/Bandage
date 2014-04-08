// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int main(){
	printf("Start\n");
	// CHECK: Start

	int *x;
	x = malloc(sizeof(int));

	*x = 6;
	printf("%d\n", *x);
	free(x);
	// CHECK-NOT: OutOfBounds
	// CHECK: 6

	return 0;
}
