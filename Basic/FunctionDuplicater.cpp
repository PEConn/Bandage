#include "FunctionDuplicater.hpp"

#include <string>
#include <fstream>
#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

FunctionDuplicater::FunctionDuplicater(Module &M, TypeDuplicater *TD, std::string FuncFile){
  // If we have a file of modifiable functions, use it
  std::set<std::string> IntDecls;
  if(FuncFile != ""){
    std::ifstream File(FuncFile);
    std::string Line;
    while(std::getline(File, Line)){
      IntDecls.insert(Line);
    }
    File.close();
    for(auto s: IntDecls){
      errs() << s << "\n";
    }
  }

  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    Function *F = &*IF;
    // Should this be changed to 'isDeclaration'?
    if(F->empty() && !IntDecls.count(F->getName()))
      continue;

    if(F->getName() == "main")
      Main = F;

    RawFunctions.insert(F);
  }

  DuplicateFunctions(M, TD);
  RenameMain();
}

void FunctionDuplicater::DuplicateFunctions(Module &M, TypeDuplicater *TD){
  for(auto F: RawFunctions){
    // Construct a new parameter list with Fat Pointers instead of Pointers
    FunctionType *OldFuncType = F->getFunctionType();

    std::vector<Type *> Params;
    for(int i=0; i<OldFuncType->getNumParams(); i++){
      Type *Param = TD->remapType(OldFuncType->getParamType(i));
      if(Param->isPointerTy() && F != Main)
        Params.push_back(FatPointers::GetFatPointerType(Param));
      else
        Params.push_back(Param);
    }

    Type *ReturnType = TD->remapType(OldFuncType->getReturnType());
    if(ReturnType->isPointerTy() && F != Main)
      ReturnType = FatPointers::GetFatPointerType(ReturnType);
    else
      ReturnType = ReturnType;

    // TODO: Copy over if the function type is VarArg
    FunctionType *NewFuncType = FunctionType::get(ReturnType, Params, false);

    // Again, I'm not sure what linkage to use here
    Function *NewFunc = Function::Create(NewFuncType, 
        GlobalValue::LinkageTypes::ExternalLinkage, 
        F->getName() + ".FP", &M);

    if(!F->empty()){
      for(auto OldArgI = F->arg_begin(), OldArgE = F->arg_end(),
          NewArgI = NewFunc->arg_begin(), NewArgE = NewFunc->arg_end();
          OldArgI != OldArgE; OldArgI++, NewArgI++){
        VMap[OldArgI] = NewArgI;
      }
      SmallVector<ReturnInst *, 5> Returns;
      CloneFunctionInto(NewFunc, F, VMap, true, Returns, "", 
          NULL, TD, NULL);
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

std::set<Function *> FunctionDuplicater::GetRawFunctions(){
  return RawFunctions;
}
std::set<Function *> FunctionDuplicater::GetFPFunctions(){
  return FPFunctions;
}
Function *FunctionDuplicater::GetFPVersion(Function *F){
  return RawToFPMap[F];
}
