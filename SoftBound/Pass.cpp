#include <set>
#include <map>
#include <algorithm>

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

#include "LocalBounds.hpp"
#include "BoundsChecks.hpp"

using namespace llvm;

namespace {
struct SoftBound : public ModulePass{
  static char ID;
  SoftBound() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) {
    auto LB = new LocalBounds(M);
    auto BC = new BoundsChecks(LB, M);
    return true;
  }
  virtual void getAnalysisUsage(AnalysisUsage &AU) const { 
    //AU.addRequired<PointerAnalysis>();
  }
};
}

char SoftBound::ID = 0;
static RegisterPass<SoftBound> Y("softbound", "SoftBound Pass", false, false);
