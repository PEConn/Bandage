// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

typedef struct Point{
  int x;
  int y;
  int z;
} Point;

int main(){
  Point *a = malloc(sizeof(Point));
  a->x = 1;
  a->y = 2;
  a->z = 0;
  free(a);

  printf("Compiles\n");
  // CHECK: Compiles
  return 0;
}

