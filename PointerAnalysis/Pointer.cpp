#include <queue>
#include <string>
#include "Pointer.hpp"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

Pointer::Pointer(Value *id, int level){
  this->id = id;
  this->level = level;
}

std::string Pointer::ToString() const{
  return "(" + (std::string)id->getName() + ", " + std::to_string(level) + ")";
}

std::string Pretty(enum CCuredPointerType PT){
  switch(PT){
    case UNSET: return "Unset";
    case SAFE: return "Safe";
    case SEQ: return "Seqential";
    case DYNQ: return "Dynamic";
  }
}

Value *FollowStore(StoreInst *S){
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
