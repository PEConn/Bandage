// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int *global;

int main(){
  int *local;
  local = global;

  return 0;
}
