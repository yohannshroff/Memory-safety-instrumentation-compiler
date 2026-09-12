# Review 2 Progress Document

**Memory-Safety Instrumentation Compiler**

| | |
|---|---|
| Course Code / Title | [COURSE CODE] — BCSE307 Compiler Design |
| Team Number | [TEAM NUMBER] — A29 |
| Project ID / Title | Memory-Safety Instrumentation Compiler |
| Team Members | Dhruv [REGISTER NO.] · Yohann [REGISTER NO.] · Vikas [REGISTER NO.] · Prajwal [REGISTER NO.] |
| Review Date | [REVIEW DATE] |
| Faculty / Evaluator | [FACULTY NAME] |

*This document updates the Review 1 submission with implementation evidence.
It does not repeat Review 1 content that hasn't changed; see §1 for what did.*

---

## 1. Review 1 Feedback and Compliance

See `docs/review1_corrections.md` for the full observation/action/evidence
table. Summary: of 7 self-assessed Review 1 gaps, 5 are closed (working
pipeline, runtime implementation, verified detection scope, heap
pointer-arithmetic bounds, reproducible build/CI), 1 is open by plan
(overhead measurement — scheduled for Review 3 prep), and 1 is partially
addressed (per-member git-commit evidence — see §10). No architecture
decision locked in at Review 1 was reversed.

## 2. Updated Abstract, Objectives and Scope

**Abstract.** C provides no automatic protection against spatial (array
out-of-bounds) or temporal (use-after-free, double-free) memory errors. This
project instruments C programs at the LLVM IR level with a New Pass Manager
plugin that inserts runtime checks around array subscripts, pointer
dereferences, and heap allocation/deallocation calls, backed by a small
dependency-free C runtime library that tracks live allocations in a hash
table. At Review 2, the pipeline is complete and integrated end-to-end: 12
benchmark programs covering 6 violation classes (stack OOB read/write, heap
OOB via subscript and via pointer arithmetic, null dereference,
use-after-free, double-free, invalid free) all produce their expected
instrumented behaviour, verified by an automated harness.

**Objectives (status).**

1. Detect array out-of-bounds accesses on statically-sized arrays — **done**.
2. Detect out-of-bounds accesses on heap buffers (subscript and pointer
   arithmetic) — **done** (added at Review 2; was an open gap at Review 1).
3. Detect null-pointer dereference — **done**.
4. Detect use-after-free, double-free, and invalid-free — **done**.
5. Provide a single-command "compiler" (`memsafec`) usable for a live demo —
   **done**.
6. Measure runtime overhead of instrumentation vs. baseline — **not started**
   (Milestone 5; scheduled before Review 3).

**Scope — unchanged from Review 1.** In scope: C programs compiled with
Clang/LLVM, spatial safety for arrays and heap buffers, temporal safety for
heap objects, single-threaded programs. Out of scope: `realloc` tracking,
static "provably safe" elision (everything is instrumented), multithreading,
general pointer-provenance/aliasing analysis. See `README.md` § Known
Limitations for the authoritative list.

## 3. Updated Architecture and Module Interfaces

No architectural change from Review 1's locked-in design (LLVM New Pass
Manager plugin; hash-table metadata store, not shadow memory; GEP+load/store
and malloc/calloc/free instrumentation; `memsafec` as the compiler driver;
plain-C dependency-free runtime). Full diagram and module interface tables
are in `docs/architecture.md`; reproduced here in brief:

```
C source --clang(-emit-llvm)--> IR --opt(-passes=memsafety)--> instrumented IR --clang(link runtime)--> executable
```

- `src/IRScanner` classifies each function's memory operations into four
  work lists: `ArrayAccess`, `HeapPtrAccess` *(new)*, `PtrDeref`, `HeapCall`.
- `src/CheckInjector` declares the five runtime entry points via
  `Module::getOrInsertFunction` and inserts guard calls with `IRBuilder`.
- `src/MemSafety` is the plugin registration (`opt -passes=memsafety`) that
  drives Scanner → Injector per function.
