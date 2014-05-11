#ifndef HEAP_BOUNDS
#define HEAP_BOUNDS

#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

class HeapBounds{
public:
  HeapBounds(Module &M);
  bool IsValid();
  void InsertTableLookup(IRBuilder<> &B, Value *Key, Value *Base, Value *Bound);
  void InsertTableAssign(IRBuilder<> &B, Value *Key, Value *Base, Value *Bound);
private:
  Function *Teardown;
  Function *Setup;
  Function *Lookup;
  Function *Assign;
};

#endif
