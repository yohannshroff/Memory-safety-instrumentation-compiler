# Risk Log

Owner: Prajwal (Member 4). Reviewed at each weekly sync.

| # | Risk | Likelihood | Impact | Mitigation | Status |
|---|---|---|---|---|---|
| R1 | LLVM pass API churn between versions (headers move, signatures change) | Med | Med | Target the stable New-PM plugin API only; `__has_include` guard for relocated headers; pin the CI LLVM version | Open — hit once (`PassPlugin.h` moved to `llvm/Plugins/` in LLVM 20+), handled |
| R2 | Instrumented IR fails to verify / crashes `opt` | Med | High | Collect work lists first, then mutate; build against `opt`'s verifier; keep `IRBuilder` insertion points explicit | Mitigated — full corpus passes the verifier |
| R3 | Scope creep into full memory safety (heap bounds, realloc, threads) | High | Med | Limitations fixed in README and enforced by the `XFAIL` row in `run_all.sh`; changes need a plan update | Open — controlled |
| R4 | Metadata hash-table bugs (probing, growth) eat time | Low | Med | Unit tests cover growth past capacity and every check path; linear-array fallback is acceptable per the plan | Closed |
| R5 | Toolchain differences across members' machines (Linux vs macOS, LLVM version) | Med | Low | `memsafec` auto-locates Homebrew LLVM; CMake takes `-DLLVM_DIR`; CI is the reference environment | Mitigated |
| R6 | Overhead measurement left to the last minute (graded objective) | Med | Med | Milestone 5 has its own script + `docs/overhead_results.md`; violation-free benchmarks reserved in `benchmarks/overhead/` | Open — scheduled Week 10 |
| R7 | Uneven contribution / integration stalls before Review 2 | Med | High | Module ownership matches Review 1 slide ownership; `memsafec` + `run_all.sh` are the integration contract; weekly sync | Open — tracked |
