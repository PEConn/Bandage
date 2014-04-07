// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

typedef struct Point{
  float x;
  float y;
  float z;
} Point;

int main(){
  Point *a;
  a->x = 1f;
  a->y = 2f;
  a->z = 0f;

  printf("Compiles\n");
  // CHECK: Compiles
  return 0;
}

