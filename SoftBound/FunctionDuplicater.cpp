#include <fstream>
#include "FunctionDuplicater.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/Cloning.h"

FunctionDuplicater::FunctionDuplicater(Module &M, std::string FuncFile){
  Main = NULL;
  // If we have a file of modifiable functions, use it
  std::set<std::string> IntDecls;
  if(FuncFile != ""){
    std::ifstream File(FuncFile);
    std::string Line;
    while(std::getline(File, Line)){
      IntDecls.insert(Line);
    }
    File.close();
  }

  PR = new PointerReturn();
  for(auto i = M.begin(), e = M.end(); i != e; ++i){
    Function *F = &*i;
    if(F->empty() && !IntDecls.count(F->getName()))
      continue;

    // Horrible hack to avoid modifying heap bounds lookup functions
    if(F->getName() == "TableSetup"
        || F->getName() == "TableTeardown"
        || F->getName() == "TableLookup"
        || F->getName() == "TableAssign")
      continue;

    if(F->getName() == "main")
      Main = F;

    RawFunctions.insert(F);
  }
  DuplicateFunctions(M);
  RenameMain();
}
FunctionDuplicater::~FunctionDuplicater(){
  delete PR;
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
    if(ReturnType->isPointerTy())
      ReturnType = PR->GetPointerReturnType(ReturnType);

    FunctionType *NewFuncType = FunctionType::get(ReturnType, Params, false);

    Function *NewFunc = Function::Create(NewFuncType,
        GlobalValue::LinkageTypes::ExternalLinkage,
        F->getName() + ".soft", &M);

    if(!F->empty()){
      ValueToValueMapTy VMap;
      // Map the arguments
      // The old function should have fewer or equal arguments than the new function
      auto OldArgI = F->arg_begin();
      auto OldArgE = F->arg_end();
      auto NewArgI = NewFunc->arg_begin();
      auto NewArgE = NewFunc->arg_end();
      for(int i=0; i<OldFuncType->getNumParams(); i++){
        VMap[OldArgI] = NewArgI;
        auto *ParamType = OldFuncType->getParamType(i);
        // Skip the base and bounds parameters
        if(ParamType->isPointerTy()){
          NewArgI++;
          NewArgI++;
        }
        OldArgI++;
        NewArgI++;
      }
      SmallVector<ReturnInst *, 5> Returns;
      CloneFunctionInto(NewFunc, F, VMap, true, Returns, "",
          NULL, NULL, NULL);
    }

    FPFunctions.insert(NewFunc);
    RawToFPMap[F] = NewFunc;

  }
}
void FunctionDuplicater::RenameMain(){
  if(Main){
    RawToFPMap[Main]->takeName(Main);
    Main->setName("oldmain");
  }
}
