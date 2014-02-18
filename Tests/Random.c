#include <stdlib.h>
#include <stdio.h>

int main(){
	int x;
	int z;

	int *y;

	x = 5;
	z = x;
	
	y = malloc(sizeof(int));
	*y = 5;
	printf("%d\n", *y);
	free(y);

	return 0;
}
