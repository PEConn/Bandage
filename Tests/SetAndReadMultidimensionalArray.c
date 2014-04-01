// RUN: ./runOn.sh SetAndReadMultidimensionalArray | /pool/users/pc424/llvm_build/bin/FileCheck %s

#include <stdio.h>

int main(){
  int arr[2][3][4];
  printf("Start\n");
  // CHECK: Start

  arr[0][0][0] = 0;
  arr[1][0][0] = 1;
  arr[0][1][0] = 1;
  arr[0][0][1] = 1;
  arr[1][1][1] = 3;
  arr[1][2][3] = 6;

  // CHECK-NOT: OutOfBounds
  printf("%d\n", arr[0][0][0]); // CHECK: 0
  printf("%d\n", arr[1][0][0]); // CHECK: 1
  printf("%d\n", arr[0][1][0]); // CHECK: 1
  printf("%d\n", arr[0][0][1]); // CHECK: 1
  printf("%d\n", arr[1][1][1]); // CHECK: 3
  printf("%d\n", arr[1][2][3]); // CHECK: 6


  arr[0][3][0] = 0;
  // CHECK: OutOfBounds

  arr[2][0][0] = 0;
  // CHECK: OutOfBounds
  
  arr[0][0][4] = 0;
  // CHECK: OutOfBounds

  printf("Done\n");
  // CHECK: Done

	return 0;
}
