// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

typedef struct ContainsInt{
  int a;
} ContainsInt;

typedef struct ContainsPointer{
  int *a;
} ContainsPointer;

int main(){
  printf("%lu\n", sizeof(ContainsInt));
  // CHECK: 4
  printf("%lu\n", sizeof(ContainsPointer));
  // CHECK: 24
  return 0;
}
