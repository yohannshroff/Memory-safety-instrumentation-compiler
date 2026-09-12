---
title: Memory-Safety Instrumentation Compiler
subtitle: "Review 2 — Implementation, Integration and Progress | Team A29"
author: "Dhruv · Yohann · Vikas · Prajwal — [COURSE CODE], VIT Vellore"
date: "[REVIEW DATE]"
---

# Review 1 Recap & Corrections
**Presenter: Dhruv**

- Review 1 was design/planning: architecture locked in, benchmark corpus written, no working code yet
- 5 of 7 self-assessed gaps closed since then: working pipeline, tested runtime, verified detection scope, heap-bounds gap closed, reproducible build + CI
- 1 open by plan: overhead measurement (scheduled, not graded-late)
- 1 partially addressed: per-member git-commit diversity (see Slide 12)
- No architecture decision from Review 1 was reversed

# Updated Objectives & Scope
**Presenter: Dhruv**

- Detect: stack array OOB · heap OOB (subscript + pointer arithmetic) · null dereference · use-after-free · double-free · invalid-free
- Provide a single-command "compiler" (`memsafec`) for live demo
- Measure instrumented vs. baseline overhead — **not yet done** (next milestone)
- Scope unchanged: C on Clang/LLVM, single-threaded; `realloc` and static in-bounds elision explicitly out of scope

# System Architecture
**Presenter: Dhruv**

```
C source --clang(-emit-llvm)--> IR
        --opt(-passes=memsafety)--> Instrumented IR
        --clang(link runtime)--> Executable
        --run--> clean exit 0  OR  [VIOLATION] ... + abort()
```

- `src/` — LLVM New Pass Manager plugin (IR scan + check injection)
- `runtime/` — dependency-free C library (allocation metadata table + checks)
- `driver/memsafec` — the integrated "compiler" for demos

# Planned vs. Completed
**Presenter: Dhruv** — ~85% of the planned M0–M6 scope complete

| Milestone | Status |
|---|---|
| M0 Repo + no-op pass | Done |
| M1 Runtime + unit tests | Done |
| M2 Array bounds + null-deref | Done |
| M3 Heap tracking | Done |
| M3.5 Heap pointer-arithmetic bounds *(new)* | Done |
| M4 Driver + benchmark harness | Done |
| M5 Overhead measurement | Not started |
| M6 CI + doc polish | Partial |

# Module 1 — Runtime Library (Design)
**Presenter: Yohann**

- `runtime/memsafety_runtime.c` — plain C, zero LLVM dependency
- One hash table: `base_ptr -> {size, freed, alloc_site}` (open addressing, grows past 70% load)
- Five functions: `boundscheck`, `heap_boundscheck`, `ptrcheck`, `heap_register`, `heap_release`
- `report_violation` prints `[VIOLATION] <type> at <loc>, addr=<addr>` and `abort()`s — unambiguous pass/fail signal

# Module 1 — Runtime Library (Demo & Tests)
**Presenter: Yohann**

- `ctest` — **16 / 16 unit tests pass**, zero LLVM involved
- Covers: exact-boundary indices, zero-length arrays, negative offsets, table growth past capacity, untracked-pointer no-ops
- Violation paths run in a forked child process so the abort() itself is verified
- Fully demoable standalone: `cmake --build build && ctest --test-dir build`

# Module 2 — LLVM Pass (Design)
**Presenter: Vikas**

- New Pass Manager plugin registered as `memsafety` (`opt -passes=memsafety`)
- `IRScanner` classifies every load/store/call into 4 work lists: array access, heap-pointer access, pointer deref, heap call
- `CheckInjector` splices runtime calls in via `IRBuilder`
- Heap bounds (new): a single-index GEP off an unknown-length base is checked **dynamically** against the runtime's own allocation table — no static provenance analysis needed

# Module 2 — LLVM Pass (Demo & Tests)
**Presenter: Vikas**

Before → after (`*(p + 4) = 9` on a 16-byte heap buffer):

```llvm
%13 = getelementptr inbounds i32, ptr %12, i64 4
+ call void @heap_boundscheck(ptr %12, i64 16, i64 4, ptr @.memsafe.loc)
+ call void @ptrcheck(ptr %13, ptr @.memsafe.loc)
store i32 9, ptr %13, align 4
```

- `opt -load-pass-plugin libMemSafety.so -passes=memsafety input.ll -S` — inspectable live
- Result: `[VIOLATION] out-of-bounds access ...`, exit 134

# Integration & End-to-End Prototype
**Presenter: Dhruv & Prajwal**

```
./driver/memsafec input.c -o out
```
chains `clang -emit-llvm` → `opt -passes=memsafety` → `clang (link runtime)` into one command

- Safe programs: byte-identical stdout with/without instrumentation
- `-v` flag echoes every underlying command; `--no-instrument` builds a baseline
- Plugin/runtime paths auto-located under `build/`, overridable via env vars

# Testing Summary
**Presenter: Prajwal**

**12 / 12 benchmarks pass, 0 fail, 0 xfail**

| Category | Examples |
|---|---|
| Positive | `01_safe_array`, `08_heap_in_bounds` |
| Boundary | `12_boundary_last_index` (exact last valid index, stack + heap) |
| Negative — spatial | `02/03_oob_*`, `09_pointer_arithmetic`, `11_heap_index_oob`, `10_mixed` |
| Negative — temporal | `04_null_deref`, `05_use_after_free`, `06_double_free`, `07_invalid_free` |

# Defects Found & Debugging Evidence
**Presenter: Prajwal**

- 8 real defects logged with root cause + fix (`docs/defect_log.md`)
- Highlights: LLVM header relocation across versions, a hidden-visibility bug that broke plugin loading, a null-`Module*` crash in `IRBuilder`, a bash-3.2 portability bug in the harness
- The core Review-2 fix: heap OOB via pointer arithmetic was undetected (`XFAIL`) until `heap_boundscheck` was added — now closed

# Repository & Contribution Evidence
**Presenter: Prajwal**

- `src/` (Vikas) · `runtime/` (Yohann) · `driver/`, `tests/`, CI (Prajwal) · `docs/`, README (Dhruv)
- One-command build: `./build.sh` (auto-detects LLVM); CI green on every push
- **Known gap:** commits currently share one git identity — module ownership enforced structurally (`docs/contribution_matrix.md`), not yet by git-author diversity; each member must be able to open/explain/demo their own column live

# Risks, Pending Work & Review 3 Plan
**Presenter: Dhruv**

- **Open:** overhead measurement (Week 10), CI validation on push (done), per-member commit evidence (Week 10–11)
- **Stretch goal:** multi-index heap GEPs (`p[i][j]`), static in-bounds elision
- Full week-by-week plan: `docs/review2_timeline.md`; full risk log: `docs/risk_log.md` (8 tracked risks)

# Expected Final Results
**All Members**

- Full detection coverage across 6 violation classes, verified end-to-end
- Runtime overhead measured and reported (Review 3 target)
- Final report + polished CI + independently-rehearsed member demos

# Conclusion
**All Members**

- Working, tested, integrated prototype — not a paper design
- Transparent compiler-pass workflow: every transformation is inspectable via `opt -S`
- On track for Review 3: remaining work is measurement + presentation polish, not core implementation
