#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
struct PointerAnalysis : public ModulePass{
  static char ID;
  PointerAnalysis() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) {
    errs() << "Hello World" << "\n";
    return false;
  }
  virtual void getAnalysisUsage(AnalysisUsage &AU) const { 
  }
};
}


char PointerAnalysis::ID = 0;
static RegisterPass<PointerAnalysis> X("pointer-analysis", "Pointer Analysis Pass", false, false);
