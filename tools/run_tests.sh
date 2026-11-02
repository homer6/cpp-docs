#!/usr/bin/env bash
# Configure, build and run the GoogleTest suite with one CMake workflow preset,
# then list every skipped test with its reason.
#
#   tools/run_tests.sh              # gcc-15 (default)
#   tools/run_tests.sh clang-22     # Homebrew Clang 22 + libc++
#   tools/run_tests.sh gcc-16       # GCC 16 inside the gcc:16 Docker image
#
# The generic presets take the compiler from CXX (see README "Running the
# tests"); they run anywhere with bash: Linux, macOS, FreeBSD and Git Bash on
# Windows (for msvc and clang-cl, from a VS Developer shell):
#
#   CXX=g++-14 tools/run_tests.sh gcc
#   CXX=clang++-20 tools/run_tests.sh clang-libcxx
#   tools/run_tests.sh msvc
#
# Extra arguments after the preset are passed to ctest on a rerun, e.g.
#   tools/run_tests.sh gcc-15 -L containers
#
# --json FILE (before the preset) also writes the results as JSON with
# tools/ctest_to_json.py: platform, compiler, counts, failures, skips.
#   tools/run_tests.sh --json build/results-gcc.json gcc
#
# gcc-16 re-runs this script inside Docker unless it's already in a container
# (/.dockerenv or /run/.containerenv) or CPPDOCS_NO_DOCKER=1 is set (a host
# whose g++ is GCC 16).
set -euo pipefail

json=""
if [[ "${1:-}" == "--json" ]]; then
    json="${2:?--json needs a file name}"
    shift 2
fi
preset="${1:-gcc-15}"
shift || true
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

write_json() {
    [[ -n "$json" ]] || return 0
    local py
    py="$(command -v python3 || command -v python || true)"
    if [[ -z "$py" ]]; then
        echo "run_tests.sh: no python3 or python on PATH; can't write $json" >&2
        return 0
    fi
    "$py" tools/ctest_to_json.py --build-dir "build/$preset" --preset "$preset" -o "$json" || true
}

in_container() {
    [[ -f /.dockerenv || -f /run/.containerenv || "${CPPDOCS_NO_DOCKER:-}" == 1 ]]
}

if [[ "$preset" == "gcc-16" ]] && ! in_container; then
    # The gcc:16 image has g++ 16 and git but no cmake/ninja; install them
    # from Debian (trixie: cmake 3.31) and rerun this script inside.
    # Run the image for the host's architecture: under emulation (Rosetta on
    # Apple silicon) huge allocations trap instead of throwing bad_alloc (#130).
    case "$(uname -m)" in arm64|aarch64) platform=linux/arm64 ;; *) platform=linux/amd64 ;; esac
    status=0
    docker run --rm --platform "$platform" -v "$root":/w -w /w gcc:16 bash -c \
        'apt-get update -qq && apt-get install -y -qq --no-install-recommends cmake ninja-build >/dev/null && tools/run_tests.sh gcc-16 "$@"' \
        bash "$@" || status=$?
    write_json  # the build tree is shared with the container
    exit "$status"
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
# The log has CRLF line ends on Windows.
log="build/$preset/Testing/Temporary/LastTest.log"
if [[ -f "$log" ]]; then
    skipped="$(awk '{sub(/\r$/, "")}
         /^[0-9]+\/[0-9]+ Test: /{name=$3}
         /: Skipped$/{getline reason; sub(/\r$/, "", reason); printf "  %-50s %s\n", name, reason}' "$log" | sort)"
    if [[ -n "$skipped" ]]; then
        echo
        echo "Skipped tests and why:"
        printf '%s\n' "$skipped"
    fi
fi
write_json
exit "$status"
