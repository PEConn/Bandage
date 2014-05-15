#include <stdio.h>
#include <stdlib.h>


typedef struct hash_entry {
  unsigned int key;
  void *entry;
  struct hash_entry *next;
  unsigned int padding;
} *HashEntry;

typedef struct hash {
  HashEntry *array;
  int (*mapfunc)(unsigned int);
  int size;
  unsigned int padding;
} *Hash;

void HashDelete(unsigned int key,Hash hash)
{
  HashEntry *ent;
  HashEntry tmp;
  int j;

  j = (hash->mapfunc)(key);
  ent = &(hash->array[j]);
  if((*ent)->key!=key)
    printf("");
  ent=&((*ent)->next);

  /*
     for (ent=&(hash->array[j]);
      CAP_VALID(*ent) &&
      (*ent)->key!=key; ent=&((*ent)->next))
    ;
    */
  tmp = *ent;
  *ent = (*ent)->next;
}

int main(){
}
