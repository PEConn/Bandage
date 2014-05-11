// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int ComputeEditDistance(const char *w1, const int s1, const char *w2, const int s2){
  short *grid = malloc(sizeof(short) * s1 * s2);
  // The grid is s1 wide and s2 tall
  // The i'th cell across and j'th cell down is given by:
  //   grid[j*s1 + i]

  for(int i0=0; i0<s1; i0++) grid[i0] = i0;
  for(int j0=0; j0<s2; j0++) grid[j0 * s1] = j0;

  for(int j=1; j<s2; j++){
    for(int i=1; i<s1; i++){
      int left    = grid[    j*s1 + (i-1)];
      int diag    = grid[(j-1)*s1 + (i-1)];
      int above   = grid[(j-1)*s1 + i    ];

      int min = (left < diag ? left : diag);
      min = (min < above ? min : above);

      if(w1[i] != w2[j]) min++;

      grid[j*s1 + i] = min;
      printf("%d ", min);
    }
    printf("\n");
  }

  // Out of Bounds error here!
  int ret = grid[s1 * s2];
  //int ret = grid[s1 * s2 - 1];
  free(grid);
  return ret;
}

int main(){
  char *w1 = "balloon";   int s1 = 7;
  char *w2 = "baboon";    int s2 = 6;

  int dist = ComputeEditDistance(w1, s1, w2, s2);
  // CHECK: OutOfBounds
  printf("The edit distance between %s and %s is %d\n", w1, w2, dist);
}

