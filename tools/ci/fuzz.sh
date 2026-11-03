#!/usr/bin/env bash
# Run each libFuzzer target in build/fuzz/fuzz/ for a time box on a copy of its
# saved corpus (fuzz/corpus/<target>/), #307. A crash fails the script; the
# crashing input lands in fuzz-out/ (uploaded by the CI job) to be minimized
# and saved in fuzz/corpus/<target>/ as a regression input.
#   tools/ci/fuzz.sh [seconds-per-target] [build-dir]
set -uo pipefail
secs="${1:-60}"
build="${2:-build/fuzz}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$root" || exit 2
out=fuzz-out
rm -rf "$out"; mkdir -p "$out"
status=0
for corpus in fuzz/corpus/*/; do
    t="$(basename "$corpus")"
    bin="$build/fuzz/fuzz_$t"
    [[ -x "$bin" ]] || { echo "fuzz.sh: missing $bin" >&2; status=1; continue; }
    mkdir -p "$out/corpus/$t"
    cp "$corpus"* "$out/corpus/$t/" 2>/dev/null || true
    echo "== fuzz_$t for ${secs}s"
    if "$bin" -max_total_time="$secs" -max_len=4096 -print_final_stats=1 \
             -artifact_prefix="$out/$t-" "$out/corpus/$t" 2>&1 | tee "$out/$t.log" | tail -n 25; then
        :
    fi
    if ls "$out/$t-"* >/dev/null 2>&1; then
        echo "fuzz.sh: fuzz_$t found a failing input:"; ls -l "$out/$t-"*
        status=1
    fi
done
exit $status
