# Revised Timeline — Review 2 → Review 3

Review 1 was Week 4; this Review 2 submission lands in Week 9. Remaining
weeks before Review 3 (Week 12):

| Week | Task | Owner | Depends on |
|---|---|---|---|
| 9 (now) | Review 2 submission: implementation complete for M0–M4 + M3.5; docs, defect log, contribution matrix produced | All | — |
| 10 | Implement `scripts/measure_overhead.sh` for real: time instrumented vs. baseline over `benchmarks/overhead/`; write `docs/overhead_results.md` (Milestone 5) | Prajwal + Yohann | benchmark content from Yohann |
| 10 | Push repository to a GitHub remote; confirm `.github/workflows/ci.yml` actually runs and passes | Prajwal | — |
| 11 | Each member independently re-derives and re-demos their module (dry-run viva); address any Review 2 evaluator corrections | All | Review 2 evaluator feedback |
| 11 | Stretch goal (time permitting): multi-index GEP support in `IRScanner`/`CheckInjector` (e.g. `p[i][j]`), static in-bounds elision for compile-time-provable accesses | Vikas | — |
| 12 | Final report compilation; presentation slides; Review 3 rehearsal | Dhruv (compile) + all (sections) | all preceding rows |

## Carried-over risks (see `docs/risk_log.md` for the full log)

- **R6** — overhead measurement is the one milestone with zero progress
  beyond a stub; scheduled first in Week 10 specifically because it's graded
  and easy to leave too late.
- **R7** — git history doesn't yet show per-member commit diversity;
  addressed by having each member commit their own Week 10–11 work under
  their own account, and by `docs/contribution_matrix.md`'s explicit
  per-module breakdown in the interim.
- **R8** — `heap_boundscheck`'s scope boundary (exact-match base pointers
  only) stays documented rather than silently expanded; any attempt at
  broader pointer-provenance tracking is a stretch goal, not a commitment.
