#include <set>
#include <map>
#include <algorithm>

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

#include "Helpers.hpp"
#include "FunctionDuplicater.hpp"
#include "InstructionCollection.hpp"
#include "Transform.hpp"
#include "FatPointers.hpp"

using namespace llvm;

namespace {
struct Bandage : public ModulePass{
  static char ID;
  Bandage() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) {
    auto *FD = new FunctionDuplicater(M);
    auto *IC = new InstructionCollection(FD->GetFPFunctions(), 
        FD->GetRawFunctions());
    auto *T  = new Transform(IC, FD->RawToFPMap, M);
    T->Apply();

    return true;
  }
};
}


char Bandage::ID = 0;
static RegisterPass<Bandage> X("bandage", "Bandage Pass", false, false);
