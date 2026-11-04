# Fuzz targets

libFuzzer targets for the parsers on the security pages (#307). Each target
feeds one input to a parser from [`parsers.hpp`](parsers.hpp) (the same code
[`docs/security/parsing-and-validation.md`](../docs/security/parsing-and-validation.md)
and [`bounds.md`](../docs/security/bounds.md) show) and checks the properties
the page promises; a broken property aborts, which libFuzzer reports as a
crash. The targets live in [`targets.hpp`](targets.hpp).

| Target | Parser | Property checked |
|---|---|---|
| `fuzz_parse_port` | `parse_port` | an accepted port is 1–65535 and spelled exactly as its digits (no second spelling) |
| `fuzz_payload` | `payload` | an accepted payload has the claimed length and lies inside the packet |
| `fuzz_nesting` | `nesting` | an accepted string is balanced and no deeper than the limit |

## Build and run (Clang)

```bash
cmake --preset clang-22 -B build/fuzz -DCPPDOCS_FUZZ=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/fuzz --target fuzz_targets
tools/ci/fuzz.sh 60          # each target for 60 s on a copy of its corpus
```

The targets are built with `-fsanitize=fuzzer,address,undefined`
(`fuzzer,address` on Apple: Homebrew LLVM 22's objects with UBSan don't link
with the macOS 14 system linker). The CI job (`fuzz` in
`.github/workflows/linux.yml`) runs each for 120 s with Clang 22 whenever the
tests run, and uploads any failing input.

## The corpus

`corpus/<target>/` holds seed inputs and every minimized failing input, kept as
regression cases. `tests/security/fuzz_corpus_test.cpp` replays all of them
through the same targets on every platform and compiler, without libFuzzer.
When a run finds a crash: fix the code, minimize the input
(`fuzz_<target> -minimize_crash=1 -runs=10000 <input>`), and save it in
`corpus/<target>/` with a name that says what it is.

The first run of `fuzz_parse_port` found that `parse_port` accepted `"05"`
as 5, a second spelling of the same port; the parser now rejects leading
zeros, and `corpus/parse_port/leading-zero` keeps the input.

## Coverage

Fuzzing time says nothing about what was reached; coverage does. Replay a
corpus through a coverage build and read the branch counts (the commands are
in [verification](../docs/security/verification.md#measure-what-the-fuzzing-reached)).
The first check, #301, found that no saved input reached `payload`'s
"claimed length too long" rejection (`too-long` claimed exactly the bytes
available) or `parse_port`'s check for values above 65535. `too-long` was
corrected and `above-max` added; `tests/security/verification_test.cpp`
keeps each rejection reached.
