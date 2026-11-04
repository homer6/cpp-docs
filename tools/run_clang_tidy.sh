#!/usr/bin/env bash
# Run clang-tidy with the repo's profile (.clang-tidy, tests/.clang-tidy) over the
# security tests and the fuzz targets' headers they include (#308). Every finding
# is an error: fix it, or keep it with "// NOLINT(<check>): <why>".
#   tools/run_clang_tidy.sh [build-dir] [files...]
# build-dir must hold compile_commands.json (configured with
# -DCMAKE_EXPORT_COMPILE_COMMANDS=ON; the presets do). CLANG_TIDY picks the binary.
set -uo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root" || exit 2
build="${1:-build/clang-22}"
shift || true
if [[ ! -f "$build/compile_commands.json" ]]; then
    echo "run_clang_tidy.sh: no $build/compile_commands.json; configure that build first" >&2
    exit 2
fi
tidy="${CLANG_TIDY:-}"
if [[ -z "$tidy" ]]; then
    for c in clang-tidy-22 clang-tidy /opt/homebrew/opt/llvm/bin/clang-tidy; do
        command -v "$c" >/dev/null 2>&1 && { tidy="$c"; break; }
    done
fi
[[ -n "$tidy" ]] || { echo "run_clang_tidy.sh: clang-tidy not found (set CLANG_TIDY)" >&2; exit 2; }
if (( $# )); then files=("$@"); else files=(tests/security/*_test.cpp); fi
echo "== $("$tidy" --version | grep -m1 -i version) on ${#files[@]} files"
status=0
for f in "${files[@]}"; do
    if ! out="$("$tidy" -p "$build" --quiet "$f" 2>&1)"; then status=1; fi
    if grep -qE "(warning|error):" <<<"$out"; then
        grep -E "(warning|error):" <<<"$out" | sed "s#^$root/##"
        status=1
    fi
done
(( status == 0 )) && echo "run_clang_tidy.sh: no findings"
exit $status
