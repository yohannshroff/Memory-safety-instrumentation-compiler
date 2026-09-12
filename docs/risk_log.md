# Risk Log

Owner: Prajwal (Member 4). Reviewed at each weekly sync.

| # | Risk | Likelihood | Impact | Mitigation | Status |
|---|---|---|---|---|---|
| R1 | LLVM pass API churn between versions (headers move, signatures change) | Med | Med | Target the stable New-PM plugin API only; `__has_include` guard for relocated headers; pin the CI LLVM version | Open — hit once (`PassPlugin.h` moved to `llvm/Plugins/` in LLVM 20+), handled |
| R2 | Instrumented IR fails to verify / crashes `opt` | Med | High | Collect work lists first, then mutate; build against `opt`'s verifier; keep `IRBuilder` insertion points explicit | Mitigated — full corpus passes the verifier |
| R3 | Scope creep into full memory safety (heap bounds, realloc, threads) | High | Med | Limitations fixed in README; heap bounds resolved via a dynamic table lookup rather than a static provenance analysis (kept in scope); realloc and thread-safety remain explicitly out | Closed for heap bounds (Review 2) — realloc/threads remain open by design, not by slippage |
| R4 | Metadata hash-table bugs (probing, growth) eat time | Low | Med | Unit tests cover growth past capacity and every check path; linear-array fallback is acceptable per the plan | Closed |
| R5 | Toolchain differences across members' machines (Linux vs macOS, LLVM version) | Med | Low | `memsafec` auto-locates Homebrew LLVM; CMake takes `-DLLVM_DIR`; CI is the reference environment; `build.sh` + CMake presets added Review 2 | Mitigated |
| R6 | Overhead measurement left to the last minute (graded objective) | Med | Med | Milestone 5 has its own script + `docs/overhead_results.md`; violation-free benchmarks reserved in `benchmarks/overhead/` | Open — scheduled for Review 3 prep |
| R7 | Uneven contribution / integration stalls before Review 2 | Med | High | Module ownership matches Review 1 slide ownership (see `README.md` repository layout); `memsafec` + `run_all.sh` are the integration contract; weekly sync | Open — tracked |
| R8 | Heap-offset check gives false confidence (silently skips pointers it can't resolve to a tracked base) | Med | Med | Documented explicitly in README/design notes as a scope boundary, not hidden; `11_heap_index_oob` and `09_pointer_arithmetic` both exercise the path it *does* cover | Open — documented, acceptable for prototype scope |
