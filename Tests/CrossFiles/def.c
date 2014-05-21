#include <stdio.h>
#include <stdlib.h>

int *ptr;

void External(){
  ptr = malloc(sizeof(int));
  *ptr = 5;


  printf("External\n");
}
