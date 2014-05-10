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
    Value *Pointer = S->getPointerOperand();
    Value *Lower = LB->GetLowerBound(Pointer);
    Value *Upper = LB->GetUpperBound(Pointer);

    if(SetOnMalloc(S, Lower, Upper)) continue;
    if(SetOnConstString(S, Lower, Upper)) continue;
    if(SetOnPointerEquals(S, Lower, Upper, LB)) continue;
  }
}

bool BoundsSetter::SetOnMalloc(StoreInst *S, Value *Lower, Value *Upper){
  // Follow through the bitcast if there is one
  Value *V = S->getValueOperand();
  if(auto B = dyn_cast<CastInst>(V))
    V = B->getOperand(0);

  CallInst *C = dyn_cast<CallInst>(V);
  if(!C || C->getCalledFunction()->getName() != "malloc") return false;

  // Set bounds here
  BasicBlock::iterator iter = C;
  iter++;
  IRBuilder<> B(iter);

  Type *IntTy = IntegerType::getInt64Ty(C->getContext());
  Type *PtrTy = Lower->getType()->getPointerElementType();
  B.CreateStore(B.CreatePointerCast(C, PtrTy), Lower);
  B.CreateStore(B.CreateIntToPtr(
        B.CreateAdd(C->getArgOperand(0),
          B.CreatePtrToInt(C, IntTy)),
        PtrTy), Upper);

  return true;
}

bool BoundsSetter::SetOnConstString(StoreInst *S, Value *Lower, Value *Upper){
  Value *V = S->getValueOperand();
  auto G = dyn_cast<GEPOperator>(S->getValueOperand());
  if(!G) return false;
  auto C = dyn_cast<Constant>(G->getPointerOperand());
  if(!C) return false;

  BasicBlock::iterator iter = S;
  iter++;
  IRBuilder<> B(iter);

  Type *IntTy = IntegerType::getInt64Ty(C->getContext());
  Type *PtrTy = Lower->getType()->getPointerElementType();
  int LengthOfString = G->getPointerOperand()->getType()->getPointerElementType()->getVectorNumElements();
  Value *Length = ConstantInt::get(IntTy, LengthOfString);

  B.CreateStore(G, Lower);
  B.CreateStore(B.CreateIntToPtr(B.CreateAdd(
          B.CreatePtrToInt(G, IntTy), Length), PtrTy), Upper);
  return true;
}

bool BoundsSetter::SetOnPointerEquals(StoreInst *S, Value *Lower, Value *Upper, LocalBounds *LB){
  Value *V = S->getValueOperand();
  if(!V->getType()->isPointerTy()) return false;
  if(!LB->HasBoundsFor(V)) return false;
  BasicBlock::iterator iter = S;
  iter++;
  IRBuilder<> B(iter);

  B.CreateStore(B.CreateLoad(LB->GetLowerBound(V)), Lower);
  B.CreateStore(B.CreateLoad(LB->GetUpperBound(V)), Upper);
  return true;
}
