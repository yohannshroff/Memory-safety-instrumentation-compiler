#!/usr/bin/env bash
#
# run_all.sh - build every benchmark with `memsafec` and check its instrumented
# behaviour against the expected result table below.
#
#   benchmarks/safe/*    : must exit 0 and print no "[VIOLATION]" line
#   benchmarks/unsafe/*  : must exit non-zero and print "[VIOLATION] <type>"
#
# XFAIL entries are known limitations (documented in README.md / docs/): they
# are reported but do not fail the suite.
#
# Every .c file under benchmarks/safe or benchmarks/unsafe must be listed in
# expect_for() or the script fails ("unlisted benchmark").
#
# Written for POSIX-ish bash 3.2+ (no associative arrays) so it runs on a
# stock macOS shell as well as CI.
#
set -u

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/.." && pwd)"
memsafec="$repo/driver/memsafec"

# name -> "KIND|detail"
#   SAFE   : must exit 0, no violation           (detail unused)
#   UNSAFE : must exit != 0, [VIOLATION] line contains <detail>
#   XFAIL  : known miss - <detail> is a note, not enforced
expect_for() {
  case "$1" in
    01_safe_array)        echo "SAFE|" ;;
    08_heap_in_bounds)    echo "SAFE|" ;;
    02_oob_read)          echo "UNSAFE|out-of-bounds access" ;;
    03_oob_write)         echo "UNSAFE|out-of-bounds access" ;;
    04_null_deref)        echo "UNSAFE|null dereference" ;;
    05_use_after_free)    echo "UNSAFE|use after free" ;;
    06_double_free)       echo "UNSAFE|double free" ;;
    07_invalid_free)      echo "UNSAFE|invalid free" ;;
    10_mixed)             echo "UNSAFE|out-of-bounds access" ;;
    09_pointer_arithmetic) echo "XFAIL|heap pointer-arithmetic bounds not yet tracked" ;;
    *)                    echo "" ;;
  esac
}

pass=0; fail=0; xfail=0
printf '%-26s %-8s %-8s %s\n' "BENCHMARK" "EXPECT" "RESULT" "DETAIL"
printf '%s\n' "----------------------------------------------------------------------------"

run_one() {
  file="$1"
  name="$(basename "${file%.c}")"
  row="$(expect_for "$name")"
  if [ -z "$row" ]; then
    printf '%-26s %-8s %-8s %s\n' "$name" "?" "ERROR" "unlisted benchmark - add it to expect_for()"
    fail=$((fail + 1)); return
  fi
  kind="${row%%|*}"; want="${row#*|}"

  bin="$(mktemp)"; cerr="$(mktemp)"; rerr="$(mktemp)"

  if ! "$memsafec" "$file" -o "$bin" >"$cerr" 2>&1; then
    printf '%-26s %-8s %-8s %s\n' "$name" "$kind" "BUILDERR" "$(tail -1 "$cerr" | cut -c1-46)"
    fail=$((fail + 1)); rm -f "$bin" "$cerr" "$rerr"; return
  fi

  # The instrumented binary aborts on a violation (by design), which makes the
  # shell print an "Abort trap: 6" / "Aborted" job notice. Silence just that
  # line by redirecting this group's stderr; the program's own stderr is
  # already captured in $rerr.
  rc=0
  { "$bin" >/dev/null 2>"$rerr"; } 2>/dev/null || rc=$?
  vline="$(grep -m1 '\[VIOLATION\]' "$rerr" 2>/dev/null || true)"
  rm -f "$bin" "$cerr" "$rerr"

  case "$kind" in
    SAFE)
      if [ "$rc" -eq 0 ] && [ -z "$vline" ]; then
        printf '%-26s %-8s %-8s %s\n' "$name" "clean" "PASS" "exit 0, no violation"
        pass=$((pass + 1))
      else
        printf '%-26s %-8s %-8s %s\n' "$name" "clean" "FAIL" "rc=$rc ${vline:+($vline)}"
        fail=$((fail + 1))
      fi
      ;;
    UNSAFE)
      if [ "$rc" -ne 0 ] && printf '%s' "$vline" | grep -qF "$want"; then
        printf '%-26s %-8s %-8s %s\n' "$name" "detect" "PASS" "$vline"
        pass=$((pass + 1))
      else
        printf '%-26s %-8s %-8s %s\n' "$name" "detect" "FAIL" "rc=$rc want='$want' got='${vline:-none}'"
        fail=$((fail + 1))
      fi
      ;;
    XFAIL)
      if [ "$rc" -ne 0 ] && [ -n "$vline" ]; then
        printf '%-26s %-8s %-8s %s\n' "$name" "xfail" "XPASS" "now detected: $vline"
      else
        printf '%-26s %-8s %-8s %s\n' "$name" "xfail" "XFAIL" "$want"
      fi
      xfail=$((xfail + 1))
      ;;
  esac
}

shopt -s nullglob
for f in "$repo"/benchmarks/safe/*.c "$repo"/benchmarks/unsafe/*.c; do
  run_one "$f"
done

printf '%s\n' "----------------------------------------------------------------------------"
printf 'total: %d pass, %d fail, %d xfail\n' "$pass" "$fail" "$xfail"
[ "$fail" -eq 0 ]
