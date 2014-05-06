#include "FunctionDuplicater.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/Cloning.h"

FunctionDuplicater::FunctionDuplicater(Module &M){
  for(auto i = M.begin(), e = M.end(); i != e; ++i){
    Function *F = &*i;
    if(F->empty())
      continue;

    if(F->getName() == "main")
      Main = F;

    RawFunctions.insert(F);
  }
  DuplicateFunctions(M);
  RenameMain();
}

void FunctionDuplicater::DuplicateFunctions(Module &M){
  for(auto F: RawFunctions){
    // Construct a new parameter list with each pointer replicated three times
    FunctionType *OldFuncType = F->getFunctionType();
    std::vector<Type *> Params;
    for(int i=0; i<OldFuncType->getNumParams(); i++){
      auto *ParamType = OldFuncType->getParamType(i);
      Params.push_back(ParamType);
      if(ParamType->isPointerTy()){
        Params.push_back(ParamType);
        Params.push_back(ParamType);
      }
    }

    // Will have to create fat pointers for the return type!
    Type *ReturnType = OldFuncType->getReturnType();
    FunctionType *NewFuncType = FunctionType::get(ReturnType, Params, false);

    Function *NewFunc = Function::Create(NewFuncType,
        GlobalValue::LinkageTypes::ExternalLinkage,
        F->getName() + ".soft", &M);

    ValueToValueMapTy VMap;
    for(auto OldArgI = F->arg_begin(), OldArgE = F->arg_end(),
        NewArgI = NewFunc->arg_begin(), NewArgE = NewFunc->arg_end();
        OldArgI != OldArgE; OldArgI++, NewArgI++){
      VMap[OldArgI] = NewArgI;
    }
    SmallVector<ReturnInst *, 5> Returns;
    CloneFunctionInto(NewFunc, F, VMap, true, Returns, "",
        NULL, NULL, NULL);


    FPFunctions.insert(NewFunc);
    RawToFPMap[F] = NewFunc;

  }
}
void FunctionDuplicater::RenameMain(){
  RawToFPMap[Main]->takeName(Main);
  Main->setName("oldmain");
}

  //std::set<Function *> RawFunctions;
  //std::set<Function *> FPFunctions;
  //std::map<Function *, Function *> RawToFPMap;
