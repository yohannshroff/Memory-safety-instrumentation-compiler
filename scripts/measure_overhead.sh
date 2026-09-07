#!/usr/bin/env bash
#
# measure_overhead.sh - instrumented vs. uninstrumented run-time comparison.
#
# PLACEHOLDER (Milestone 5). The full version will:
#   * compile each program in benchmarks/overhead/ with plain clang -O0 and via
#     memsafec, time N runs of each, and write a markdown table to
#     docs/overhead_results.md (program, baseline_ms, instrumented_ms, overhead_%).
#
# benchmarks/overhead/ is intentionally empty until then. This stub exists so
# the documented repo layout stays intact and the script path is stable.
#
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
overhead_dir="$here/../benchmarks/overhead"

shopt -s nullglob
progs=("$overhead_dir"/*.c)
if [ ${#progs[@]} -eq 0 ]; then
  echo "measure_overhead.sh: no programs in benchmarks/overhead/ yet (Milestone 5)." >&2
  exit 0
fi

echo "measure_overhead.sh: full timing harness not implemented yet (Milestone 5)." >&2
exit 0
