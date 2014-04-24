#include <stdio.h>

int main(){
  printf("");

  int **x;
  *x = malloc(sizeof(int*));
  //x = malloc(sizeof(int));
  printf("1");
  //int *y = *x;
  printf("2");
  //int z = **x;

  return 0;
}
