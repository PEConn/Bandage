// RUN: ./runOn.sh ArrayOfStructs | /pool/users/pc424/llvm_build/bin/FileCheck %s
// RUN: rm ArrayOfStructs.bc ArrayOfStructs_ban.bc ArrayOfStructs_ban.s ArrayOfStructs

#include <stdio.h>

struct Coord{
	float x;
	float y;
};

int main(){
	struct Coord arr[5];

	printf("Clean\n");
	// CHECK: Clean
	for(int i=0; i<5; i++)
		arr[i].x = arr[i].y;

	// CHECK-NOT: OutOfBounds

	printf("Dirty\n");
	// CHECK: Dirty
	arr[0].x = arr[5].x;
	arr[-1].y = arr[0].x;
	// CHECK: OutOfBounds
	// CHECK: OutOfBounds

	return 0;
}
