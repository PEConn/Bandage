// RUN: ./runOn.sh PointerArithmetic | /pool/users/pc424/llvm_build/bin/FileCheck %s
// RUN: rm PointerArithmetic.bc PointerArithmetic_ban.bc PointerArithmetic_ban.s PointerArithmetic
#include <stdio.h>
#include <stdlib.h>

int main(){
  printf("Clean\n");
  // CHECK: Clean
  int *x;

  x = malloc(2*sizeof(int));

  *x = 2;
  printf("%d\n", *x);
  // CHECK-NOT: OutOfBounds
  // CHECK: 2
  x++;
  *x = 3;
  printf("%d\n", *x);
  // CHECK-NOT: OutOfBounds
  // CHECK: 3
  x++;
  *x = 4;
  // CHECK: OutOfBounds

  free(x);
  return 0;

}
