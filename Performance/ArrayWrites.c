#include <stdio.h>

int main(){
	printf("");
	int arr[100000];
	
	for(int i=0; i<1000; i++){
		for(int j=0; j<100000; j++){
			arr[j] = -j;
		}
	}
	return 0;
}
