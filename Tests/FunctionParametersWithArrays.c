// RUN: ./runOn.sh FunctionParametersWithArrays | /pool/users/pc424/llvm_build/bin/FileCheck %s

#include <stdio.h>
#include <stdlib.h>

void PrintInt(int[] x){
  printf("%d\n", x[0]);
}

int main(){
  int x[1];
  x[0] = 74;
  printf("Clean\n");
  // CHECK: Clean

  PrintInt(x);
  // CHECK: 74
  // CHECK-NOT: OutOfBounds

  printf("End\n");
  // CHECK: End

	return 0;
}
