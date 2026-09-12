//===- CheckInjector.h - insert runtime check calls into the IR ----------===//
//
// CheckInjector owns the declarations of the runtime entry points and rewrites
// the IR: it inserts boundscheck / heap_boundscheck / ptrcheck calls in front
// of guarded memory operations and heap_register / heap_release calls around
// heap calls.
//
//===--------------------------------------------------------------------===//
#ifndef MEMSAFETY_CHECKINJECTOR_H
#define MEMSAFETY_CHECKINJECTOR_H

#include "IRScanner.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/IR/DerivedTypes.h"

namespace llvm {
class Constant;
class Instruction;
class Module;
} // namespace llvm

namespace memsafety {

class CheckInjector {
public:
  explicit CheckInjector(llvm::Module &M);

  // Returns true if any IR was modified.
  bool injectArrayAccess(const ArrayAccess &AA);
  bool injectHeapPtrAccess(const HeapPtrAccess &HA);
  bool injectPtrDeref(const PtrDeref &PD);
  bool injectHeapCall(const HeapCall &HC);

private:
  llvm::Constant *locString(llvm::Instruction *I);

  llvm::Module &M;
  llvm::LLVMContext &Ctx;

  llvm::FunctionCallee BoundsCheckFn;
  llvm::FunctionCallee HeapBoundsCheckFn;
  llvm::FunctionCallee PtrCheckFn;
  llvm::FunctionCallee HeapRegisterFn;
  llvm::FunctionCallee HeapReleaseFn;

  llvm::StringMap<llvm::Constant *> LocStrings;
};

} // namespace memsafety

#endif // MEMSAFETY_CHECKINJECTOR_H
