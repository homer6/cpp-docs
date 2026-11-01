#!/usr/bin/env bash
# Pick (and if needed install) one macOS toolchain for tools/ci/run.sh and
# print its settings as NAME=value lines; appended to $GITHUB_ENV in Actions.
#
#   tools/ci/macos-toolchain.sh apple-clang     # Xcode's clang++ + the SDK's libc++
#   tools/ci/macos-toolchain.sh homebrew-gcc    # brew install gcc; newest g++-N
#   tools/ci/macos-toolchain.sh homebrew-llvm   # brew install llvm; its own libc++
#
# Sets CC, CXX, CPPDOCS_PRESET, CPPDOCS_COMPILER (label with the major version),
# CPPDOCS_PLATFORM (macos-<major>-<arch>), CPPDOCS_OS_VERSION (macOS and
# Xcode versions), CPPDOCS_BASELINE and, for Homebrew LLVM, LDFLAGS,
# CPPDOCS_CMAKE_ARGS and CPPDOCS_SNIPPET_ARGS (the same flags as the local
# clang-22 preset and CLAUDE.md's snippet command).
set -euo pipefail
kind="${1:?usage: macos-toolchain.sh apple-clang|homebrew-gcc|homebrew-llvm}"

major() { sed -nE 's/.*version ([0-9]+)\..*/\1/p' | head -1; }

macos="$(sw_vers -productVersion)"
xcode="$(xcodebuild -version 2>/dev/null | head -1 || echo 'Command Line Tools only')"
arch="$(uname -m)"
env=(
    "CPPDOCS_PLATFORM=macos-${macos%%.*}-${arch}"
    "CPPDOCS_OS_VERSION=macOS ${macos} ($(sw_vers -buildVersion)), ${xcode}"
)
case "$kind" in
apple-clang)
    cxx="$(xcrun -f clang++)"
    env+=("CC=$(xcrun -f clang)" "CXX=$cxx" "CPPDOCS_PRESET=clang" "CPPDOCS_BASELINE="
          "CPPDOCS_COMPILER=apple-clang-$("$cxx" --version | major)")
    ;;
homebrew-gcc)
    if ! brew list --formula gcc >/dev/null 2>&1; then
        brew install gcc >&2
    fi
    prefix="$(brew --prefix gcc)"
    # The formula's newest driver: g++-16 for gcc 16.x.
    cxx="" best=0
    for f in "$prefix"/bin/g++-[0-9]*; do
        n="${f##*/g++-}"
        [[ "$n" =~ ^[0-9]+$ ]] && (( n > best )) && best=$n cxx=$f
    done
    [[ -n "$cxx" ]] || { echo "macos-toolchain.sh: no g++-N in $prefix/bin" >&2; exit 1; }
    env+=("CC=${cxx/g++-/gcc-}" "CXX=$cxx" "CPPDOCS_PRESET=gcc" "CPPDOCS_BASELINE=tools/snippet_baseline/gcc.txt"
          "CPPDOCS_COMPILER=gcc-$("$cxx" -dumpversion | cut -d. -f1)")
    ;;
homebrew-llvm)
    if ! brew list --formula llvm >/dev/null 2>&1; then
        brew install llvm >&2
    fi
    p="$(brew --prefix llvm)"
    cxx="$p/bin/clang++"
    # Link and load LLVM's own libc++ (the formula's caveats), not the SDK's.
    env+=("CC=$p/bin/clang" "CXX=$cxx" "CPPDOCS_PRESET=clang-libcxx" "CPPDOCS_BASELINE=tools/snippet_baseline/clang.txt"
          "CPPDOCS_COMPILER=llvm-$("$cxx" --version | major)"
          "LDFLAGS=-L$p/lib/c++ -L$p/lib/unwind -lunwind -Wl,-rpath,$p/lib/c++ -Wl,-rpath,$p/lib/unwind"
          "CPPDOCS_CMAKE_ARGS=-DCMAKE_CXX_STDLIB_MODULES_JSON=$p/lib/c++/libc++.modules.json"
          "CPPDOCS_SNIPPET_ARGS=--flag=-stdlib=libc++ --flag=-fexperimental-library")
    ;;
*)
    echo "macos-toolchain.sh: unknown toolchain '$kind'" >&2
    exit 2
    ;;
esac

printf '%s\n' "${env[@]}"
if [[ -n "${GITHUB_ENV:-}" ]]; then
    printf '%s\n' "${env[@]}" >> "$GITHUB_ENV"
fi
