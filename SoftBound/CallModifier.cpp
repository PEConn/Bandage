#include "CallModifier.hpp"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

CallModifier::CallModifier(FunctionDuplicater *FD, LocalBounds *LB, HeapBounds *HB){
  this->FD = FD;
  this->LB = LB;
  this->HB = HB;
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

    errs() << __LINE__ << "\n";
  IRBuilder<> B(C);
  // Add bounds parameters
  std::vector<Value *> Params;
  for(int i=0; i<C->getNumArgOperands(); i++){
    Value *Param = C->getArgOperand(i);
    Params.push_back(Param);
    if(Param->getType()->isPointerTy()){
      if(LB->HasBoundsFor(Param)){
        Params.push_back(B.CreateLoad(LB->GetLowerBound(Param)));
        Params.push_back(B.CreateLoad(LB->GetUpperBound(Param)));
      } else {
        Type *PtrTy = Type::getInt8PtrTy(Param->getContext());
        Type *PtrPtrTy = PtrTy->getPointerTo();

        Value *LowerBound = B.CreateAlloca(Param->getType());
        Value *UpperBound = B.CreateAlloca(Param->getType());

        HB->InsertTableLookup(B, 
            B.CreatePointerCast(Param, PtrTy), 
            B.CreatePointerCast(LowerBound, PtrPtrTy), 
            B.CreatePointerCast(UpperBound, PtrPtrTy));

        Params.push_back(B.CreateLoad(LowerBound));
        Params.push_back(B.CreateLoad(UpperBound));
      }
    } 
  }
    errs() << __LINE__ << "\n";

  // Replace with a new call instruction
  Function *NewFunc = FD->RawToFPMap[OrigFunc];
  CallInst *NewCall = CallInst::Create(NewFunc, Params, "", C);
    errs() << __LINE__ << "\n";

  if(NewFunc->getReturnType()->isVoidTy()){
    C->replaceAllUsesWith(NewCall);
    C->eraseFromParent();
    return;
  }
    errs() << __LINE__ << "\n";

  Value *LowerBound, *UpperBound;
  if(OrigFunc->getReturnType()->isPointerTy()){
    LowerBound = LB->GetLowerBound(C);
    UpperBound = LB->GetUpperBound(C);
  }
    errs() << __LINE__ << "\n";

  C->replaceAllUsesWith(NewCall);
  C->eraseFromParent();
    errs() << __LINE__ << "\n";

  // Unwrap return value if nessecary
  if(OrigFunc->getReturnType()->isPointerTy()){
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
    errs() << __LINE__ << "\n";
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
