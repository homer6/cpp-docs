#!/usr/bin/env bash
# One platform-workflow job: configure, build and run the test suite with a
# portable preset, run the snippet checker, then write
# results-<platform>-<compiler>.json and a Markdown summary.
#
# Used by .github/workflows/{linux,macos,windows-gnu,freebsd,qemu}.yml (on the
# runner, inside a container, a FreeBSD VM or MSYS2) and runnable by hand:
#
#   CPPDOCS_PLATFORM=macos-arm64 CPPDOCS_COMPILER=gcc-15 CPPDOCS_PRESET=gcc \
#     CXX=g++-15 tools/ci/run.sh
#
# Settings (environment):
#   CPPDOCS_PLATFORM    platform label for the file name, e.g. linux-x64-ubuntu-24.04 (required)
#   CPPDOCS_COMPILER    compiler label, e.g. gcc-14, clang-20-libcxx            (required)
#   CPPDOCS_PRESET      CMake preset: gcc, clang, clang-libcxx, mingw, ...      (required)
#   CXX                 the compiler (the presets read it; also the snippet --cxx)
#   CPPDOCS_AREAS       ctest label regex; empty = every area. Builds only the
#                       matching area targets and checks only their docs/examples/<area>
#   CPPDOCS_TESTS       true/false (default true)
#   CPPDOCS_SNIPPETS    true/false (default true)
#   CPPDOCS_CMAKE_ARGS  extra configure arguments (word-split), e.g. -DCMAKE_CXX_STDLIB_MODULES_JSON=...
#   CPPDOCS_SNIPPET_TOOLCHAIN  --toolchain for check_snippets.py (default auto)
#   CPPDOCS_SNIPPET_ARGS       extra check_snippets.py arguments (word-split),
#                              e.g. --flag=-stdlib=libc++ --flag=-fexperimental-library
#   CPPDOCS_BASELINE    snippet baseline file; empty = none (every failure counts)
#   CPPDOCS_SNIPPETS_MODE  enforced | report-only; default from cppdocs_ci.py snippet-mode (#159)
#   CPPDOCS_OS_VERSION  OS description for the results (default: from CMake / uname)
#   CPPDOCS_OUT         where logs and partial JSON go (default ci-out)
#
# The results file is written to the repo root; the summary goes to
# $CPPDOCS_OUT/summary.md and, when set, $GITHUB_STEP_SUMMARY. Every step runs
# even if an earlier one fails (except build/test after a failed configure);
# the exit status is non-zero if any step failed.
set -uo pipefail

: "${CPPDOCS_PLATFORM:?set CPPDOCS_PLATFORM}" "${CPPDOCS_COMPILER:?set CPPDOCS_COMPILER}" \
  "${CPPDOCS_PRESET:?set CPPDOCS_PRESET}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$root" || exit 2

preset="$CPPDOCS_PRESET"
areas="${CPPDOCS_AREAS:-}"
run_tests="${CPPDOCS_TESTS:-true}"
run_snippets="${CPPDOCS_SNIPPETS:-true}"
out="${CPPDOCS_OUT:-ci-out}"
py="$(command -v python3 || command -v python)"
results="results-${CPPDOCS_PLATFORM}-${CPPDOCS_COMPILER}.json"
mkdir -p "$out"
rm -f "$out/tests.json" "$out/snippets.json"

steps=()
status=0
step() {  # step NAME CODE: record an exit code
    steps+=(--step "$1=$2")
    [[ "$2" == 0 ]] || status=1
}
group() { [[ -n "${GITHUB_ACTIONS:-}" ]] && echo "::group::$*" || echo "== $*"; }
endgroup() { [[ -n "${GITHUB_ACTIONS:-}" ]] && echo "::endgroup::" || true; }

group "toolchain"
echo "platform=$CPPDOCS_PLATFORM compiler=$CPPDOCS_COMPILER preset=$preset areas='${areas}'"
uname -a || true
"${CXX:-c++}" --version || true
cmake --version | head -1
ninja --version
"$py" --version
endgroup

