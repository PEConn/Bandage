// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>

int main(){
	int nums[5];
  printf("Done\n");
  // CHECK: Done

	return 0;
}
