Yohann Benchmark Dataset
=========================

These small C99/C11-compatible programs are intended for Review 1 feasibility testing.

Build example:
  clang -std=c11 -Wall -Wextra 02_oob_read.c -o oob_read

Suggested evaluation:
1. Run the baseline executable.
2. Run the instrumented executable.
3. Record whether the expected violation is reported.
4. Record execution time for safe and unsafe cases.
5. Optionally cross-check with AddressSanitizer or Valgrind.

Note:
Some undefined-behaviour cases (especially invalid free) may be diagnosed by the
baseline compiler/runtime or behave differently across environments. The project
should evaluate the instrumented output rather than rely on a particular crash.
