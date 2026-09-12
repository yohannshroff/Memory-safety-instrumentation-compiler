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

## Heap bounds checking without static allocation sizes (added for Review 2)

Stack/global array bounds checking is easy: the array's length is a compile-time
constant sitting right on the `alloca`/`GlobalVariable`. A heap buffer has no
such constant in the IR — `malloc(n)`'s `n` is a run-time value, and by the time
a heap pointer is subscripted (`p[i]`) or offset (`*(p + k)`), the IR has long
since forgotten which `malloc` call produced it.

Rather than build a static points-to/provenance analysis (a project on its
own), we lean on something we already have at run time: the allocation
metadata table used for `heap_register`/`heap_release`. For any load/store
reached through a single-index GEP that *isn't* a known-length array, the pass
emits `heap_boundscheck(gep_base_pointer, byte_offset, access_size, loc)`. The
runtime looks `gep_base_pointer` up in the same hash table:

- found → validate `byte_offset` is non-negative and `byte_offset + access_size`
  does not exceed the recorded allocation size;
- not found → **silent no-op**. The pass could not prove this base is a heap
  pointer, so the runtime doesn't assert anything about it either way (no new
  false positives on ordinary pointer parameters, struct fields, etc.).

This catches `p[BIG]` and `*(p + BIG)` whenever `p` itself is the live,
unmodified return value of `malloc`/`calloc` (`benchmarks/unsafe/09_pointer_arithmetic.c`,
`11_heap_index_oob.c`). It does **not** catch cases where the checked pointer
was itself already offset from the true base by an earlier, uninstrumented
step (e.g. a pointer that arrives as a function parameter, or one derived
through a chain of GEPs where only the *last* hop is instrumented) — see the
README's Known Limitations.

## What is deliberately out of scope

| Item | Why | Where noted |
|---|---|---|
| `realloc` tracking | needs an *update* to a metadata entry, not a clean add/remove; ownership/aliasing corner cases | README, `CheckInjector::injectHeapCall` |
| Static in-bounds elision | optimisation, not correctness; everything is instrumented | README |
| General pointer-provenance / aliasing analysis for heap bounds | `heap_boundscheck` only recognises a base pointer that is *exactly* a live `malloc`/`calloc` return value; deeper aliasing is out of scope | README, this file |
| Thread safety | single-threaded benchmark programs only | README |

## Runtime function signatures (pass ↔ runtime contract)

```
void boundscheck(void *base, size_t elem_size, size_t len, long index, const char *loc);
void heap_boundscheck(void *base, long byte_offset, size_t access_size, const char *loc);
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
