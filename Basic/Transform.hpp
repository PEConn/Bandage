#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <map>
#include "InstructionCollection.hpp"
#include "FatPointers.hpp"
#include "llvm/IR/Module.h"

using namespace llvm;

class Transform{
public:
  Transform(InstructionCollection *, std::map<Function *, Function *>, Module &);
  void Apply();
private:
  InstructionCollection *Instructions;
  std::map<Function *, Function *> RawToFPFunctionMap;
  FatPointers *FPS;

  DataLayout *DL;
  Function *Print;

  void TransformPointerAllocas();
  void TransformPointerStores();
  void TransformPointerLoads();
  void TransformArrayAllocas();
  void TransformArrayGeps();

  void TransformFunctionCalls();

  void SetBoundsForConstString(IRBuilder<> &B, StoreInst *PointerStore);
  void SetBoundsForMalloc(IRBuilder<> &B, StoreInst *PointerStore);

  void CreateBoundsCheck(IRBuilder<> &B, Value *Val, Value *Base, Value *Bound);

  void AddPrint(IRBuilder<> B, std::string str, Value *v);
};

#endif