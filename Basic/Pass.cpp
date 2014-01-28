#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

using namespace llvm;

namespace {
  struct Bandage : public ModulePass{
    static char ID;
    Bandage() : ModulePass(ID) {}

    virtual bool runOnModule(Module &M) {
	  for(auto IFunc = M.begin(), EFunc = M.end(); IFunc != EFunc; ++IFunc){
	    Function *F = IFunc;

		for(auto IInst = inst_begin(F), EInst = inst_end(F); IInst != EInst; ++IInst){
		  //Instruction *I = &*IInst;
          if(GetElementPtrInst *I = dyn_cast<GetElementPtrInst>(&*IInst)){
		    errs() << *I << "\n";
		  }

		  if(AllocaInst *I = dyn_cast<AllocaInst>(&*IInst)){
		  	errs() << *I << "\n";
		  }
		}
	  }

      return false;
    }
  };
}

char Bandage::ID = 0;
static RegisterPass<Bandage> X("bandage", "Bandage Pass", false, false);
