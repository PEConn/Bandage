#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
  struct Bandage : public ModulePass{
    static char ID;
    Bandage() : ModulePass(ID) {}

    virtual bool runOnModule(Module &M) {
	  for(auto I = M.begin(), E = M.end(); I != E; ++I){
	    Function *F = I;
	  	errs() << F->getName() << "\n";
	  }

      return false;
    }
  };
}

char Bandage::ID = 0;
static RegisterPass<Bandage> X("bandage", "Bandage Pass", false, false);
