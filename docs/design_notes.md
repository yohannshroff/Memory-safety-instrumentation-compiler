# Design Notes & Rationale

## Why an LLVM IR pass, not source-to-source

- The New Pass Manager plugin API (`opt -load-pass-plugin`) is small, stable and
  well documented — no need to hand-roll a C parser or fight a full front end.
- Instrumenting at IR level means the same pass works for anything Clang can
  lower, and the transformation is inspectable (`opt -S`) for the review demo.
- Source rewriting would entangle us with the C preprocessor, macros and
  formatting; none of that is the point of the project.

## Why a hash table, not shadow memory

The metadata store is a single `base_ptr → {size, freed, alloc_site}` hash
table. Shadow memory (a byte-per-byte or bit-per-byte map of the address space,
as AddressSanitizer uses) is powerful but:

- needs allocator interception and a large reserved mapping,
- is much harder to explain and debug in a 12-week project,
- buys precision we do not need for a bounded benchmark corpus.

The hash table is O(1) per lookup at benchmark scale and fits in one C file.

## Why `abort()` on the first violation

`report_violation` prints the diagnostic and calls `abort()` (exit 134):

- the unsafe access never executes,
- the benchmark harness gets an unambiguous non-zero exit + a stderr marker,
- there is no question of cascading errors after the first real bug.

`exit(1)` was the alternative; `abort()` was chosen for the clearer signal.

## What is deliberately out of scope

| Item | Why | Where noted |
|---|---|---|
| `realloc` tracking | needs an *update* to a metadata entry, not a clean add/remove; ownership/aliasing corner cases | README, `CheckInjector::injectHeapCall` |
| Static in-bounds elision | optimisation, not correctness; everything is instrumented | README |
| Heap pointer-arithmetic bounds | requires threading allocation size into every GEP check; planned as a follow-up milestone | README, `run_all.sh` XFAIL row |
| Thread safety | single-threaded benchmark programs only | README |

## Runtime function signatures (pass ↔ runtime contract)

```
void boundscheck(void *base, size_t elem_size, size_t len, long index, const char *loc);
void ptrcheck  (void *ptr, const char *loc);
void heap_register(void *ptr, size_t size, const char *loc);
void heap_release (void *ptr, const char *loc);
void report_violation(const char *type, const char *loc, const void *addr);
```

The pass declares these with `Module::getOrInsertFunction` using matching LLVM
types (`ptr`, `i64`); the runtime provides the definitions. The 5-argument
`boundscheck` form (with `elem_size`) is canonical — an earlier draft of the
implementation plan showed a 4-argument variant; `elem_size` is kept for
byte-accurate diagnostics later.
