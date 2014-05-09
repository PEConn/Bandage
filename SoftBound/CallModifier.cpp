#include "CallModifier.hpp"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

CallModifier::CallModifier(FunctionDuplicater *FD, LocalBounds *LB){
  this->FD = FD;
  this->LB = LB;
  this->PR = new PointerReturn();
  ModifyCalls();
}

CallModifier::~CallModifier(){
  delete PR;
}

void CallModifier::ModifyCalls(){
  std::vector<CallInst *> Calls;
  std::vector<ReturnInst *> Returns;
  for(auto F: FD->FPFunctions){
    for(auto II = inst_begin(F), EI = inst_end(F); II != EI; ++II){
      Instruction *I = &*II;
      if(auto C = dyn_cast<CallInst>(I)){
        Calls.push_back(C);
      }
      if(auto R = dyn_cast<ReturnInst>(I)){
        Returns.push_back(R);
      }
    }
  }
  for(auto C: Calls)
    ModifyCall(C);
  for(auto R: Returns)
    WrapReturn(R);
}

void CallModifier::ModifyCall(CallInst *C){
  Function *OrigFunc = C->getCalledFunction();
  if(!FD->RawToFPMap.count(OrigFunc))
    return;

  IRBuilder<> B(C);
  // Add bounds parameters
  std::vector<Value *> Params;
  for(int i=0; i<C->getNumArgOperands(); i++){
    Value *Param = C->getArgOperand(i);
    Params.push_back(Param);
    if(Param->getType()->isPointerTy()){
      Params.push_back(B.CreateLoad(LB->GetLowerBound(Param)));
      Params.push_back(B.CreateLoad(LB->GetUpperBound(Param)));
    } 
  }

  // Replace with a new call instruction
  Function *NewFunc = FD->RawToFPMap[OrigFunc];
  CallInst *NewCall = CallInst::Create(NewFunc, Params, "", C);

  if(NewFunc->getReturnType()->isVoidTy()){
    C->replaceAllUsesWith(NewCall);
    C->eraseFromParent();
    return;
  }
  Value *LowerBound = LB->GetLowerBound(C);
  Value *UpperBound = LB->GetUpperBound(C);

  C->replaceAllUsesWith(NewCall);
  C->eraseFromParent();

  // Unwrap return value if nessecary
  if(OrigFunc->getType()->isPointerTy()){
    BasicBlock::iterator iter = NewCall;
    iter++;
    IRBuilder<> B(iter);

    Value *NewReturn = B.CreateAlloca(OrigFunc->getReturnType());
    LoadInst *Dummy = B.CreateLoad(NewReturn);
    NewCall->replaceAllUsesWith(Dummy);

    Value *PointerReturn = B.CreateAlloca(NewFunc->getReturnType());
    B.CreateStore(NewCall, PointerReturn);

    PR->ExtractFromPointerReturn(NewReturn, LowerBound, UpperBound, PointerReturn, B);
    Dummy->replaceAllUsesWith(B.CreateLoad(NewReturn));
    Dummy->eraseFromParent();
  }
}
void CallModifier::WrapReturn(ReturnInst *R){
  // This will only be called inside a SoftBound modified function
  if(!R->getReturnValue())
    return;
  if(!R->getReturnValue()->getType()->isPointerTy())
    return;

  IRBuilder<> B(R);
  Value *OldRet = R->getReturnValue();
  Value *NewRet = B.CreateAlloca(PR->GetPointerReturnType(OldRet->getType()));
  PR->WrapInPointerReturn(OldRet, LB->GetLowerBound(OldRet), LB->GetUpperBound(OldRet), NewRet, B);
  R->setOperand(0, B.CreateLoad(NewRet));
}
