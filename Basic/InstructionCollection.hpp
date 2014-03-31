#ifndef INSTRUCTIONS_HPP
#define INSTRUCTIONS_HPP

#include <set>
#include "llvm/IR/Instructions.h"

using namespace llvm;

class InstructionCollection{
public:
  InstructionCollection(std::set<Function *> Functions, std::set<Function *> RawFunctions);

  std::set<AllocaInst *>          PointerAllocas;
  std::set<StoreInst *>           PointerStores;
  std::set<StoreInst *>           PointerStoresFromReturn;
  std::set<StoreInst *>           PointerStoresFromPointerEquals;
  std::set<LoadInst *>            PointerLoads;
  std::set<LoadInst *>            PointerLoadsForParameters;
  std::set<LoadInst *>            PointerLoadsForReturn;
  std::set<LoadInst *>            PointerLoadsForPointerEquals;

  std::set<CallInst *>            Calls;
  std::set<ReturnInst *>          Returns;

  std::set<AllocaInst *>          ArrayAllocas;
  std::set<GetElementPtrInst *>   ArrayGeps;

private:
  std::set<Function *> RawFunctions;

  void CollectInstructions(std::set<Function *> Functions);

  void CheckForPointerAlloca(Instruction *I);
  void CheckForPointerStore(Instruction *I);
  void CheckForPointerLoad(Instruction *I);
  void CheckForPointerParameter(Instruction *I);
  void CheckForArrayAlloca(Instruction *I); 

  void CheckForFunctionCall(Instruction *I);
  void CheckForReturn(Instruction *I);

  void AddArrayGeps();
  void AddPointerEquals();
};

#endif
