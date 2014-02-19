#include <stdlib.h>
#include <stdio.h>

int main(){
	printf("");
  int x;
  int *y;
	
	for(int i=0; i<1000; i++){
		for(int j=0; j<1000; j++){
      y = malloc(sizeof(int));
      free(y);
		}
	}
	return 0;
}
