# Review 1 Feedback and Compliance

No written evaluator feedback sheet for Review 1 has been transcribed into
this repository yet — **if the faculty evaluator returned specific marked
comments, replace the "Observation" column below with those verbatim before
submission.** In its absence, the observations below are the team's own
self-assessment against the Review 1 rubric (design/planning/feasibility),
identifying exactly what Review 1 left as a design/paper artifact versus what
Review 2 required to be a working, tested implementation.

| # | Observation (Review 1 state) | Action taken | Owner | Evidence | Closure status |
|---|---|---|---|---|---|
| 1 | Initial progress was a design document and a benchmark dataset; no compiling code | Built the full pipeline: LLVM pass, runtime library, `memsafec` driver, CMake build | Prajwal (integration) + team | commits `44a0c11`, `dd088d5`; `./build.sh` | **Closed** |
| 2 | Runtime metadata table (`base_ptr → {size, freed, alloc_site}`) existed only as a slide/pseudocode description | Implemented as an open-addressing hash table with growth, plus all five check functions | Yohann | `runtime/memsafety_runtime.{c,h}` | **Closed** |
| 3 | Detection scope (array bounds, null deref, heap safety) was declared but unverified | 12/12 benchmarks now produce the expected instrumented behaviour; 16/16 runtime unit tests pass | Vikas (pass) + Yohann (runtime) + Prajwal (harness) | `benchmarks/run_all.sh`, `tests/test_runtime.c` | **Closed** |
| 4 | Heap pointer-arithmetic bounds checking was flagged as a known gap in the design, with no resolution plan | Implemented `heap_boundscheck()` — a dynamic table lookup against the recorded allocation size (see `docs/design_notes.md`) | Vikas + Yohann | commit `dd088d5`; `benchmarks/unsafe/09_pointer_arithmetic.c`, `11_heap_index_oob.c` | **Closed** |
| 5 | No reproducible/automated build or CI shown at Review 1 | Added `CMakeLists.txt` (+ per-directory), `build.sh` one-command build, `CMakePresets.json`, `.github/workflows/ci.yml` | Prajwal | `build.sh`, `.github/workflows/ci.yml` | **Closed** (CI written; not yet validated against a live GitHub remote — see `docs/review2_timeline.md`) |
| 6 | Overhead-measurement objective stated in scope, no harness existed | `scripts/measure_overhead.sh` scaffolded (Milestone 5) | Prajwal + Yohann | `scripts/measure_overhead.sh` | **Open** — full timing table is a Review 3 deliverable |
| 7 | Responsibility matrix existed on paper; repository did not yet show member-wise technical evidence | Module ownership formalised in the repo's conventions file and demonstrated per-module in `docs/contribution_matrix.md`; each member must open/explain/demo their own files at Review 2 viva | All | `docs/contribution_matrix.md` | **Open / partially addressed** — see note on git authorship in `docs/contribution_matrix.md` |

## Changes in scope, tools or architecture since Review 1

None of the Review 1 "locked-in" architecture decisions changed:
LLVM New Pass Manager plugin, hash-table metadata store (not shadow memory),
GEP+load/store / malloc-calloc-free instrumentation, `memsafec` as the
compiler-driver, plain-C runtime. The **only extension** is `heap_boundscheck`
(item 4 above) — it was anticipated as a documented limitation at Review 1 and
is implemented as an addition to the existing metadata table, not an
architectural change.
