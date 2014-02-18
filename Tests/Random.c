#include <stdlib.h>

int main(){
	int x;
	int z;

	int *y;

	x = 5;
	z = x;
	
	y = malloc(sizeof(int));
	*y = 5;
	free(y);

	return 0;
}
