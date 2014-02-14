#include <stdio.h>

int main(){
	int nums[5];
	for(int i=0; i<6; i++){
		printf("%d\n", nums[i]);
	}
	return 0;
}

// RUN: ./runOn.sh ReadArray | /pool/users/pc424/llvm_build/bin/FileCheck %s
// RUN: rm ReadArray.bc ReadArray_ban.bc ReadArray_ban.s ReadArray

// CHECK: OutOfBounds
