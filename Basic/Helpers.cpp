#include "Helpers.hpp"

#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Constants.h"


LinkType GetLinkType(Value *V){
  if(isa<LoadInst>(V))
    return LOAD;
  else if (isa<GetElementPtrInst>(V))
    return GEP;
  else if (isa<CastInst>(V))
    return CAST;
  else
    return NO_LINK;
}
Value *GetNextLink(Value *Link){
  if(auto L = dyn_cast<LoadInst>(Link))
    return L->getPointerOperand();
  else if(auto G = dyn_cast<GetElementPtrInst>(Link))
    return G->getPointerOperand();
  else if(auto B = dyn_cast<CastInst>(Link))
    return B->getOperand(0);
  assert(false && "GetNextLink has been given invalid link type");
  return NULL;
}
Pointer GetOriginator(Value *Link, int level){
  while(GetLinkType(Link) != NO_LINK){
    if(GetLinkType(Link) == LOAD)
      (level)++;
    Link = GetNextLink(Link);
  }
  return Pointer(Link, level);
}
PointerDestination GetDestination(Value *Link){
  while(true){
    if(isa<ReturnInst>(Link))
      return RETURN;
    else if(isa<CallInst>(Link))
      return CALL;
    else if(isa<StoreInst>(Link))
      return STORE;
    Link = Link->use_back();
  }
  return OTHER;
}
unsigned int GetNumElementsInArray(AllocaInst * ArrayAlloc){
    Type *ArrayType = ArrayAlloc->getAllocatedType();
    return ArrayType->getVectorNumElements();
}
int CountPointerLevels(Type *T){
  int level =0;
  while(T->isPointerTy()){
    level++;
    T = T->getPointerElementType();
  }
  return level;
}

void StoreInFatPointerValue(Value *FatPointer, Value *Val, IRBuilder<> &B){
  std::vector<Value *> FieldIdx = GetIndices(0, FatPointer->getContext());
  Value *FatPointerField = B.CreateGEP(FatPointer, FieldIdx, "Value"); 
  B.CreateStore(Val, FatPointerField);
}
void StoreInFatPointerBase(Value *FatPointer, Value *Val, IRBuilder<> &B){
  std::vector<Value *> FieldIdx = GetIndices(1, FatPointer->getContext());
  Value *FatPointerField = B.CreateGEP(FatPointer, FieldIdx, "Base"); 
  B.CreateStore(Val, FatPointerField);
}
void StoreInFatPointerBound(Value *FatPointer, Value *Val, IRBuilder<> &B){
  std::vector<Value *> FieldIdx = GetIndices(2, FatPointer->getContext());
  Value *FatPointerField = B.CreateGEP(FatPointer, FieldIdx, "Bound"); 
  B.CreateStore(Val, FatPointerField);
}

unsigned int GetArrayElementSizeInBits(AllocaInst *ArrayAlloc, DataLayout *DL){
    Type *ArrayType = ArrayAlloc->getAllocatedType();
    Type *ElementType = ArrayType->getVectorElementType();
    return DL->getTypeAllocSizeInBits(ElementType);
}

void PrintIrWithHighlight(Module &M, std::set<Instruction *> H1){
  PrintIrWithHighlight(M, H1, std::set<Instruction *>(), 

      std::set<Instruction *>(), std::set<Instruction *>());
}

void PrintIrWithHighlight(Module &M, std::set<Instruction *> H1,
    std::set<Instruction *> H2){

  PrintIrWithHighlight(M, H1, H2, 
      std::set<Instruction *>(), std::set<Instruction *>());
}

void PrintIrWithHighlight(Module &M, std::set<Instruction *> H1,
    std::set<Instruction *> H2, std::set<Instruction *> H3){

  PrintIrWithHighlight(M, H1, H2, H3, std::set<Instruction *>());
}

void PrintIrWithHighlight(Module &M, std::set<Instruction *> H1,
    std::set<Instruction *> H2, std::set<Instruction *> H3,
    std::set<Instruction *> H4){

  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    for(auto II = inst_begin(IF), EI = inst_end(IF); II != EI; ++II){
      Instruction *I = &*II;

      if(H1.count(I))       errs() << red    << *I << reset << "\n";
      else if(H2.count(I))  errs() << green  << *I << reset << "\n";
      else if(H3.count(I))  errs() << blue   << *I << reset << "\n";
      else if(H4.count(I))  errs() << yellow << *I << reset << "\n";
      else                  errs()           << *I << "\n";
    }
  }
}

void removeTerminator(BasicBlock *BB) {
  TerminatorInst *BBTerm = BB->getTerminator();
  // Remove the BB as a predecessor from all of  successors
  for (unsigned i = 0, e = BBTerm->getNumSuccessors(); i != e; ++i) {
    BBTerm->getSuccessor(i)->removePredecessor(BB);
  }
  BBTerm->replaceAllUsesWith(UndefValue::get(BBTerm->getType()));
  // Remove the terminator instruction itself.
  BBTerm->eraseFromParent();
}

std::vector<Value *> GetIndices(int val, LLVMContext& C){
  std::vector<Value *> Idxs;
  Idxs.push_back(ConstantInt::get(IntegerType::getInt32Ty(C), 0));
  Idxs.push_back(ConstantInt::get(IntegerType::getInt32Ty(C), val));
  return Idxs;
}

Value* Str(IRBuilder<> B, std::string str){
  return B.CreateGlobalStringPtr(StringRef(blue + str + "\n" + reset));
}
