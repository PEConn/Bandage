// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

int main(){
  printf("Start\n");
  // CHECK: Start

  FILE *pFile = tmpfile();

  printf("Clean\n");
  // CHECK: Clean
  fclose(pFile);
  // CHECK: Unset

  return 0;
}
