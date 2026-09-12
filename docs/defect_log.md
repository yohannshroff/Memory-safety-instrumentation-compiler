# Defect / Issue Log

Defects found and fixed during implementation, in the order they were hit.
Kept for Review 2's "testing, debugging and correctness evidence" requirement
— this is a real log, not a reconstruction.

| # | Symptom | Where | Root cause | Fix | Status |
|---|---|---|---|---|---|
| D1 | `opt`: "Plugin entry point not found ... Is this a legacy plugin?" | loading `libMemSafety.so` | `CXX_VISIBILITY_PRESET hidden` on the `MemSafety` CMake target hid the `llvmGetPassPluginInfo` symbol | dropped the hidden-visibility preset (`src/CMakeLists.txt`); kept default visibility on the plugin target | Fixed |
| D2 | Compile error: `'llvm/Passes/PassPlugin.h' file not found` | building `src/MemSafety.cpp` against LLVM 23 (Homebrew) | header relocated from `llvm/Passes/` to `llvm/Plugins/` in LLVM 20+ | `#if __has_include(...)` guard picks whichever path exists (`src/MemSafety.cpp`) | Fixed |
| D3 | Compile error: `'llvm/IR/FunctionType.h' file not found` | building `src/CheckInjector.h` | wrong header name; `FunctionType`/`FunctionCallee` live in `llvm/IR/DerivedTypes.h` | corrected the include | Fixed |
| D4 | `opt` crashes (SIGSEGV) inside `IRBuilderBase::CreateGlobalString` | running the pass on any benchmark | `IRBuilder<> B(Ctx)` has no insertion block, and `CreateGlobalString` needs a `Module*` to attach the global to when there is none | pass `&M` explicitly as the 4th argument (`CheckInjector::locString`) | Fixed |
| D5 | `run_all.sh: value too great for base (error token is "01_safe_array")` | running the benchmark harness on macOS | `declare -A` (associative arrays) used in the harness; the stock macOS `/bin/bash` is 3.2, which has no associative arrays | rewrote `expect_for()` as a `case` statement — portable to bash 3.2+ and to CI's bash 5 | Fixed |
| D6 | Harness output cluttered with `run_all.sh: line N: PID Abort trap: 6 ...` | running the benchmark harness | the shell prints a job-control notice when a *foreground* command dies from a signal; the instrumented binaries `abort()` on purpose | wrapped the run in `{ "$bin" ...; } 2>/dev/null || rc=$?` so only that notice is discarded — the program's own stderr is still captured separately | Fixed |
| D7 | `CMake Error: ... directory ... is different than the directory ... where CMakeCache.txt was created` | rebuilding after the project folder was moved (`.../Compiler/Project` → `.../ComD/Project`) | `CMakeCache.txt` records an absolute source/build path | `./build.sh --clean` (operational note, not a code defect: moving/renaming the repo root requires a clean reconfigure) | Fixed / documented |
| D8 | `09_pointer_arithmetic.c`'s heap out-of-bounds write via `*(p + 4)` went undetected | benchmark harness (marked `XFAIL` at the time) | bounds checking only covered arrays with a compile-time-known length (stack/global); a heap pointer offset by arithmetic has no such length in the IR | added `heap_boundscheck()`: a dynamic lookup of the GEP's base pointer in the existing allocation table, checked against the *recorded* allocation size at run time (see `docs/design_notes.md`) | Fixed — see commit `dd088d5` |

## Currently open (not defects — documented scope boundaries)

These are not bugs; they are stated limitations (see `README.md` § Known
Limitations) tracked here so they aren't silently rediscovered later:

- `realloc` is recognised by the pass but not instrumented.
- `heap_boundscheck` only resolves a base pointer that is *exactly* a live
  `malloc`/`calloc` return value — no general aliasing/provenance analysis.
- The runtime has no locking; concurrent programs are out of scope.
- `scripts/measure_overhead.sh` is a stub (Milestone 5, scheduled for Review 3 prep).
