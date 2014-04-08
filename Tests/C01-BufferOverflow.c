// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

int main(){
  printf("Clean\n");
  // CHECK: Clean

  char *w1 = malloc(sizeof(char)*4);
  char *w2 = malloc(sizeof(char)*2);

  w1 = w1+3;
  *w1 = 'x';
  w1 = w1-3;

  // CHECK-NOT: OutOfBounds
  printf("Dirty\n");
  // CHECK: Dirty
  for(; *w1 == *w2; w1++, w2++){
    if (*w1 != '\0')
      break;
  }
  // CHECK: OutOfBounds
  // CHECK: OutOfBounds
  // CHECK: OutOfBounds
  // CHECK: OutOfBounds

  free(w1);
  free(w2);
  printf("Finished\n");
  // CHECK: Finished
  return 0;

}
