# Implementation Progress Summary (Planned vs. Completed)

Tracked against the milestones in `IMPLEMENTATION_PLAN.md`.

| Milestone | Planned task | Status | % | Owner | Evidence |
|---|---|---|---|---|---|
| M0 | Repo scaffold, CMake, no-op pass plugin | Done | 100% | Prajwal | commit `44a0c11` |
| M1 | Runtime library skeleton + unit tests | Done | 100% | Yohann | `runtime/`, `tests/`; commit `44a0c11` |
| M2 | Array-bounds + null-deref instrumentation | Done | 100% | Vikas | `src/`; commit `44a0c11` |
| M3 | Heap tracking (`malloc`/`calloc`/`free`) | Done | 100% | Vikas + Yohann | commit `44a0c11` |
| M3.5 *(added at Review 2)* | Heap pointer-arithmetic bounds checking | Done | 100% | Vikas + Yohann | commit `dd088d5` |
| M4 | Driver + benchmark harness | Done | 100% | Prajwal | commit `44a0c11`; corpus expanded to 12 cases in `dd088d5` |
| M5 | Overhead measurement | **Not started** (stub only) | 0% | Prajwal + Yohann | `scripts/measure_overhead.sh` |
| M6 | CI + documentation polish | Partial | 70% | Prajwal / Dhruv | `.github/workflows/ci.yml` written, not yet validated on a live remote; README/docs polished |
| Review 2 doc set | Progress tracker, defect log, contribution matrix, revised timeline, consolidated report | Done | 100% | Dhruv + Prajwal | `docs/review2_*.md`, this file |

**Overall: ~6.7 of 8 tracked milestones fully closed (~85%).** The core
detection pipeline (M0–M4 + M3.5) is complete and verified end-to-end;
remaining work is performance measurement (M5) and CI validation against a
live repository (tail of M6).

## Test-evidence snapshot (reproducible via `./build.sh --bench`)

- `ctest`: **16 / 16** runtime unit checks pass.
- `benchmarks/run_all.sh`: **12 / 12** benchmarks resolve to their expected
  outcome, **0 fail, 0 xfail**.
