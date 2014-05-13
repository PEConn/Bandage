#ifndef POINTER_USE_TRANSFORM
#define POINTER_USE_TRANSFORM

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

#include "PointerUseCollection.hpp"
#include "../PointerAnalysis/Pointer.hpp"

class PointerUseTransform{
public:
  PointerUseTransform(PointerUseCollection *PUC, Module &M, std::map<Function *, Function *> RawToFPMap, std::map<AllocaInst *, AllocaInst *> RawToFPAllocaMap);

  void Apply();
  void ApplyTo(PointerParameter *PP);
  void ApplyTo(PU *P);
  void AddPointerAnalysis(std::map<Pointer, CCuredPointerType> Qs, ValueToValueMapTy &VMap);

  int SafeLoads;
  int NoneSafeLoads;
private:
  void RecreateValueChain(std::vector<Value *> Chain);
  PointerUseCollection *PUC;
  Module *M;
  Function *Print;
  std::map<Function *, Function *> RawToFPMap;
  std::map<Pointer, CCuredPointerType> Qualifiers;
  //std::map<AllocaInst *, AllocaInst *> RawToFPAllocaMap;
  std::map<Value *, Value *> FPToRawVariableMap;
  std::map<Value *, Value *> Replacements;
};

#endif
