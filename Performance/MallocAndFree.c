#include <stdlib.h>
#include <stdio.h>
#include "Timing.h"

int main(){
	printf("");
  int x;
  int *y;
	
  float t0 = TIME;
	for(int i=0; i<1000; i++){
		for(int j=0; j<1000; j++){
      y = malloc(sizeof(int));
      free(y);
		}
	}
  printf("%f\n", TIME - t0);
	return x;
}
