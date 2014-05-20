#include <stdio.h>
#include <stdlib.h>

#include "SoftBound.h"

int main(){
  TableSetup();
  int *a = malloc(sizeof(int));
  int *b = malloc(sizeof(int));

  TableAssign(a, NULL, NULL);
  TableAssign(b, NULL, NULL);

  int *x, *y;
  TableLookup(a, (void*)&x, (void*)&y);
  TableLookup(b, (void*)&x, (void*)&y);

  TableTeardown();
  return 0;
}
