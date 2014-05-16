#include "HeapBounds.hpp"
#include "llvm/Support/raw_ostream.h"

HeapBounds::HeapBounds(Module &M){
  Teardown = M.getFunction("TableTeardown");
  Setup = M.getFunction("TableSetup");
  Lookup = M.getFunction("TableLookup");
  Assign = M.getFunction("TableAssign");
  if(!IsValid())
    errs() << "Could not find heap bounds headers.\n";
}
bool HeapBounds::IsValid(){
  return (Teardown != NULL) && (Setup != NULL) 
    && (Lookup != NULL) && (Assign != NULL);
}
void HeapBounds::InsertTableLookup(IRBuilder<> &B, Value *Key, Value *Base, Value *Bound){
  // Assumes that key, base and value are all i8*
  if(!IsValid())
    return;

  B.CreateCall3(Lookup, Key, Base, Bound);
}
void HeapBounds::InsertTableAssign(IRBuilder<> &B, Value *Key, Value *Base, Value *Bound){
  // Assumes that key, base and value are all i8*
  if(!IsValid())
    return;

  B.CreateCall3(Assign, Key, Base, Bound);
}