- `runtime/memsafety_runtime.c` implements the metadata table and the five
  checks: `boundscheck`, `heap_boundscheck` *(new)*, `ptrcheck`,
  `heap_register`, `heap_release`, plus `report_violation`.
- `driver/memsafec` is the integration point: it chains the three
  command-line tools into one invocation.

**Interface contract (pass ↔ runtime), unchanged in ABI terms except for one
addition:**

```c
void boundscheck(void *base, size_t elem_size, size_t len, long index, const char *loc);
void heap_boundscheck(void *base, long byte_offset, size_t access_size, const char *loc);  /* new */
void ptrcheck(void *ptr, const char *loc);
void heap_register(void *ptr, size_t size, const char *loc);
void heap_release(void *ptr, const char *loc);
```

## 4. Implementation Progress Summary

See `docs/progress_tracker.md` for the full planned-vs-completed table.
**Headline: ~85% of the M0–M6 milestone plan is complete.** All core
detection milestones (M0–M4, plus the M3.5 heap-bounds extension added for
Review 2) are done and verified; the CI workflow is written but not yet
validated against a live GitHub remote; overhead measurement (M5) has not
started beyond a stub script.

## 5. Module 1 Implementation — Runtime Library (`runtime/`)

**Design.** A single open-addressing hash table (linear probing, grows past
70% load factor) maps a heap base pointer to `{size, freed, alloc_site}`.
Freed entries are retained (not deleted) so that a later access through the
same pointer can still be recognised as use-after-free.

**Algorithm (`heap_boundscheck`, the Review-2 addition):**

```
heap_boundscheck(base, byte_offset, access_size, loc):
    entry = table_lookup(base)
    if entry is None:
        return                      # base isn't a tracked allocation - no-op
    if byte_offset < 0 or byte_offset + access_size > entry.size:
        report_violation("out-of-bounds access", loc, base + byte_offset)
```

**Source structure:** `runtime/memsafety_runtime.h` (public API, documented
per-function), `runtime/memsafety_runtime.c` (~174 lines: hash function,
table init/grow/lookup, then the five public functions).

**Input/output:** every function takes raw pointers/sizes/a location string
and either returns normally or calls `report_violation`, which prints
`[VIOLATION] <type> at <loc>, addr=<addr>` to stderr and `abort()`s
(exit 134) — chosen over `exit(1)` so the pass/fail signal from the benchmark
harness is unambiguous and the unsafe access itself never executes.

**Current limitations:** no `realloc` support; `heap_boundscheck` only
resolves a pointer that is *exactly* a live `malloc`/`calloc` return value
(no aliasing analysis); no locking (single-threaded only). All three are
deliberate scope boundaries, not oversights — see `docs/design_notes.md`.

## 6. Module 2 Implementation — LLVM Pass (`src/`)

**Design.** A New Pass Manager module pass (`opt -passes=memsafety`) that,
per function, walks every instruction once (`IRScanner::scan`) and then
mutates the collected work lists (`CheckInjector`).

**Algorithm (classification, `IRScanner::scan`):**

```
for each instruction I in F:
    if I is a call to malloc/calloc/free/realloc: record as HeapCall
    if I is a load/store:
        ptr = I's pointer operand, stripped of casts
        if ptr is a GEP into an alloca/global of known ArrayType:
            record ArrayAccess(base, index, num_elements, elem_size)
        else if ptr is a single-index GEP off some other pointer:
            record HeapPtrAccess(gep_base, index, elem_size)   # new at Review 2
        if ptr is not directly an alloca/global:
            record PtrDeref(ptr)
```

**Transformation example** (`benchmarks/unsafe/09_pointer_arithmetic.c`,
`*(p + 4) = 9` where `p = malloc(4 * sizeof(int))`):

```llvm
; before
%13 = getelementptr inbounds i32, ptr %12, i64 4, !dbg !29
store i32 9, ptr %13, align 4, !dbg !30

; after memsafety
%13 = getelementptr inbounds i32, ptr %12, i64 4, !dbg !29
call void @heap_boundscheck(ptr %12, i64 16, i64 4, ptr @.memsafe.loc), !dbg !30
call void @ptrcheck(ptr %13, ptr @.memsafe.loc), !dbg !30
store i32 9, ptr %13, align 4, !dbg !30
```

