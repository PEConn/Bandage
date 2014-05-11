#include "BoundsSetter.hpp"
#include "llvm/Support/InstIterator.h"
#include "../Basic/Helpers.hpp"
#include "llvm/Support/raw_ostream.h"

BoundsSetter::BoundsSetter(std::set<Function *> Functions){
  CollectStores(Functions);
}
void BoundsSetter::CollectStores(std::set<Function *> Functions){
  for(auto F: Functions){
    for(auto II = inst_begin(F), EI = inst_end(F); II != EI; ++II){
      Instruction *I = &*II;
      if(auto S = dyn_cast<StoreInst>(I)){
        // Add other checks here
        if(!S->getPointerOperand()->getType()
            ->getPointerElementType()->isPointerTy())
          continue;
        Stores.insert(S);
      }
    }
  }
}
void BoundsSetter::SetBounds(LocalBounds *LB, HeapBounds *HB){
  for(auto S: Stores){
    Type *PtrTy = Type::getInt8PtrTy(S->getContext());
    Type *PtrPtrTy = PtrTy->getPointerTo();
    Value *Pointer = S->getPointerOperand();
    Value *ToStoreInLower = NULL;
    Value *ToStoreInUpper = NULL;
    IRBuilder<> B(S);

    if(SetOnMalloc(B, S, ToStoreInLower, ToStoreInUpper))
      ;
    else if(SetOnConstString(B, S, ToStoreInLower, ToStoreInUpper))
      ;
    else if(SetOnPointerEquals(B, S, ToStoreInLower, ToStoreInUpper, LB))
      ;

    if(ToStoreInLower == NULL){
      errs() << "Could not store bounds on: " << *S << "\n";
      continue;
    }

    if(LB->HasBoundsFor(Pointer, false)){
      Value *Lower = LB->GetLowerBound(Pointer, false);
      B.CreateStore(ToStoreInLower, Lower);
      Value *Upper = LB->GetUpperBound(Pointer, false);
      B.CreateStore(ToStoreInUpper, Upper);
    } else {
      B.SetInsertPoint(S);
      HB->InsertTableAssign(B, 
          B.CreatePointerCast(S->getValueOperand(), PtrTy), 
          B.CreatePointerCast(ToStoreInLower, PtrTy), 
          B.CreatePointerCast(ToStoreInUpper, PtrTy));
    }
  }
}

bool BoundsSetter::SetOnMalloc(IRBuilder<> &B, StoreInst *S, Value *&StoreToLower, Value *&StoreToUpper){
  // Follow through the bitcast if there is one
  Value *V = S->getValueOperand();
  if(auto B = dyn_cast<CastInst>(V))
    V = B->getOperand(0);

  CallInst *C = dyn_cast<CallInst>(V);
  if(!C || C->getCalledFunction()->getName() != "malloc") return false;

  // Set bounds here
  BasicBlock::iterator iter = C;
  iter++;
  B.SetInsertPoint(iter);

  Type *IntTy = IntegerType::getInt64Ty(C->getContext());
  Type *PtrTy = S->getPointerOperand()->getType()->getPointerElementType();
  StoreToLower = (B.CreatePointerCast(C, PtrTy));
  StoreToUpper = (B.CreateIntToPtr(
        B.CreateAdd(C->getArgOperand(0),
          B.CreatePtrToInt(C, IntTy)),
        PtrTy));

  return true;
}

bool BoundsSetter::SetOnConstString(IRBuilder<> &B, StoreInst *S, Value *&StoreToLower, Value *&StoreToUpper){
  Value *V = S->getValueOperand();
  auto G = dyn_cast<GEPOperator>(S->getValueOperand());
  if(!G) return false;
  auto C = dyn_cast<Constant>(G->getPointerOperand());
  if(!C) return false;

  BasicBlock::iterator iter = S;
  iter++;
  B.SetInsertPoint(iter);

  Type *IntTy = IntegerType::getInt64Ty(C->getContext());
  Type *PtrTy = S->getPointerOperand()->getType()->getPointerElementType();
  int LengthOfString = G->getPointerOperand()->getType()->getPointerElementType()->getVectorNumElements();
  Value *Length = ConstantInt::get(IntTy, LengthOfString);

  StoreToLower = (G);
  StoreToUpper = (B.CreateIntToPtr(B.CreateAdd(
          B.CreatePtrToInt(G, IntTy), Length), PtrTy));
  return true;
}

bool BoundsSetter::SetOnPointerEquals(IRBuilder<> &B, StoreInst *S, Value *&StoreToLower, Value *&StoreToUpper, LocalBounds *LB){
  Value *V = S->getValueOperand();
  if(!V->getType()->isPointerTy()) return false;
  if(!LB->HasBoundsFor(V)) return false;
  BasicBlock::iterator iter = S;
  iter++;
  B.SetInsertPoint(iter);

  StoreToLower = B.CreateLoad(LB->GetLowerBound(V));
  StoreToUpper = B.CreateLoad(LB->GetUpperBound(V));
  return true;
}
