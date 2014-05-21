#include <stdio.h>
#include <stdlib.h>

extern int *ptr;

void SayHello(){
  printf("Hello\n");
}

void External();

int main(){
  External();
  printf("%d\n", *ptr);
  ptr++;
  printf("%d\n", *ptr);
  free(ptr);
  return 0;
}
