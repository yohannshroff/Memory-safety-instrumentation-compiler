//===- CheckInjector.cpp ------------------------------------------------===//
#include "CheckInjector.h"

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"

using namespace llvm;
using namespace memsafety;

CheckInjector::CheckInjector(Module &M) : M(M), Ctx(M.getContext()) {
  Type *VoidTy = Type::getVoidTy(Ctx);
  PointerType *PtrTy = PointerType::get(Ctx, 0);
  Type *I64Ty = Type::getInt64Ty(Ctx);

  // void boundscheck(ptr base, i64 elem_size, i64 len, i64 index, ptr loc)
  BoundsCheckFn = M.getOrInsertFunction(
      "boundscheck",
      FunctionType::get(VoidTy, {PtrTy, I64Ty, I64Ty, I64Ty, PtrTy}, false));

  // void heap_boundscheck(ptr base, i64 byte_offset, i64 access_size, ptr loc)
  HeapBoundsCheckFn = M.getOrInsertFunction(
      "heap_boundscheck",
      FunctionType::get(VoidTy, {PtrTy, I64Ty, I64Ty, PtrTy}, false));

  // void ptrcheck(ptr p, ptr loc)
  PtrCheckFn = M.getOrInsertFunction(
      "ptrcheck", FunctionType::get(VoidTy, {PtrTy, PtrTy}, false));

  // void heap_register(ptr p, i64 size, ptr loc)
  HeapRegisterFn = M.getOrInsertFunction(
      "heap_register", FunctionType::get(VoidTy, {PtrTy, I64Ty, PtrTy}, false));

  // void heap_release(ptr p, ptr loc)
  HeapReleaseFn = M.getOrInsertFunction(
      "heap_release", FunctionType::get(VoidTy, {PtrTy, PtrTy}, false));
}

Constant *CheckInjector::locString(Instruction *I) {
  std::string Loc = "<unknown>";
  if (const DebugLoc &DL = I->getDebugLoc()) {
    if (auto *Scope = dyn_cast_or_null<DIScope>(DL.getScope()))
      Loc = (Scope->getFilename() + ":" + Twine(DL.getLine())).str();
    else
      Loc = (Twine("line ") + Twine(DL.getLine())).str();
  }

  auto It = LocStrings.find(Loc);
  if (It != LocStrings.end())
    return It->second;

  // Build the string global directly against the module (no insertion point).
  IRBuilder<> B(Ctx);
  Constant *GV = B.CreateGlobalString(Loc, ".memsafe.loc",
                                     /*AddressSpace=*/0, &M);
  LocStrings[Loc] = GV;
  return GV;
}

bool CheckInjector::injectArrayAccess(const ArrayAccess &AA) {
  IRBuilder<> B(AA.MemOp);
  Type *I64Ty = B.getInt64Ty();

  Value *Idx = B.CreateSExtOrTrunc(AA.Index, I64Ty, "memsafe.idx");
  B.CreateCall(BoundsCheckFn,
               {AA.Base, ConstantInt::get(I64Ty, AA.ElementSize),
                ConstantInt::get(I64Ty, AA.NumElements), Idx,
                locString(AA.MemOp)});
  return true;
}

bool CheckInjector::injectHeapPtrAccess(const HeapPtrAccess &HA) {
  IRBuilder<> B(HA.MemOp);
  Type *I64Ty = B.getInt64Ty();

  Value *Idx = B.CreateSExtOrTrunc(HA.Index, I64Ty, "memsafe.hidx");
  Value *ByteOffset =
      B.CreateMul(Idx, ConstantInt::get(I64Ty, HA.ElementSize), "memsafe.hoff");
  B.CreateCall(HeapBoundsCheckFn,
               {HA.GepBase, ByteOffset, ConstantInt::get(I64Ty, HA.ElementSize),
                locString(HA.MemOp)});
  return true;
}

bool CheckInjector::injectPtrDeref(const PtrDeref &PD) {
  IRBuilder<> B(PD.MemOp);
  B.CreateCall(PtrCheckFn, {PD.Ptr, locString(PD.MemOp)});
  return true;
}

bool CheckInjector::injectHeapCall(const HeapCall &HC) {
  Type *I64Ty = Type::getInt64Ty(Ctx);

  switch (HC.Kind) {
  case HeapKind::Malloc: {
    // heap_register(result, size_arg, loc) immediately after the call
    IRBuilder<> B(HC.Call->getNextNode());
    Value *Size = B.CreateZExtOrTrunc(HC.Call->getArgOperand(0), I64Ty);
    B.CreateCall(HeapRegisterFn, {HC.Call, Size, locString(HC.Call)});
    return true;
  }
  case HeapKind::Calloc: {
    IRBuilder<> B(HC.Call->getNextNode());
    Value *N = B.CreateZExtOrTrunc(HC.Call->getArgOperand(0), I64Ty);
    Value *Sz = B.CreateZExtOrTrunc(HC.Call->getArgOperand(1), I64Ty);
    Value *Total = B.CreateMul(N, Sz, "memsafe.callocsz");
    B.CreateCall(HeapRegisterFn, {HC.Call, Total, locString(HC.Call)});
    return true;
  }
  case HeapKind::Free: {
    // heap_release(ptr_arg, loc) immediately before the call
    IRBuilder<> B(HC.Call);
    B.CreateCall(HeapReleaseFn,
                 {HC.Call->getArgOperand(0), locString(HC.Call)});
    return true;
  }
  case HeapKind::Realloc:
    // Out of scope - documented limitation (would require updating a metadata
    // entry rather than a clean add/remove). Left untracked on purpose.
    return false;
  }
  return false;
}
