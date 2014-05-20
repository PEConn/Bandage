#include "LocalBounds.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

LocalBounds::LocalBounds(Module &M, FunctionDuplicater *FD){
  CreateBounds(M, FD);
}

void LocalBounds::CreateBounds(Module &M, FunctionDuplicater *FD){
  // Duplicate Globals
  std::set<GlobalVariable *> Globals;
  for(auto i=M.global_begin(), e=M.global_end(); i!=e; ++i){
    GlobalVariable *G = &*i;
    Globals.insert(G);
  }
  for(auto G: Globals){
    std::string name = G->getName();
    if(name[0] == '_' && name[1] == '_')
      continue;
    
    if(!G->getType()->isPointerTy())
      continue;
    Type *PointerTy = G->getType()->getPointerElementType();
    if(!PointerTy->isPointerTy())
      continue;

    GlobalVariable *LB = new GlobalVariable(M, PointerTy, 
        G->isConstant(), G->getLinkage(), NULL, G->getName() + "_lower");
    GlobalVariable *UB = new GlobalVariable(M, PointerTy,
        G->isConstant(), G->getLinkage(), NULL, G->getName() + "_upper");

    ConstantAggregateZero* Init= ConstantAggregateZero::get(PointerTy);
    LB->setInitializer(Init);
    UB->setInitializer(Init);

    LowerBounds[G] = LB;
    UpperBounds[G] = UB;
  }

  for(auto F: FD->FPFunctions){
    for(auto II = inst_begin(F), EI = inst_end(F); II != EI; ++II){
      Instruction *I = &*II;
      if(auto A = dyn_cast<AllocaInst>(I)){
        if(A->getType()->getPointerElementType()->isPointerTy())
          CreateBound(A);
      }
      if(auto C = dyn_cast<CallInst>(I)){
        if(C->getType()->isPointerTy())
          CreateBound(C);
      }
    }
  }
}

void LocalBounds::CreateBound(AllocaInst *A){
  IRBuilder<> B(A);
  LowerBounds[A] = B.CreateAlloca(A->getType()->getPointerElementType());
  UpperBounds[A] = B.CreateAlloca(A->getType()->getPointerElementType());

  Value *Null = ConstantPointerNull::get(
      cast<PointerType>(A->getType()->getPointerElementType()));
  // Set the bounds to null, unless the alloca is a function parameter
  // in which case set the bounds to the next two arguments
  for(auto i = A->use_begin(), e = A->use_end(); i != e; ++i){
    Instruction *I = dyn_cast<Instruction>(*i);
    if(auto S = dyn_cast<StoreInst>(I)){
      if(auto Arg = dyn_cast<Argument>(S->getValueOperand())){
        Function *F = Arg->getParent();
        int ArgNo = Arg->getArgNo();

        auto ArgIter = F->arg_begin();
        while(ArgNo--) ArgIter++;
        ArgIter++;
        Value *L = ArgIter++;
        Value *U = ArgIter++;

        B.CreateStore(L, LowerBounds[A]);
        B.CreateStore(U, UpperBounds[A]);
        return;
      }
    }
  }
  B.CreateStore(Null, LowerBounds[A]);
  B.CreateStore(Null, UpperBounds[A]);
}

void LocalBounds::CreateBound(CallInst *C){
  IRBuilder<> B(C);
  LowerBounds[C] = B.CreateAlloca(C->getType());
  UpperBounds[C] = B.CreateAlloca(C->getType());

  Value *Null = ConstantPointerNull::get(cast<PointerType>(C->getType()));
  B.CreateStore(Null, LowerBounds[C]);
  B.CreateStore(Null, UpperBounds[C]);
}

Value *LocalBounds::GetDef(Value *V, bool IgnoreOneLoad){
  bool OneLoad = IgnoreOneLoad;
  while(true){
    if(isa<CallInst>(V) || isa<AllocaInst>(V) || isa<GlobalVariable>(V))
      break;
    else if(auto G = dyn_cast<GetElementPtrInst>(V))
      V = G->getPointerOperand();
    else if(auto C = dyn_cast<CastInst>(V))
      V = C->getOperand(0);
    else if(auto L = dyn_cast<LoadInst>(V)){
      if(OneLoad){
        V = L->getPointerOperand();
        OneLoad = false;
      } else {
        V = L;
        break;
      }
    } else {
      errs() << "Don't know how to follow :\n\t";
      errs() << *V << "\n";
      return NULL;
    }

  }
  return V;
}
bool LocalBounds::HasBoundsFor(Value *V, bool IgnoreOneLoad){
  V = GetDef(V, IgnoreOneLoad);
  return LowerBounds.count(V);
}
Value *LocalBounds::GetLowerBound(Value *V, bool IgnoreOneLoad){
  V = GetDef(V, IgnoreOneLoad);
  if(V == NULL) return NULL;
  if(!LowerBounds.count(V)){
    errs() << "Could not find: " << *V << "\n";
    errs() << "Contents:\n";
    for(auto Pair: LowerBounds)
      errs() << "\t" << *Pair.first << "\n";
    assert(false && "Could not find lower bound");
  }
  return LowerBounds[V];
}
Value *LocalBounds::GetUpperBound(Value *V, bool IgnoreOneLoad){
  V = GetDef(V, IgnoreOneLoad);
  if(V == NULL) return NULL;
  assert(UpperBounds.count(V));
  return UpperBounds[V];
}
