#include <stdlib.h>
#include <stdio.h>
#include "Timing.h"

int main(){
  int *a, *b = malloc(sizeof(int));

  float t0 = TIME;
	for(int i=0; i<10000; i++)
		for(int j=0; j<100000; j++)
      a = b;
  float t1 = TIME;

  printf("%f\n", t1-t0);
	return 0;
}