`heap_boundscheck` receives `byte_offset = index(4) * elem_size(4) = 16`; the
allocation is 16 bytes, so `16 + 4 > 16` → reported and the store never runs.

**Source structure:** `IRScanner.{h,cpp}` (~85 + 123 lines), `CheckInjector.{h,cpp}`
(~52 + 127 lines), `MemSafety.cpp` (~105 lines) — plugin registration and the
per-function drive loop.

**Current limitations:** single-index GEPs only (no `p[i][j]`-style
multi-dimensional heap indexing yet); no static "provably safe" elision —
every candidate access is instrumented, which is correct but not
overhead-optimal.

## 7. Integration / Prototype

`driver/memsafec` is the integrated prototype: `memsafec input.c -o out`
runs `clang -S -emit-llvm` → `opt -load-pass-plugin libMemSafety -passes=memsafety`
→ `clang <instrumented> <runtime.a> -o out` as one command, auto-locating the
plugin/runtime under `build/` (overridable via `MEMSAFE_PLUGIN`/`MEMSAFE_RT`/
`MEMSAFE_CLANG`/`MEMSAFE_OPT`). `-v` echoes each underlying command;
`--no-instrument` builds an uninstrumented baseline for comparison.

**End-to-end demonstration:**

```
$ ./driver/memsafec benchmarks/safe/01_safe_array.c -o /tmp/safe && /tmp/safe
30                                              # exit 0, clean

$ ./driver/memsafec benchmarks/unsafe/09_pointer_arithmetic.c -o /tmp/oob && /tmp/oob
[VIOLATION] out-of-bounds access at benchmarks/unsafe/09_pointer_arithmetic.c:7, addr=0x...
                                                 # exit 134
```

A `safe/` program produces byte-identical stdout whether built with or
without `--no-instrument`, confirming the instrumentation is transparent when
there is nothing to report.

## 8. Testing and Debugging

**Test levels:** unit (`tests/test_runtime.c` via `ctest`, no LLVM involved),
integration (`memsafec` on a single file), system (`benchmarks/run_all.sh`
over the full corpus). Full plan in `docs/test_plan.md`.

**Results as of this submission:**

| Suite | Result |
|---|---|
| `ctest` (runtime unit tests) | **16 / 16 pass** |
| `benchmarks/run_all.sh` (system corpus) | **12 / 12 pass, 0 fail, 0 xfail** |

**Representative test-case table** (full corpus in `docs/test_plan.md`):

| Benchmark | Category | Expected | Actual | Pass/Fail |
|---|---|---|---|---|
| `safe/01_safe_array` | positive | exit 0, no violation | exit 0, no violation | Pass |
| `safe/12_boundary_last_index` | boundary | exit 0 (exact last valid index, stack + heap) | exit 0 | Pass |
| `unsafe/02_oob_read` | negative | `out-of-bounds access` | `out-of-bounds access at ...02_oob_read.c:4` | Pass |
| `unsafe/09_pointer_arithmetic` | negative, heap | `out-of-bounds access` | `out-of-bounds access at ...09_pointer_arithmetic.c:7` | Pass |
| `unsafe/11_heap_index_oob` | negative, heap, boundary | `out-of-bounds access` | `out-of-bounds access at ...11_heap_index_oob.c:5` | Pass |
| `unsafe/06_double_free` | negative, invalid | `double free` | `double free at ...06_double_free.c:6` | Pass |

**Defect/debugging evidence:** 8 real defects were found and fixed while
building this — header relocation across LLVM versions, a hidden-visibility
build bug that broke plugin loading, a null-module crash in `IRBuilder`, a
bash-portability bug in the harness, and the pointer-arithmetic detection
gap itself. Full log with root cause and fix: `docs/defect_log.md`.

## 9. Repository and Build Information

- **Structure:** `src/` (pass), `runtime/` (checks), `driver/` (compiler
  wrapper), `tests/` (unit tests), `benchmarks/{safe,unsafe}/` (corpus) +
  `run_all.sh`, `docs/` (this document and its supporting files),
  `.github/workflows/ci.yml`.
