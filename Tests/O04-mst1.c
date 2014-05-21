// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

typedef struct Vertex{
  int x, y, z;
} Vertex;

Vertex *v;

int main(){
  Vertex *tmp;
  v = tmp;
  printf("Compiled\n");
  // CHECK: Compiled
  return 0;
}
