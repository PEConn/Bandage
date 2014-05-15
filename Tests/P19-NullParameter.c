// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

void DoSomething(int *a){ } 

int main(){
  DoSomething(NULL);
  printf("Compiled\n");
  // CHECK: Compiled
  return 0;
}
