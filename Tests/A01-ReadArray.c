// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>

int main(){
	int nums[5];

	printf("Clean\n");
	// CHECK: Clean
	printf("%d\n", nums[0]);
	printf("%d\n", nums[4]);
	// CHECK-NOT: OutOfBounds

	printf("Dirty\n");
	// CHECK: Dirty
	printf("%d\n", nums[-1]);
	// CHECK: OutOfBounds
	printf("%d\n", nums[5]);
	// CHECK: OutOfBounds

	return 0;
}
