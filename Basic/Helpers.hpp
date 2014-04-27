#ifndef HELPERS
#define HELPERS

#include <set>
#include <vector>
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "../PointerAnalysis/Pointer.hpp"

using namespace llvm;

const std::string red = "\033[31m";
const std::string green = "\033[32m";
const std::string yellow = "\033[33m";
const std::string blue = "\033[34m";
const std::string reset = "\033[0m";

void PrintIrWithHighlight(Module &M, std::set<Instruction *> H1);
void PrintIrWithHighlight(Module &M, std::set<Instruction *> H1,
    std::set<Instruction *> H2);
void PrintIrWithHighlight(Module &M, std::set<Instruction *> H1,
    std::set<Instruction *> H2, std::set<Instruction *> H3);
void PrintIrWithHighlight(Module &M, std::set<Instruction *> H1,
    std::set<Instruction *> H2, std::set<Instruction *> H3,
    std::set<Instruction *> H4);

unsigned int GetNumElementsInArray(AllocaInst *);
unsigned int GetArrayElementSizeInBits(AllocaInst *, DataLayout *);

void removeTerminator(BasicBlock *BB);
std::vector<Value *> GetIndices(int val, LLVMContext& C);
Value* Str(IRBuilder<> B, std::string str);

enum LinkType {LOAD, GEP, CAST, NO_LINK};

LinkType GetLinkType(Value *V);
Value *GetNextLink(Value *Link);
Pointer GetOriginator(Value *Link, int level=0);

enum PointerDestination {OTHER, RETURN, CALL, STORE};
PointerDestination GetDestination(Value *Link);

int CountPointerLevels(Type *);

void StoreInFatPointerValue(Value *FatPointer, Value *Val, IRBuilder<> &B);
void StoreInFatPointerBase(Value *FatPointer, Value *Val, IRBuilder<> &B);
void StoreInFatPointerBound(Value *FatPointer, Value *Val, IRBuilder<> &B);

Value *LoadFatPointerValue(Value *FatPointer, IRBuilder<> &B);
Value *LoadFatPointerBase(Value *FatPointer, IRBuilder<> &B);
Value *LoadFatPointerBound(Value *FatPointer, IRBuilder<> &B);

Value *GetFatPointerValueAddr(Value *FatPointer, IRBuilder<> &B);
Value *GetFatPointerBaseAddr(Value *FatPointer, IRBuilder<> &B);
Value *GetFatPointerBoundAddr(Value *FatPointer, IRBuilder<> &B);

Value *GetSizeValue(Type *T, IRBuilder<> &B);

void SetFatPointerToAddress(Value *FatPointer, Value *Address, IRBuilder<> B);
void SetFatPointerBaseAndBound(Value *FP, Value *Base, Value *Size, IRBuilder<> B);
Value *CalculateBound(Value *Base, Value *Size, IRBuilder<> B);
#endif
