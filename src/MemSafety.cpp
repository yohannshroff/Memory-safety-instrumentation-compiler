//===- MemSafety.cpp - the "memsafety" instrumentation pass --------------===//
//
// New Pass Manager plugin. Registers a module pass named "memsafety" that:
//   * guards array subscripts into known-length stack/global arrays with
//     boundscheck(),
//   * guards other pointer dereferences with ptrcheck() (null / use-after-free),
//   * brackets malloc/calloc/free calls with heap_register() / heap_release().
//
// The actual IR walking lives in IRScanner; the IR mutation in CheckInjector.
// Set MEMSAFE_VERBOSE in the environment to print a per-module summary while
// `opt` runs.
//
//===--------------------------------------------------------------------===//
#include "CheckInjector.h"
#include "IRScanner.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
// PassPlugin.h moved from llvm/Passes/ to llvm/Plugins/ in LLVM 20.
#if __has_include("llvm/Passes/PassPlugin.h")
#include "llvm/Passes/PassPlugin.h"
#else
#include "llvm/Plugins/PassPlugin.h"
#endif
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>

using namespace llvm;

namespace {

class MemSafetyPass : public PassInfoMixin<MemSafetyPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    memsafety::CheckInjector Injector(M);

    unsigned NumBounds = 0, NumHeapBounds = 0, NumPtr = 0, NumHeap = 0;
    bool Changed = false;

    for (Function &F : M) {
      if (F.isDeclaration())
        continue;

      memsafety::IRScanner Scanner;
      Scanner.scan(F);

      for (const auto &AA : Scanner.arrayAccesses()) {
        Changed |= Injector.injectArrayAccess(AA);
        ++NumBounds;
      }
      for (const auto &HA : Scanner.heapPtrAccesses()) {
        Changed |= Injector.injectHeapPtrAccess(HA);
        ++NumHeapBounds;
      }
      for (const auto &PD : Scanner.ptrDerefs()) {
        Changed |= Injector.injectPtrDeref(PD);
        ++NumPtr;
      }
      for (const auto &HC : Scanner.heapCalls()) {
        Changed |= Injector.injectHeapCall(HC);
        ++NumHeap;
      }
    }

    if (std::getenv("MEMSAFE_VERBOSE")) {
      errs() << "[memsafety] " << M.getName() << ": " << NumBounds
             << " bounds checks, " << NumHeapBounds
             << " heap-offset checks, " << NumPtr << " pointer checks, "
             << NumHeap << " heap ops instrumented\n";
    }

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};

} // namespace

//===--------------------------------------------------------------------===//
// Plugin registration
//===--------------------------------------------------------------------===//

static llvm::PassPluginLibraryInfo getMemSafetyPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "MemSafety", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "memsafety") {
                    MPM.addPass(MemSafetyPass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getMemSafetyPluginInfo();
}
