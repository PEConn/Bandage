// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

typedef struct Point{
  int x;
  int y;
} Point;

typedef struct Line{
  Point a;
  Point b;
} Line;

int main(){
	printf("Start\n");
	// CHECK: Start
  Line line;

  line.a.x = 2;
  line.a.y = 5;
  line.b.x = 6;
  line.b.y = 2;

  int diffX = line.a.x - line.b.x;
  int diffY = line.a.y - line.b.y;
	// CHECK-NOT: OutOfBounds

  int distSq = diffX*diffX + diffY*diffY;

  printf("%d\n", distSq);
  // CHECK: 25

	// CHECK-NOT: OutOfBounds
  printf("Finished\n");
  // CHECK: Finished

	return 0;
}
