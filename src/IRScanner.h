//===- IRScanner.h - locate memory operations to instrument ---------------===//
//
// IRScanner walks a function and classifies the memory operations the
// instrumentation pass cares about into three work lists:
//
//   * ArrayAccess - a load/store reached through a getelementptr into a
//     stack- or global-allocated array of statically known length.
//   * PtrDeref    - a load/store whose pointer operand is not a trivially
//     safe alloca/global (candidate for a null / use-after-free check).
//   * HeapCall    - a direct call to malloc / calloc / free (realloc is
//     recognised but deliberately left untracked).
//
// It only records work; CheckInjector performs the IR mutation.
//
//===--------------------------------------------------------------------===//
#ifndef MEMSAFETY_IRSCANNER_H
#define MEMSAFETY_IRSCANNER_H

#include "llvm/IR/InstrTypes.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {
class Function;
class Instruction;
class Value;
} // namespace llvm

namespace memsafety {

struct ArrayAccess {
  llvm::Instruction *MemOp;  // the load or store to guard
  llvm::Value *Base;         // the array object (alloca / global)
  llvm::Value *Index;        // the subscript value (last GEP index)
  uint64_t NumElements;      // static array length
  uint64_t ElementSize;      // element size in bytes
};

struct PtrDeref {
  llvm::Instruction *MemOp;  // the load or store to guard
  llvm::Value *Ptr;          // its pointer operand
};

enum class HeapKind { Malloc, Calloc, Free, Realloc };

struct HeapCall {
  llvm::CallBase *Call;
  HeapKind Kind;
};

class IRScanner {
public:
  void scan(llvm::Function &F);

  const llvm::SmallVectorImpl<ArrayAccess> &arrayAccesses() const {
    return ArrayAccesses;
  }
  const llvm::SmallVectorImpl<PtrDeref> &ptrDerefs() const { return PtrDerefs; }
  const llvm::SmallVectorImpl<HeapCall> &heapCalls() const { return HeapCalls; }

private:
  llvm::SmallVector<ArrayAccess, 16> ArrayAccesses;
  llvm::SmallVector<PtrDeref, 16> PtrDerefs;
  llvm::SmallVector<HeapCall, 8> HeapCalls;
};

} // namespace memsafety

#endif // MEMSAFETY_IRSCANNER_H