- **Build:** `./build.sh` (auto-detects Homebrew LLVM on macOS / LLVM on
  `PATH` on Linux; `--bench` also runs the benchmark suite; `--clean` wipes
  the build directory). CMake presets (`default`, `macos-brew`) are also
  provided. Dependencies: LLVM + Clang ≥ 15 (tested on 18 and 23), CMake
  ≥ 3.16, a C11/C++17 toolchain.
- **Reproduce the demo:** `./build.sh --bench` builds everything, runs the
  unit tests, and runs the full benchmark suite in one command.
- **Branch strategy:** single `main` branch; commit messages prefixed
  `feat:`/`fix:`/`docs:`/`test:`/`chore:` per module touched (repository
  convention).

## 10. Member-Wise Contribution Record

Full table in `docs/contribution_matrix.md`, including the note on current
git-authorship workflow and what each member must be able to demonstrate
live at the Review 2 viva. Summary: Yohann owns and can demo `runtime/`
standalone via `ctest`; Vikas owns and can demo `src/` via `opt -S` IR
diffs; Prajwal owns and can demo the integrated pipeline via `memsafec` and
`run_all.sh`; Dhruv owns the narrative/report and the acceptance-criteria
framing tying benchmarks back to the project's objectives.

## 11. Risks, Issues and Pending Work

Full log in `docs/risk_log.md` (8 tracked risks). Highlights: the heap-bounds
scope gap flagged at Review 1 is now closed (R3); the outstanding graded item
with zero progress is overhead measurement (R6), deliberately scheduled first
in the Review 3 prep timeline so it isn't left to the last minute; the
per-member git-commit evidence gap (R7) is tracked with an explicit plan to
address it before Review 3.

## 12. Revised Timeline for Review 3

Full week-by-week plan in `docs/review2_timeline.md`: Week 10 — implement
real overhead measurement, push to a GitHub remote and validate CI; Week 11 —
independent per-member re-demo/dry-run and address evaluator corrections,
plus a stretch goal (multi-index heap GEPs); Week 12 — final report and
Review 3 rehearsal.

## 13. Preliminary Results

- **Correctness:** 12/12 benchmarks across 6 violation classes (stack OOB
  read/write, heap OOB via subscript, heap OOB via pointer arithmetic, null
  dereference, use-after-free, double-free, invalid-free) plus 2 positive/
  boundary cases produce their expected instrumented behaviour.
- **Transparency:** instrumented safe programs produce byte-identical stdout
  to their uninstrumented baseline.
- **Unit coverage:** 16/16 runtime checks pass in isolation, including edge
  cases (zero-length array, exact-boundary offsets, negative offsets,
  untracked base pointers).
- **Comparison to existing tools:** unlike AddressSanitizer's shadow-memory
  approach, this project uses a single allocation-metadata hash table — less
  precise (page/sub-object granularity is not modelled) but small enough to
  explain and inspect end-to-end in a review, which is the project's stated
  contribution (see `A29-Compiler_Design_Project.docx` §5, Research Gap and
  Project Positioning).
- **Performance:** not yet measured (Milestone 5, Review 3 prep).

## 14. References and Appendix

- Review 1 submission: `A29-Compiler_Design_Project.docx`.
- Design rationale: `docs/design_notes.md`.
- Architecture diagram and module interfaces: `docs/architecture.md`.
- Full test plan and corpus: `docs/test_plan.md`.
- Defect log: `docs/defect_log.md`.
- Risk log: `docs/risk_log.md`.
- Progress tracker: `docs/progress_tracker.md`.
- Contribution matrix: `docs/contribution_matrix.md`.
- Revised timeline: `docs/review2_timeline.md`.
- Tools referenced for background/comparison: AddressSanitizer, Valgrind
  Memcheck, Purify (see Review 1 submission for the full survey table).

---

*Placeholders to fill before submission: course code, team number, register
numbers, review date, faculty name (cover table), and any actual written
Review 1 evaluator feedback (§1 / `docs/review1_corrections.md`).*
