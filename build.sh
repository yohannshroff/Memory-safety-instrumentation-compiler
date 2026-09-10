#!/usr/bin/env bash
#
# build.sh - one-command configure + build + test.
#
#   ./build.sh              configure (if needed), build, run unit tests
#   ./build.sh --clean      wipe build/ first
#   ./build.sh --bench      also run the end-to-end benchmark suite
#
# Finds LLVM automatically: Homebrew's keg-only llvm on macOS, otherwise
# whatever `llvm-config` / CMake find on PATH (Linux, CI). Override with
#   LLVM_DIR=/path/to/lib/cmake/llvm ./build.sh
#
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$here"

clean=0
bench=0
for arg in "$@"; do
  case "$arg" in
    --clean) clean=1 ;;
    --bench) bench=1 ;;
    -h|--help) sed -n '2,14p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "build.sh: unknown option: $arg" >&2; exit 2 ;;
  esac
done

[ "$clean" -eq 1 ] && rm -rf build

# --- locate LLVM ---------------------------------------------------------
cmake_args=()
if [ -n "${LLVM_DIR:-}" ]; then
  cmake_args+=("-DLLVM_DIR=$LLVM_DIR")
  echo "build.sh: using LLVM_DIR=$LLVM_DIR (from environment)"
elif command -v brew >/dev/null 2>&1 && brew --prefix llvm >/dev/null 2>&1; then
  brew_llvm="$(brew --prefix llvm)"
  cmake_args+=(
    "-DLLVM_DIR=$brew_llvm/lib/cmake/llvm"
    "-DCMAKE_C_COMPILER=$brew_llvm/bin/clang"
    "-DCMAKE_CXX_COMPILER=$brew_llvm/bin/clang++"
  )
  echo "build.sh: using Homebrew LLVM at $brew_llvm"
else
  echo "build.sh: relying on LLVM found via PATH / default CMake search"
fi

# --- configure + build + test -----------------------------------------
cmake -B build "${cmake_args[@]}"
cmake --build build
ctest --test-dir build --output-on-failure

if [ "$bench" -eq 1 ]; then
  echo
  ./benchmarks/run_all.sh
fi

echo
echo "build.sh: done. Try:"
echo "  ./driver/memsafec benchmarks/unsafe/02_oob_read.c -o /tmp/oob && /tmp/oob"
