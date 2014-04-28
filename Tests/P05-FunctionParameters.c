// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

void PrintInt(int *x){
  x += 2;
  printf("%d\n", *x);
}

void DoBadStuff(int *x){
  x += 4;
  printf("%d\n", *x);
}

int main(){
  int *x;
  x = malloc(4 * sizeof(int));
  x[2] = 74;
  printf("Clean\n");
  // CHECK: Clean

  PrintInt(x);
  // CHECK: 74
  // CHECK-NOT: OutOfBounds
  
  printf("Dirty\n");
  // CHECK: Dirty

  DoBadStuff(x);
  // CHECK: OutOfBounds

  printf("End\n");
  // CHECK: End

  free(x);
	return 0;
}
