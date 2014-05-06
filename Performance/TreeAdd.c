#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TIME (float)clock() / CLOCKS_PER_SEC

typedef struct tree{
  int val;
  struct tree *left, *right;
} tree_t;

tree_t *TreeAlloc(int level, int lo, int proc){
  if(level == 0)
    return NULL;

  //tree_t *new = malloc(sizeof(tree_t));
  tree_t *new = malloc(52);
  tree_t *left = TreeAlloc(level-1, lo+proc/2, proc/2);
  tree_t *right = TreeAlloc(level-1, lo, proc/2);
  new->val = 1;
  new->left = left;
  new->right = right;

  return new;
}

int TreeAdd(tree_t *t){
  if(t == NULL)
    return 0;
  int leftval = TreeAdd(t->left);
  int rightval = TreeAdd(t->right);
  int value = t->val;
  return leftval + rightval + value;
}

int main(){
  tree_t *t;
  int tot;
  float t0 = TIME;
  t = TreeAlloc(24, 0, 4);
  float t1 = TIME;
  tot = TreeAdd(t);
  float t2 = TIME;
  //printf("%f\n", t1-t0);
  printf("%f\n", t2-t1);
  //printf("Alloc: %f\n", t1-t0);
  //printf("Add  : %f\n", t2-t1);
}
