//===- IRScanner.cpp ----------------------------------------------------===//
#include "IRScanner.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

using namespace llvm;
using namespace memsafety;

namespace {

// Return the pointer operand of a load/store, or null for any other opcode.
Value *memPointerOperand(Instruction &I) {
  if (auto *LI = dyn_cast<LoadInst>(&I))
    return LI->getPointerOperand();
  if (auto *SI = dyn_cast<StoreInst>(&I))
    return SI->getPointerOperand();
  return nullptr;
}

// If `V` is an array object with a statically known length - an alloca of
// ArrayType, or a global of ArrayType - report its element type and count.
bool arrayObjectShape(Value *V, Type *&ElemTy, uint64_t &NumElems) {
  if (auto *AI = dyn_cast<AllocaInst>(V)) {
    if (auto *AT = dyn_cast<ArrayType>(AI->getAllocatedType())) {
      ElemTy = AT->getElementType();
      NumElems = AT->getNumElements();
      return true;
    }
  }
  if (auto *GV = dyn_cast<GlobalVariable>(V)) {
    if (auto *AT = dyn_cast<ArrayType>(GV->getValueType())) {
      ElemTy = AT->getElementType();
      NumElems = AT->getNumElements();
      return true;
    }
  }
  return false;
}

HeapKind *heapKindFor(CallBase *CB, HeapKind &Storage) {
  Function *Callee = CB->getCalledFunction();
  if (!Callee)
    return nullptr;
  StringRef Name = Callee->getName();
  if (Name == "malloc")
    Storage = HeapKind::Malloc;
  else if (Name == "calloc")
    Storage = HeapKind::Calloc;
  else if (Name == "free")
    Storage = HeapKind::Free;
  else if (Name == "realloc")
    Storage = HeapKind::Realloc;
  else
    return nullptr;
  return &Storage;
}

} // namespace

void IRScanner::scan(Function &F) {
  const DataLayout &DL = F.getParent()->getDataLayout();

  for (Instruction &I : instructions(F)) {
    // --- heap calls -------------------------------------------------------
    if (auto *CB = dyn_cast<CallBase>(&I)) {
      HeapKind K;
      if (heapKindFor(CB, K))
        HeapCalls.push_back({CB, K});
      continue;
    }

    // --- memory accesses ------------------------------------------------
    Value *PtrOp = memPointerOperand(I);
    if (!PtrOp)
      continue;

    Value *Stripped = PtrOp->stripPointerCasts();

    // Array subscript: load/store fed by a GEP into a known-length array.
    if (auto *GEP = dyn_cast<GetElementPtrInst>(Stripped)) {
      Value *ArrObj = GEP->getPointerOperand()->stripPointerCasts();
      Type *ElemTy = nullptr;
      uint64_t NumElems = 0;
      if (GEP->getNumIndices() >= 1 &&
          arrayObjectShape(ArrObj, ElemTy, NumElems)) {
        Value *Index = GEP->getOperand(GEP->getNumOperands() - 1);
        ArrayAccesses.push_back(
            {&I, ArrObj, Index, NumElems,
             DL.getTypeAllocSize(ElemTy).getFixedValue()});
        continue;
      }

      // Not a known-length array: a flat, single-index GEP (`p[i]`,
      // `*(p + k)`) off some other pointer may be an offset into a live
      // heap allocation. We can't know its size statically, so check it
      // dynamically at run time against whatever the runtime's metadata
      // table has on record for that exact base pointer value - see
      // heap_boundscheck() in runtime/memsafety_runtime.c. If the base
      // turns out not to be a tracked allocation, the check is a no-op.
      if (GEP->getNumIndices() == 1) {
        Value *Index = GEP->getOperand(GEP->getNumOperands() - 1);
        uint64_t ElemSize =
            DL.getTypeAllocSize(GEP->getSourceElementType()).getFixedValue();
        HeapPtrAccesses.push_back(
            {&I, GEP->getPointerOperand(), Index, ElemSize});
      }
      // Fall through: still also apply the ptrcheck below (null / UAF).
    }

    // Otherwise: guard the dereference unless the pointer is obviously a
    // live stack slot or global (those are always valid to address).
    if (isa<AllocaInst>(Stripped) || isa<GlobalVariable>(Stripped))
      continue;
    PtrDerefs.push_back({&I, PtrOp});
  }
}
