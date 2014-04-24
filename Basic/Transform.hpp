#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <map>
#include "InstructionCollection.hpp"
#include "FatPointers.hpp"
#include "llvm/IR/Module.h"
#include "../PointerAnalysis/Pointer.hpp"

using namespace llvm;

class Transform{
public:
  Transform(InstructionCollection *, std::map<Function *, Function *>, Module &);
  void Apply();
  void AddPointerAnalysis(std::map<Pointer, CCuredPointerType> Qs, ValueToValueMapTy &VMap);
private:
  InstructionCollection *Instructions;
  std::map<Function *, Function *> RawToFPFunctionMap;
  std::map<Value *, Value *> FPToRawVariableMap;
  std::map<Pointer, CCuredPointerType> Qs;

  DataLayout *DL;
  Function *Print;
  Module *M;

  void TransformPointerAllocas();
  void TransformPointerStores();
  void TransformPointerLoads();
  void TransformArrayAllocas();
  void TransformArrayGeps();

  void TransformFunctionCalls();
  void TransformExternalFunctionCalls();
  void TransformReturns();

  void RecreateGeps();

  void SetBoundsOnConstString(IRBuilder<> &B, StoreInst *PointerStore);
  void SetBoundsOnMalloc(IRBuilder<> &B, StoreInst *PointerStore);
  void SetBoundsOnExternalFunctionCall(IRBuilder<> &B, StoreInst *PointerStore);
  void SetBoundsOnFree(Instruction *Next, Value *FatPointer);

  void AddPrint(IRBuilder<> B, std::string str, Value *v);
};

#endif
