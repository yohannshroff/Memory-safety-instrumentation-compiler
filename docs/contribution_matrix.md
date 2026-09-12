# Member-Wise Contribution Record

**Note on git authorship:** every commit in this repository currently lands
under one shared git identity, made during focused integration sessions
building the scaffold up to Review 2. That is a workflow choice, not a claim
that one person did all the work — module ownership is enforced structurally
(the project ownership table in the repo root's conventions file, and the columns below) rather than by git-author
diversity. **Per the Review 2 guideline's individual-viva requirement, each
member must be able to open their column's files, explain the logic, execute
it, and discuss its tests unassisted.** Going into Review 3, each member
committing their own follow-up changes under their own account would
strengthen this specific piece of evidence.

| Member | Assigned Technical Module | Work Completed for Review 2 | Evidence / Commit / Demo | Pending Work for Review 3 | Integration Dependency |
|---|---|---|---|---|---|
| **Dhruv** — Project Lead / Problem Analyst | Project narrative, README intro, problem statement & objectives, acceptance criteria per benchmark | Drafted `README.md` framing, `docs/review2_progress.md` narrative sections (abstract, objectives/scope, results) | `README.md`, `docs/review2_progress.md` §§2, 13 | Finalise report language for submission; prepare Slides 1–4 (Review 2 structure) | Needs the final feature set (Yohann/Vikas) and test results (Prajwal) frozen before writing §13 |
| **Yohann** — Background/Requirements Analyst | `runtime/` — metadata table, `boundscheck`, `heap_boundscheck`, `ptrcheck`, `heap_register`, `heap_release`, `report_violation` | Full runtime implementation + `heap_boundscheck` extension; 16 unit tests in `tests/test_runtime.c`, all passing; benchmark content review (`benchmarks/`) | `runtime/memsafety_runtime.{c,h}`, `tests/test_runtime.c`; commits `44a0c11`, `dd088d5` | Populate `benchmarks/overhead/` with violation-free timing programs (Milestone 5 input) | `src/CheckInjector.cpp` must call these exact signatures — currently in sync; any signature change needs to be coordinated with Vikas |
| **Vikas** — System Designer / Core Algorithm Owner | `src/` — `IRScanner` (classify GEP/load/store/heap calls), `CheckInjector` (insert calls via `IRBuilder`), `MemSafety` (New-PM plugin registration) | Array-bounds, pointer-deref and heap-call instrumentation; extended `IRScanner`/`CheckInjector` with the `HeapPtrAccess` work list and `heap_boundscheck` injection to close the pointer-arithmetic gap | `src/IRScanner.{h,cpp}`, `src/CheckInjector.{h,cpp}`, `src/MemSafety.cpp`; commits `44a0c11`, `dd088d5` | Consider multi-index GEPs (`p[i][j]`-style) and static in-bounds elision as stretch goals; keep pass in sync with any new runtime function | Depends on `runtime/` function signatures (Yohann) staying stable; `MEMSAFE_VERBOSE` counters used to sanity-check instrumentation counts during development |
| **Prajwal** — Implementation, Testing & Planning Coordinator | `driver/memsafec`, `tests/` harness wiring, `benchmarks/run_all.sh`, `CMakeLists.txt`, `build.sh`, `CMakePresets.json`, `.github/workflows/ci.yml`, `docs/risk_log.md` | Built the `memsafec` driver, the 12-case benchmark corpus + expected-result harness (bash-3.2-portable), the one-command `build.sh`, CMake presets, and the CI workflow; maintains the risk log and defect log | `driver/memsafec`, `benchmarks/run_all.sh`, `build.sh`, `.github/workflows/ci.yml`, `docs/risk_log.md`, `docs/defect_log.md`; commits `44a0c11`, `1e63507`, `dd088d5` | Implement `scripts/measure_overhead.sh` for real (Milestone 5); push to a GitHub remote and confirm CI actually runs; compile the Review 2/3 document sections on testing & repository | Harness (`run_all.sh`) must be updated whenever a new benchmark or violation type is added (owns that convention per the project's repository conventions) |

## Summary for the viva

- **Two working core modules, demonstrable independently:** `runtime/` runs
  and tests entirely without LLVM (`ctest`); `src/` is inspectable via
  `opt -S` on any benchmark to show the before/after IR diff.
- **Integration is the driver + harness:** `driver/memsafec` chains the two
  modules into one command; `benchmarks/run_all.sh` is the reproducible
  evidence that they work together (12/12 pass).
