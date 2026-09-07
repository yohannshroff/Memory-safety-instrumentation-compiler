# Test Plan

## Levels

| Level | What | How |
|---|---|---|
| Unit | `runtime/` metadata table + check functions in isolation | `tests/test_runtime.c` via `ctest`; violation paths run in a forked child and must abort with the right `[VIOLATION]` line |
| Integration | pass + runtime through the full `clang → opt → clang` pipeline | `driver/memsafec` on single files |
| System | whole benchmark corpus, expected pass/fail | `benchmarks/run_all.sh` |
| Performance | instrumented vs. uninstrumented run time | `scripts/measure_overhead.sh` (Milestone 5) |

## Benchmark corpus

| Benchmark | Class | Expected instrumented behaviour |
|---|---|---|
| `safe/01_safe_array` | positive | exits 0, no violation |
| `safe/08_heap_in_bounds` | positive | exits 0, no violation |
| `unsafe/02_oob_read` | negative | `out-of-bounds access`, non-zero exit |
| `unsafe/03_oob_write` | negative | `out-of-bounds access`, non-zero exit |
| `unsafe/04_null_deref` | negative | `null dereference`, non-zero exit |
| `unsafe/05_use_after_free` | negative | `use after free`, non-zero exit |
| `unsafe/06_double_free` | negative | `double free`, non-zero exit |
| `unsafe/07_invalid_free` | negative | `invalid free`, non-zero exit |
| `unsafe/10_mixed` | negative | `out-of-bounds access` (stack read), valid ops unflagged |
| `unsafe/09_pointer_arithmetic` | **XFAIL** | heap pointer-arithmetic OOB — not yet tracked (documented limitation) |

## Edge cases exercised by `tests/test_runtime.c`

- `boundscheck` at `index == len` and at negative index
- `free(NULL)` / `heap_register(NULL, …)` are no-ops
- metadata table growth past its initial capacity (128 live allocations)
- register → use → release happy path does not trip any check

## Acceptance criteria

- `ctest` reports 0 failures.
- `benchmarks/run_all.sh` prints `0 fail` and exits 0 (XFAIL rows allowed).
- A `safe/` program produces byte-identical stdout with and without
  `--no-instrument`.

## Adding a benchmark

1. Drop the `.c` file in `benchmarks/safe/` or `benchmarks/unsafe/`.
2. Add a row to `expect_for()` in `benchmarks/run_all.sh` (`SAFE`, `UNSAFE|<substring>`, or `XFAIL|<note>`).
3. An unlisted benchmark makes the harness fail on purpose.
