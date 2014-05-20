// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

typedef struct{
  int *x;
} MyStruct;

int main(){
  MyStruct *S = malloc(24);
  S->x = NULL;

  printf("%p\n", S->x);
  // CHECK-NOT: Null
}
