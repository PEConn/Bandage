// RUN: ./runOn.sh CreateArray| /pool/users/pc424/llvm_build/bin/FileCheck %s

#include <stdio.h>

int main(){
	int nums[5];
  printf("Done\n");
  // CHECK: Done

	return 0;
}
