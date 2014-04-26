// RUN: ./runOn.sh %s 

#include <stdlib.h>
int main(){
  int *a;
  free(a);
}
