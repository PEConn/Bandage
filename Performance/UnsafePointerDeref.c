#include <stdlib.h>
#include <stdio.h>
#include "Timing.h"

int main(){
	printf("");
  int x;
  int *y;
  y = malloc(sizeof(int));
  x = y[0];
	
  float t0 = TIME;
	for(int i=0; i<10000; i++){
		for(int j=0; j<100000; j++){
      x = *y;
		}
	}
  
  printf("%f\n", TIME - t0);
  free(y);
	return 0;
}
