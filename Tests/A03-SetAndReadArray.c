// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>

int main(){
	int arr[5];

	printf("Clean\n");
	// CHECK: Clean
	for(int i=0; i<5; i++)
		arr[i] = i;

	// CHECK-NOT: OutOfBounds

	printf("Dirty\n");
	// CHECK: Dirty
	for(int i=0; i<6; i++)
		printf("%d\n", arr[i]);
	// CHECK: OutOfBounds

	return 0;
}
