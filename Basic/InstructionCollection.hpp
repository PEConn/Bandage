#ifndef INSTRUCTIONS_HPP
#define INSTRUCTIONS_HPP

#include <set>
#include "llvm/IR/Instructions.h"
#include "FunctionDuplicater.hpp"
#include "TypeDuplicater.hpp"

using namespace llvm;

class InstructionCollection{
public:
  InstructionCollection(FunctionDuplicater *FD, TypeDuplicater *TD);

  std::set<AllocaInst *>          PointerAllocas;
  std::set<StoreInst *>           PointerStores;
  std::set<StoreInst *>           PointerStoresFromReturn;
  std::set<StoreInst *>           PointerStoresFromPointerEquals;
  std::set<LoadInst *>            PointerLoads;
  std::set<LoadInst *>            PointerLoadsForParameters;
  std::set<LoadInst *>            PointerLoadsForReturn;
  std::set<LoadInst *>            PointerLoadsForPointerEquals;

  std::set<CallInst *>            Calls;
  std::set<CallInst *>            ExternalCalls;
  std::set<ReturnInst *>          Returns;

  std::set<AllocaInst *>          ArrayAllocas;
  std::set<GetElementPtrInst *>   ArrayGeps;

  std::set<GetElementPtrInst *>   GepsToRecreate;
private:
  FunctionDuplicater *FD;
  TypeDuplicater *TD;

  void CollectInstructions(std::set<Function *> Functions);

  void CheckForPointerAlloca(Instruction *I);
  void CheckForPointerStore(Instruction *I);
  void CheckForPointerLoad(Instruction *I);
  void CheckForPointerParameter(Instruction *I);
  void CheckForArrayAlloca(Instruction *I); 

  void CheckForStructGep(Instruction *I);

  void CheckForFunctionCall(Instruction *I);
  void CheckForReturn(Instruction *I);

  void AddArrayGeps();
  void AddPointerEquals();
};

#endif
