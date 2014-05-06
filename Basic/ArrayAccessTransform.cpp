#include "ArrayAccessTransform.hpp"

#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "Helpers.hpp"

ArrayAccessTransform::ArrayAccessTransform(std::set<Function *> Functions, Function *OnError){
  this->OnError = OnError;
  CollectGeps(Functions);
  TransformGeps();
}
void ArrayAccessTransform::CollectGeps(std::set<Function *> Functions){
  for(auto F: Functions){
    for(auto II = inst_begin(F), EI = inst_end(F); II != EI; ++II){
      Instruction *I = &*II;
      if(auto *Gep = dyn_cast<GetElementPtrInst>(I)){
        if(Gep->getPointerOperand()->getType()->getPointerElementType()
            ->isArrayTy())
          Geps.insert(Gep);
      }
    }
  }
}
void ArrayAccessTransform::TransformGeps(){
  for(auto G: Geps)
    AddBoundsCheck(G);
}
void ArrayAccessTransform::AddBoundsCheck(GetElementPtrInst *Gep){
  LLVMContext *C = &Gep->getContext();
  IRBuilder<> B(Gep);
  BasicBlock::iterator iter(Gep);

  // Create Basic Blocks
  BasicBlock *BeforeCheck = Gep->getParent();
  BasicBlock *AfterCheck = BeforeCheck->splitBasicBlock(iter);
  BasicBlock *FailedCheck = BasicBlock::Create(*C,
      "BoundsCheckFailed", BeforeCheck->getParent());

  removeTerminator(BeforeCheck);

  B.SetInsertPoint(BeforeCheck);

  // Create check
  Value *Passing = NULL;
  Type *IntType = Type::getInt64Ty(*C);
  auto CurrentGep = Gep;
  while(true){
    Type *ArrayType = Gep->getPointerOperand()->getType()->getPointerElementType();
    Constant *Len = ConstantInt::get(IntType, ArrayType->getVectorNumElements());

    auto Idx = Gep->idx_begin();
    Idx++;
    Value *Offset = *Idx;

    Value *Test= B.CreateICmpULT(Offset, Len);
    Passing = (Passing == NULL ? Test : B.CreateAnd(Test, Passing));

    if(auto NextGep = dyn_cast<GetElementPtrInst>(Gep->use_back()))
      Gep = NextGep;
    else
      break;
  }

  B.CreateCondBr(Passing, AfterCheck, FailedCheck);

  B.SetInsertPoint(FailedCheck);
  B.CreateCall(OnError);
  B.CreateBr(AfterCheck);

  B.SetInsertPoint(AfterCheck);
}

