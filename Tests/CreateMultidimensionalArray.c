// RUN: ./runOn.sh CreateMultidimensionalArray| /pool/users/pc424/llvm_build/bin/FileCheck %s

#include <stdio.h>

int main(){
  int arr1[2][4];

  int arr2[5][4][3][2];

  int arr3[4];

  printf("Done\n");
  // CHECK: Done

	return 0;
}
