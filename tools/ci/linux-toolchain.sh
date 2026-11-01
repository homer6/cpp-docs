#!/usr/bin/env bash
# Install one Linux toolchain for tools/ci/run.sh and print its settings as
# NAME=value lines; appended to $GITHUB_ENV in Actions.
#
#   tools/ci/linux-toolchain.sh gcc-image                 # inside an official gcc:N container
#   tools/ci/linux-toolchain.sh gcc-ubuntu 15             # Ubuntu's g++-15 (the runner's archive)
#   tools/ci/linux-toolchain.sh clang-apt-llvm 20 libcxx  # apt.llvm.org clang-20 + libc++
#   tools/ci/linux-toolchain.sh clang-ubuntu 22 libstdcxx # Ubuntu's clang-22 + libstdc++
#
# Every kind also installs cmake, ninja-build and python3 from the
# distribution. Sets CC (GoogleTest's project() needs a C compiler too), CXX, CPPDOCS_PRESET, CPPDOCS_COMPILER (gcc-N,
# clang-N-libstdcxx, clang-N-libcxx), CPPDOCS_PLATFORM
# (linux-<arch>-<distro>-<version>), CPPDOCS_OS_VERSION, CPPDOCS_BASELINE and
# CPPDOCS_SNIPPET_ARGS. Needs root or passwordless sudo, and apt.
#
# apt.llvm.org: the repository is added by hand (key + one deb line, the
# instructions on https://apt.llvm.org/) rather than by piping llvm.sh to a
# shell; llvm.sh's default version also lags (it still says 22).
set -euo pipefail
kind="${1:?usage: linux-toolchain.sh gcc-image|gcc-ubuntu N|clang-apt-llvm N STDLIB|clang-ubuntu N STDLIB}"
ver="${2:-}"
stdlib="${3:-libstdcxx}"

sudo=""
[[ "$(id -u)" == 0 ]] || sudo="sudo"
export DEBIAN_FRONTEND=noninteractive

# shellcheck source=/dev/null
. /etc/os-release
arch="$(uname -m)"
apt_install() {
    $sudo apt-get update -qq
    $sudo apt-get install -y -qq --no-install-recommends "$@" >/dev/null
}
base=(cmake ninja-build python3 git ca-certificates)

env=("CPPDOCS_PLATFORM=linux-${arch}-${ID}-${VERSION_ID}"
     "CPPDOCS_OS_VERSION=${PRETTY_NAME} ${arch}, kernel $(uname -r)")
case "$kind" in
gcc-image)
    apt_install "${base[@]}"
    env+=("CC=gcc" "CXX=g++" "CPPDOCS_PRESET=gcc" "CPPDOCS_COMPILER=gcc-$(g++ -dumpversion | cut -d. -f1)"
          "CPPDOCS_BASELINE=tools/snippet_baseline/gcc.txt" "CPPDOCS_SNIPPET_ARGS=")
    ;;
gcc-ubuntu)
    apt_install "${base[@]}" "g++-$ver"
    env+=("CC=gcc-$ver" "CXX=g++-$ver" "CPPDOCS_PRESET=gcc" "CPPDOCS_COMPILER=gcc-$ver"
          "CPPDOCS_BASELINE=tools/snippet_baseline/gcc.txt" "CPPDOCS_SNIPPET_ARGS=")
    ;;
clang-apt-llvm|clang-ubuntu)
    if [[ "$kind" == clang-apt-llvm ]]; then
        apt_install curl gnupg ca-certificates
        curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key \
            | gpg --dearmor | $sudo tee /usr/share/keyrings/apt-llvm.gpg >/dev/null
        echo "deb [signed-by=/usr/share/keyrings/apt-llvm.gpg] https://apt.llvm.org/${VERSION_CODENAME}/ llvm-toolchain-${VERSION_CODENAME}-${ver} main" \
            | $sudo tee /etc/apt/sources.list.d/apt-llvm.list >/dev/null
    fi
    pkgs=("${base[@]}" "clang-$ver")
    if [[ "$stdlib" == libcxx ]]; then
        pkgs+=("libc++-$ver-dev" "libc++abi-$ver-dev")
        env+=("CPPDOCS_PRESET=clang-libcxx" "CPPDOCS_BASELINE=tools/snippet_baseline/clang.txt"
              "CPPDOCS_SNIPPET_ARGS=--flag=-stdlib=libc++ --flag=-fexperimental-library")
    else
        # libstdc++ from the newest GCC on the machine; clang.txt lists libc++
        # gaps, so there's no baseline for this pairing.
        env+=("CPPDOCS_PRESET=clang" "CPPDOCS_BASELINE=" "CPPDOCS_SNIPPET_ARGS=")
    fi
    apt_install "${pkgs[@]}"
    env+=("CC=clang-$ver" "CXX=clang++-$ver" "CPPDOCS_COMPILER=clang-$ver-$stdlib")
    # Ubuntu 26.04 also installs a copy of libc++.modules.json in the multiarch
    # directory, and clang reports that one; its relative paths
    # (../share/libc++/v1/std.cppm) don't resolve from there, so `import std`
    # can't find its sources. The copy under /usr/lib/llvm-N/lib resolves.
    json="/usr/lib/llvm-$ver/lib/libc++.modules.json"
    if [[ "$stdlib" == libcxx && -f "$json" ]]; then
        env+=("CPPDOCS_CMAKE_ARGS=-DCMAKE_CXX_STDLIB_MODULES_JSON=$json")
    fi
    ;;
*)
    echo "linux-toolchain.sh: unknown kind '$kind'" >&2
    exit 2
    ;;
esac

printf '%s\n' "${env[@]}"
if [[ -n "${GITHUB_ENV:-}" ]]; then
    printf '%s\n' "${env[@]}" >> "$GITHUB_ENV"
fi
cxx="$(printf '%s\n' "${env[@]}" | sed -n 's/^CXX=//p')"
"$cxx" --version >&2