if [[ "$run_tests" == true ]]; then
    targets=(examples_tests)
    ctest_args=(--output-junit junit.xml)
    if [[ -n "$areas" ]]; then
        mapfile_targets="$("$py" tools/ci/cppdocs_ci.py areas --regex "$areas" --targets)" || mapfile_targets=""
        # shellcheck disable=SC2206  # target names have no spaces
        targets=($mapfile_targets)
        ctest_args+=(-L "$areas")
    fi
    group "configure ($preset)"
    # shellcheck disable=SC2086  # CPPDOCS_CMAKE_ARGS is a word list on purpose
    cmake --preset "$preset" ${CPPDOCS_CMAKE_ARGS:-} 2>&1 | tee "$out/configure.log"
    rc=${PIPESTATUS[0]}
    endgroup
    step configure "$rc"
    if [[ $rc == 0 && ${#targets[@]} -gt 0 ]]; then
        group "build (${targets[*]})"
        cmake --build --preset "$preset" --target "${targets[@]}" 2>&1 | tee "$out/build.log"
        rc=${PIPESTATUS[0]}
        endgroup
        step build "$rc"
        group "test"
        ctest --preset "$preset" "${ctest_args[@]}" 2>&1 | tee "$out/test.log"
        step tests "${PIPESTATUS[0]}"
        endgroup
        "$py" tools/ctest_to_json.py --build-dir "build/$preset" --preset "$preset" -o "$out/tests.json" || true
        cp "build/$preset/Testing/Temporary/LastTest.log" "$out/" 2>/dev/null || true
    elif [[ ${#targets[@]} == 0 ]]; then
        step build 1   # the areas regex matched nothing
    fi
fi

snippets_mode="$("$py" tools/ci/cppdocs_ci.py snippet-mode --compiler "$CPPDOCS_COMPILER" \
    --platform "$CPPDOCS_PLATFORM" --baseline "${CPPDOCS_BASELINE:-}")"
if [[ "$run_snippets" == true ]]; then
    paths=(docs)
    if [[ -n "$areas" ]]; then
        p="$("$py" tools/ci/cppdocs_ci.py areas --regex "$areas" --paths)" || p=""
        # shellcheck disable=SC2206
        paths=($p)
    fi
    args=(--toolchain "${CPPDOCS_SNIPPET_TOOLCHAIN:-auto}" -q --json "$out/snippets.json")
    [[ -n "${CXX:-}" ]] && args+=(--cxx "$CXX")
    [[ -n "${CPPDOCS_BASELINE:-}" ]] && args+=(--baseline "$CPPDOCS_BASELINE")
    if [[ "$snippets_mode" == report-only ]]; then
        args+=(--report-only)
        echo "snippets: report-only for $CPPDOCS_COMPILER on $CPPDOCS_PLATFORM (no reviewed baseline; #159)"
    fi
    if [[ ${#paths[@]} -gt 0 ]]; then
        group "snippets (${paths[*]})"
        # shellcheck disable=SC2086
        "$py" tools/check_snippets.py "${paths[@]}" "${args[@]}" ${CPPDOCS_SNIPPET_ARGS:-} 2>&1 | tee "$out/snippets.log"
        step snippets "${PIPESTATUS[0]}"
        endgroup
    else
        step snippets 1
    fi
fi

"$py" tools/ci/cppdocs_ci.py collect \
    --platform "$CPPDOCS_PLATFORM" --compiler "$CPPDOCS_COMPILER" --preset "$preset" \
    --os-version "${CPPDOCS_OS_VERSION:-}" --areas "$areas" \
    --run-tests "$run_tests" --run-snippets "$run_snippets" \
    --tests "$out/tests.json" --snippets "$out/snippets.json" --snippets-mode "$snippets_mode" \
    ${steps[@]+"${steps[@]}"} -o "$results" --summary "$out/summary.md" || status=1
if [[ -n "${GITHUB_STEP_SUMMARY:-}" && -f "$out/summary.md" ]]; then
    cat "$out/summary.md" >> "$GITHUB_STEP_SUMMARY"
fi
exit "$status"
