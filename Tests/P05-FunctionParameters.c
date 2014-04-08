// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

void PrintInt(int *x){
  printf("%d\n", *x);
}

int main(){
  int *x;
  x = malloc(sizeof(int));
  *x = 74;
  printf("Clean\n");
  // CHECK: Clean

  PrintInt(x);
  // CHECK: 74
  // CHECK-NOT: OutOfBounds

  printf("End\n");
  // CHECK: End

  free(x);
	return 0;
}
