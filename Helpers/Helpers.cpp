#include "Helpers.h"

#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/IR/DataLayout.h"

unsigned int GetNumElementsInArray(AllocaInst * ArrayAlloc){
    Type *ArrayType = ArrayAlloc->getAllocatedType();
    return ArrayType->getVectorNumElements();
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
