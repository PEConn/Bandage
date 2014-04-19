#include <queue>
#include <string>
#include "Pointer.hpp"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

std::string Pretty(enum CCuredPointerType PT){
  switch(PT){
    case SAFE: return "Safe";
    case SEQ: return "Seqential";
    case DYN: return "Dynamic";
  }
}

Pointer::Pointer(AllocaInst *Declaration){
  this->Declaration = Declaration;
  CollectUses();
  DeterminePointerType();
  Print();
}

void Pointer::CollectUses(){
  Uses.insert(Declaration);

  std::queue<Value *> Q;
  Q.push(Declaration);

  while(!Q.empty()){
    Value *Use = Q.front();
    for(auto UI = Use->use_begin(), UE = Use->use_end(); UI != UE; ++UI){
      Q.push(*UI);
      Uses.insert(*UI);
    }
    Q.pop();
  }
}

void Pointer::DeterminePointerType(){
  CheckStores();
}

void Pointer::CheckStores(){
  std::set<StoreInst *> Stores;
  for(auto Use: Uses)
    if(auto S = dyn_cast<StoreInst>(Use))
      if(S->getPointerOperand() == Declaration)
        Stores.insert(S);

  if(Stores.empty())
    PointerType = SAFE;

  for(auto S: Stores){
    Value *Source = FollowStore(S);
    if(auto C = dyn_cast<CallInst>(Source)){
      errs() << "From function " << C->getCalledFunction()->getName() << "\n";
    } else if(auto A = dyn_cast<AllocaInst>(Source)){
      errs() << "From value " << A->getName() << "\n";
    } else {
      errs() << *Source << "\n";
    }
  }
}

Value *Pointer::FollowStore(StoreInst *S){
  Value *Inst = S;
  Value *PrevInst;
  errs() << *Inst << "\n";
  do{
    PrevInst = Inst;
    Inst = NULL;
    if(auto Store = dyn_cast<StoreInst>(PrevInst))
      Inst = Store->getValueOperand();
    if(auto Gep = dyn_cast<GetElementPtrInst>(PrevInst))
      Inst = Gep->getPointerOperand();
    if(auto Load = dyn_cast<LoadInst>(PrevInst))
      Inst = Load->getPointerOperand();
    if(auto BitCast = dyn_cast<BitCastInst>(PrevInst))
      Inst = BitCast->getOperand(0);
  errs() << *Inst << "\n";
  }while(Inst != NULL);
  return PrevInst;
}

void Pointer::Print(){
  errs() << Pretty(PointerType) << ": " << Declaration->getName() << "\n";
  for(auto Use: Uses)
    errs() << *Use << "\n";
  errs() << "----------" << "\n";
}
