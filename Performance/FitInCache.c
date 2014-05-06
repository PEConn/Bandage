#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "Timing.h"

int main(){
  int no_pointers = INT_MAX/300;
  int **a = malloc(no_pointers*24);

  for(int i=0; i<no_pointers; i++)
    a[i] = malloc(sizeof(int));

  int b;
  float t0 = TIME;
  for(int i = 0; i<1000000; i++)
    b = *a[i%no_pointers];
  float t1 = TIME;

  printf("%f\n", t1-t0);

	return 0;
}

