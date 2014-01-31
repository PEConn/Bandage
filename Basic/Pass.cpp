#include <set>

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

#include "../Helpers/Helpers.h"

using namespace llvm;

namespace {
struct Bandage : public ModulePass{
  static char ID;
  Bandage() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) {
    DL = new DataLayout(&M);
    Print = M.getFunction("printf");

    std::set<Instruction *> ArrayAllocs = CollectArrayAllocs(M);
    std::set<Instruction *> GetElementPtrs = CollectGetElementPtrs(M);

    //PrintIrWithHighlight(M, ArrayAllocs, GetElementPtrs);
    //DisplayArrayInformation(ArrayAllocs);
    //DisplayGepInformation(GetElementPtrs);
    ModifyGeps(GetElementPtrs);
    ModifyArrayAllocs(ArrayAllocs);

    return true;
  }
private:
  DataLayout *DL = NULL;
  Function *Print = NULL;

  std::set<Instruction *> CollectArrayAllocs(Module &M);
  std::set<Instruction *> CollectGetElementPtrs(Module &M);

  void DisplayArrayInformation(std::set<Instruction *> ArrayAllocs);
  void DisplayGepInformation(std::set<Instruction *> GetElementPtrs);

  void ModifyGeps(std::set<Instruction *> GetElementPtrs);
  void ModifyArrayAllocs(std::set<Instruction *> ArrayAllocs);
};
}

std::set<Instruction *> Bandage::CollectArrayAllocs(Module &M){
  std::set<Instruction *> ArrayAllocs;

  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    for(auto II = inst_begin(IF), EI = inst_end(IF); II != EI; ++II){
      Instruction *I = &*II;
      if(AllocaInst *I = dyn_cast<AllocaInst>(&*II)){
        if(!II->getType()->isPointerTy())
          continue;
        if(!II->getType()->getPointerElementType()->isArrayTy())
          continue;
        ArrayAllocs.insert(I);
      }
    }
  }
  return ArrayAllocs;
}

std::set<Instruction *> Bandage::CollectGetElementPtrs(Module &M){
  std::set<Instruction *> GetElementPtrs;

  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    for(auto II = inst_begin(IF), EI = inst_end(IF); II != EI; ++II){
      if(GetElementPtrInst *I = dyn_cast<GetElementPtrInst>(&*II)){
        GetElementPtrs.insert(I);
      }
    }
  }
  return GetElementPtrs;
}

void Bandage::DisplayArrayInformation(std::set<Instruction *> ArrayAllocs){
  for(auto I: ArrayAllocs){
    auto ArrayAlloc = cast<AllocaInst>(I);

    errs() << "Instruction:\t" << *ArrayAlloc << "\n";
    errs() << "Array Name:\t\t" << ArrayAlloc->getName() << "\n";
    //errs() << "No of Elements:\t\t" << (cast<ConstantInt>(ArrayAlloc->getArraySize()))->getSExtValue() << "\n";

    Type *ArrayType = ArrayAlloc->getAllocatedType();
    errs() << "No of Elements:\t\t" << ArrayType->getVectorNumElements() << "\n";
    Type *ElementType = ArrayType->getVectorElementType();
    //errs() << "Element Size(p):\t" << ElementType->getPrimitiveSizeInBits() << "\n";
    //errs() << "Element Size(s):\t" << ElementType->getScalarSizeInBits() << "\n";

    errs() << "Element Size(d):\t" << DL->getTypeAllocSize(ElementType) << "\n";
    
  }
}

void Bandage::DisplayGepInformation(std::set<Instruction *> GetElementPtrs){
  for(auto I: GetElementPtrs){
    auto Gep = cast<GetElementPtrInst>(I);
    errs() << "Instruction:\t" << *Gep << "\n";
  }
}

void Bandage::ModifyArrayAllocs(std::set<Instruction *> ArrayAllocs){
  for(auto I: ArrayAllocs){
    auto ArrayAlloc = cast<AllocaInst>(I);
    errs() << "Instruction:\t" << *ArrayAlloc << "\n";

    BasicBlock::iterator iter = ArrayAlloc;
    iter++;
    IRBuilder<> B(iter);

    Value *PrintString = B.CreateGlobalStringPtr(StringRef(blue + "Arr: %p\n" + reset), 
        "PrintString");
    B.CreateCall2(Print, PrintString, ArrayAlloc, "DebugPrint"); 
  }
}

void Bandage::ModifyGeps(std::set<Instruction *> GetElementPtrs){
  for(auto I: GetElementPtrs){
    auto Gep = cast<GetElementPtrInst>(I);
    errs() << "Instruction:\t" << *Gep << "\n";

    BasicBlock::iterator iter = Gep;
    iter++;
    IRBuilder<> B(iter);

    Value *PrintString = B.CreateGlobalStringPtr(StringRef(blue + "GEP: %p\n" + reset), 
        "PrintString");
    B.CreateCall2(Print, PrintString, Gep, "DebugPrint"); 
  }
}

char Bandage::ID = 0;
static RegisterPass<Bandage> X("bandage", "Bandage Pass", false, false);
