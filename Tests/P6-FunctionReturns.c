// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int* CreateInt(){
  int *x;
  x = malloc(sizeof(int));
  *x = 28;
  printf("%d\n", *x);
  return x;
}

int main(){
  int *x;
  printf("Clean\n");
  // CHECK: Clean

  x = CreateInt();
  // CHECK: 28

  printf("%d\n", *x);
  // CHECK-NOT: OutOfBounds
  // CHECK: 28

  printf("End\n");
  // CHECK: End

  free(x);
	return 0;
}
