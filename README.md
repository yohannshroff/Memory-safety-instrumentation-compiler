# Memory-Safety Instrumentation Compiler

BCSE307 Compiler Design Project — Team A29 (Dhruv, Yohann, Vikas, Prajwal), VIT Vellore.

An out-of-tree **LLVM instrumentation pass** that rewrites C programs at the IR
level to catch memory-safety violations at run time:

| Violation class | Example |
|---|---|
| Array out-of-bounds read / write (stack/global array) | `int a[4]; a[4] = x;` |
| Heap out-of-bounds access (subscript or pointer arithmetic) | `int *p = malloc(16); p[4] = x;` / `*(p + 4) = x;` |
| Invalid pointer dereference (NULL) | `int *p = 0; *p = 1;` |
| Use-after-free | `free(p); *p = 1;` |
| Double free | `free(p); free(p);` |
| Invalid free (non-heap address) | `int x; free(&x);` |

The pass (`src/`) inserts calls to a small dependency-free C runtime library
(`runtime/`) around array subscripts, pointer dereferences, and
`malloc`/`calloc`/`free` calls. The `driver/memsafec` wrapper chains
`clang → opt → clang` into a single "compiler" command for demos.

This is a bounded educational prototype, not a replacement for AddressSanitizer
or Valgrind. See [Known Limitations](#known-limitations).

## Prerequisites

- **LLVM + Clang** development package (LLVM 15 or newer; tested on 18 and 23)
  - Ubuntu/Debian: `sudo apt-get install llvm-dev clang cmake`
  - macOS: `brew install llvm cmake`
- **CMake** ≥ 3.16, a C11 and C++17 compiler

## Build

One command (auto-detects Homebrew LLVM on macOS, LLVM on `PATH` elsewhere):

```bash
./build.sh            # configure + build + run unit tests
./build.sh --bench    # also run the end-to-end benchmark suite
./build.sh --clean    # wipe build/ first
```

Or drive CMake directly:

```bash
cmake -B build                     # Linux, LLVM on PATH
#   macOS / Homebrew LLVM:
# cmake -B build -DLLVM_DIR="$(brew --prefix llvm)/lib/cmake/llvm"
cmake --build build
```

CMake presets are also provided (`cmake --preset macos-brew` / `--preset default`).

Artifacts:

- `build/src/libMemSafety.so` — the pass plugin (`.dylib` on macOS)
- `build/runtime/libmemsafety_runtime.a` — the runtime library
- `build/tests/test_runtime` — runtime unit tests

## Test

```bash
ctest --test-dir build --output-on-failure   # runtime unit tests
./benchmarks/run_all.sh                       # end-to-end benchmark suite
```

## Quick Demo
```bash
# A safe program: builds and runs cleanly.
./driver/memsafec benchmarks/safe/01_safe_array.c -o /tmp/safe
/tmp/safe; echo "exit $?"          # -> prints 30, exit 0

# An out-of-bounds read: the instrumented binary reports and aborts.
./driver/memsafec benchmarks/unsafe/02_oob_read.c -o /tmp/oob
/tmp/oob; echo "exit $?"
# -> [VIOLATION] out-of-bounds access at benchmarks/unsafe/02_oob_read.c:4, addr=0x...
#    exit 134

# Compare against an uninstrumented baseline build:
./driver/memsafec --no-instrument benchmarks/unsafe/02_oob_read.c -o /tmp/oob0
```

`memsafec -v <file> -o <bin>` echoes the three underlying commands.

## Repository layout

| Path | Contents | Owner |
|---|---|---|
| `src/` | LLVM pass: `IRScanner` (find ops), `CheckInjector` (insert checks), `MemSafety` (plugin) | Vikas |
| `runtime/` | C runtime: allocation metadata table + `boundscheck` / `heap_boundscheck` / `ptrcheck` / `heap_register` / `heap_release` | Yohann |
| `driver/` | `memsafec` compiler-driver script | Prajwal |
| `tests/` | `test_runtime.c` — direct unit tests of the runtime | Prajwal |
| `benchmarks/` | `safe/` and `unsafe/` C programs + `run_all.sh` harness | Prajwal / Yohann |
| `docs/` | architecture, test plan, design notes, risk log | Dhruv / Prajwal |
| `.github/workflows/` | CI: build + test + benchmark suite | Prajwal |

## Known Limitations

These are deliberate scope boundaries for an undergraduate prototype, kept
visible rather than silently worked around:

- **No `realloc` tracking.** `realloc` would require *updating* a metadata
  entry rather than a clean add/remove; it is recognised but left untracked.
- **No static "provably in bounds" skip.** Every candidate access is
  instrumented, including ones a compiler could prove safe. Lower run-time
  overhead via static elision is future work.
- **Heap bounds checking only covers pointer values that exactly match a
  live `malloc`/`calloc` result.** `heap_boundscheck` looks the base pointer
  up in the same allocation table used for free-tracking; an offset computed
  from a pointer the runtime never registered (e.g. one derived by pointer
  arithmetic across an `alloca`, or a pointer that arrived from outside the
  instrumented translation unit) is not checked. No general pointer-provenance
  / aliasing analysis is attempted.
- **Single-threaded runtime.** The metadata table has no locking.
- **Platforms:** developed and tested on x86-64 Linux and arm64 macOS.
- First violation aborts the process (`abort()`, exit 134); the program does
  not continue past it.
