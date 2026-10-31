#!/usr/bin/env bash
# Configure, build and run the GoogleTest suite with one CMake workflow preset,
# then list every skipped test with its reason.
#
#   tools/run_tests.sh              # gcc-15 (default)
#   tools/run_tests.sh clang-22     # Homebrew Clang 22 + libc++
#   tools/run_tests.sh gcc-16       # GCC 16 inside the gcc:16 Docker image
#
# Extra arguments after the preset are passed to ctest on a rerun, e.g.
#   tools/run_tests.sh gcc-15 -L containers
set -euo pipefail

preset="${1:-gcc-15}"
shift || true
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

if [[ "$preset" == "gcc-16" && ! -f /.dockerenv ]]; then
    # The gcc:16 image has g++ 16 and git but no cmake/ninja; install them
    # from Debian (trixie: cmake 3.31) and rerun this script inside.
    exec docker run --rm -v "$root":/w -w /w gcc:16 bash -c \
        'apt-get update -qq && apt-get install -y -qq --no-install-recommends cmake ninja-build >/dev/null && tools/run_tests.sh gcc-16 "$@"' \
        bash "$@"
fi

status=0
if [[ $# -gt 0 ]]; then
    cmake --preset "$preset"
    cmake --build --preset "$preset"
    ctest --preset "$preset" "$@" || status=$?
else
    cmake --workflow --preset "$preset" || status=$?
fi

# ctest only says "(Skipped)"; the reason is in the test's output in the log.
log="build/$preset/Testing/Temporary/LastTest.log"
if [[ -f "$log" ]] && grep -q ': Skipped$' "$log"; then
    echo
    echo "Skipped tests and why:"
    awk '/^[0-9]+\/[0-9]+ Test: /{name=$3}
         /: Skipped$/{getline reason; printf "  %-50s %s\n", name, reason}' "$log" | sort
fi
exit "$status"
