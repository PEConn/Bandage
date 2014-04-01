// RUN: ./runOn.sh SetArray | /pool/users/pc424/llvm_build/bin/FileCheck %s

#include<stdio.h>

int main(){
	int nums[5];

	printf("Clean\n");
	// CHECK: Clean
	nums[0] = 0;
	nums[4] = 0;
	// CHECK-NOT: OutOfBounds

	printf("Dirty\n");
	// CHECK: Dirty
	nums[5] = 0;
	// CHECK: OutOfBounds
	nums[-1] = 0;
	// CHECK: OutOfBounds

	return 0;
}
