#include "FunctionDuplicater.hpp"

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

FunctionDuplicater::FunctionDuplicater(FatPointers *FPS, Module &M){
  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    Function *F = &*IF;
    // Should this be changed to 'isDeclaration'?
    if(F->empty())
      continue;

    if(F->getName() == "main")
      Main = F;
    else
      RawFunctions.insert(F);
  }

  DuplicateFunctions(FPS, M);
}

void FunctionDuplicater::DuplicateFunctions(FatPointers *FPS, Module &M){
  for(auto F: RawFunctions){
    // Construct a new parameter list with Fat Pointers instead of Pointers
    std::vector<Type*> Params;
    FunctionType *OldFuncType = F->getFunctionType();

    for(int i=0; i<OldFuncType->getNumParams(); i++){
      if(OldFuncType->getParamType(i)->isPointerTy())
        Params.push_back(FPS->GetFatPointerType(OldFuncType->getParamType(i)));
      else
        Params.push_back(OldFuncType->getParamType(i));
    }

    // TODO: Copy over if the function type is VarArg
    FunctionType *NewFuncType = FunctionType::get(OldFuncType->getReturnType(),
        Params, false);

    // Again, I'm not sure what linkage to use here
    Function *NewFunc = Function::Create(NewFuncType, 
        GlobalValue::LinkageTypes::ExternalLinkage, 
        F->getName() + ".FP", &M);

    // Iterate through all of the arguments of the original function
    ValueMap<Value *, Value*> ArgMap;
    for(auto OldArgI = F->arg_begin(), OldArgE = F->arg_end(),
       NewArgI = NewFunc->arg_begin(), NewArgE = NewFunc->arg_end();
       OldArgI != OldArgE; OldArgI++, NewArgI++){
      ArgMap[OldArgI] = NewArgI;
    }

    for(auto BBI = F->begin(), BBE = F->end(); BBI != BBE; ++BBI){
      BasicBlock *NewBB = BasicBlock::Create(F->getContext(), "", NewFunc);
      IRBuilder<> builder(NewBB);
      for(auto II = BBI->begin(), IE = BBI->end(); II != IE; ++II){

        Instruction *NewInst = II->clone();
        for(int i=0; i<NewInst->getNumOperands(); i++){
          if(ArgMap.count(NewInst->getOperand(i)))
            NewInst->setOperand(i, ArgMap[NewInst->getOperand(i)]);
        }

        ArgMap[II] = NewInst;

        builder.Insert(NewInst);
      }
    } 

    FPFunctions.insert(NewFunc);
    RawToFPMap[F] = NewFunc;
  }
}

std::set<Function *> FunctionDuplicater::GetRawFunctions(){
  return RawFunctions;
}
std::set<Function *> FunctionDuplicater::GetFPFunctions(){
  std::set<Function *> FPAndMain = FPFunctions;
  FPAndMain.insert(Main);
  return FPAndMain;
}
Function *FunctionDuplicater::GetFPVersion(Function *F){
  return RawToFPMap[F];
}
