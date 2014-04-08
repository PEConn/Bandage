
#include <stdlib.h>
#include <stdio.h>

typedef struct Node {
  int Info;
  struct Node *Next;
} Node;

void addToList(Node *Predecessor, int Info){
  Node *Next = Predecessor->Next;

  Node *New = malloc(sizeof(Node));
  New->Info = Info;
  New->Next = Next;

  Predecessor->Next = New;
}

void printList(Node *Start){
  while(Start != NULL){
    printf("%d ", Start->Info);
    Start = Start->Next;
  }
  printf("\n");
}

void freeList(Node *Start){
  while(Start != NULL){
    Node *Next = Start->Next;
    free(Start);
    Start = Next;
  }
}

int main(){
  Node *Fib = malloc(sizeof(Node));
  Fib->Info = 1;

  Node *Curr = Fib;
  addToList(Curr, 1); Curr = Curr->Next;
  addToList(Curr, 2); Curr = Curr->Next;
  addToList(Curr, 3); Curr = Curr->Next;
  addToList(Curr, 5); Curr = Curr->Next;
  addToList(Curr, 8); Curr = Curr->Next;

  printList(Fib);
  freeList(Fib);

  return 0;
}
