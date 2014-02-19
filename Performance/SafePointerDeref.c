#include <stdlib.h>
#include <stdio.h>

int main(){
	printf("");
  int x;
  int *y;
  y = malloc(sizeof(int));
	
	for(int i=0; i<1000; i++){
		for(int j=0; j<100000; j++){
      x = *y;
		}
	}
  free(y);
	return 0;
}
