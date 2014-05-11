#include <stdio.h>

typedef struct Point{
  int x, y;
} Point;

Point GetOrigin(){
  Point p;
  p.x = 0;
  p.y = 0;
  return p;
}
int main(){
  Point origin = GetOrigin();
  printf("(%d, %d)", origin.x, origin.y);
  return 0;
}
