#ifndef TYPE_DUPLICATER
#define TYPE_DUPLICATER

#include <set>
#include <map>
#include "llvm/IR/Module.h"
#include "llvm/Analysis/FindUsedTypes.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

using namespace llvm;

class TypeDuplicater : public ValueMapTypeRemapper{
public:
  TypeDuplicater(Module &M, FindUsedTypes *FUT);

  virtual Type *remapType(Type *srcType);

  std::set<StructType *> GetFPTypes();
  std::set<StructType *> GetRawTypes();

  // This is a Type * -> StructType * for each of use, so we don't need to cast
  // the key to a StructType.
  std::map<Type *, StructType *> RawToFPMap;
private:
  bool NeedsFPType(StructType *ST);
  void CreateSkeletonTypes();
  void FillTypeBodies();
  void DisplayFPTypes(DataLayout *DL);

  std::set<StructType *> RawStructs;
  std::set<StructType *> FPStructs;
};

#endif
