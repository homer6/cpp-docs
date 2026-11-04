# Security verification

> Prove the security properties you claim: choose the tool that can see each property, run it on the code paths an attacker reaches, measure what it reached, and keep every finding as a test.

## Mental model

Each method sees some defects and is blind to others, so a security claim
needs the method that can see it:

| Property you claim | Method that can see it | Blind spot |
|---|---|---|
| No out-of-bounds access or use after free on tested paths | AddressSanitizer (ASan) | paths the tests never run |
| No undefined arithmetic, misaligned or null access | UndefinedBehaviorSanitizer (UBSan) | defined-but-wrong results (unsigned wrap) unless you opt in |
| No data races | ThreadSanitizer (TSan), in its own build | interleavings the tests never produce |
| No reads of uninitialized memory | MemorySanitizer (MSan, Clang on Linux), every library instrumented | anything not instrumented |
| Parsers survive malformed input | fuzzing, under ASan and UBSan | inputs it never generated; check coverage |
| Code follows the rules you adopted | compiler warnings, static analysis (clang-tidy, `/analyze`) | anything outside the rules |
| A bad input is rejected, not just survived | tests of the failure path | cases nobody wrote down |
| The design puts trust and authority where it should | review against the [threat model](threat-modeling.md) | — |

No single row is a proof. A clean sanitizer run says the executed paths had
none of the errors that sanitizer detects; warning-free code says only that
no warning fired. Microsoft's guidance for secure C++ lists the same
families: static and dynamic analysis, fuzzing, and tests that check security
behaviour.[^ms]

**MUST:** For each security property in your threat model, name the method
that checks it and where it runs. A property no method checks is an
assumption.

## Choose the sanitizer for the property

The flags and how to read each report are in
[tooling](../guidelines/tooling.md) (ASan, UBSan, TSan, LSan, HWASan, MSan,
platform support). What matters for security:

- **One program, one of ASan, TSan or MSan.** Clang refuses the combination
  (`'-fsanitize=address' not allowed with '-fsanitize=thread'`, checked with
  Clang 22), so race detection and uninitialized-read detection are separate
  builds, each run on the tests that exercise its property.[^clang]
- **MSan is Clang on Linux only** and needs every linked library, the standard
  library included, built with it. Clang 22 on macOS rejects
  `-fsanitize=memory`, and GCC 15 doesn't have it.
- **UBSan recovers by default.** With plain `-fsanitize=undefined`, Clang 22
  prints "signed integer overflow" and carries on. Add
  `-fno-sanitize-recover=undefined` (or `=all`) so the first report stops the
  test and fails it.
- **`undefined` isn't every arithmetic bug.** Unsigned wraparound and
  value-changing implicit conversions are defined behaviour, so `undefined`
  can't report them. Clang's opt-in `-fsanitize=unsigned-integer-overflow`
  and `-fsanitize=implicit-conversion` (both in `-fsanitize=integer`) do,
  measured with Clang 22. GCC 15 has neither, so use the checked arithmetic
  on [integers](integers.md) there. Expect reports from intended wraps such
  as hashes, and suppress those per function.

**MUST:** Treat every sanitizer report as a bug until it's understood; a test
that "passes" with a report hasn't passed.

**MUST NOT** ship a sanitizer build as your production build. Sanitizer
runtimes are diagnostic tools and change memory layout and failure handling.
Ship the [hardened](hardening.md) profile, which is designed for production.

## Fuzz the security boundaries

**MUST:** Fuzz the code that interprets attacker-controlled structure:
protocol decoders, file formats, deserializers, decompression, IPC messages,
configuration parsers. A harness:

- calls the **real** parser, not a copy, and checks its **properties** (an
  accepted value is in range; an accepted span lies inside the buffer), not
  only that it didn't crash;
- builds with ASan and UBSan (`-fsanitize=fuzzer,address,undefined`), so
  silent corruption crashes;
- bounds input size and time, and resets state between inputs;
- keeps every crashing input, minimized, as a regression case replayed by an
  ordinary test.

The repo does this for the parsers on these pages: `fuzz/` has libFuzzer
targets for `parse_port`, `payload` and the bounded nesting check, each
asserting the page's promise, and `tests/security/fuzz_corpus_test.cpp`
replays the saved inputs on every compiler, without libFuzzer. Its first run
found that `parse_port` accepted `"05"` as port 5, a second spelling of the
same port ([fuzz/README.md](../../fuzz/README.md)).

### Measure what the fuzzing reached

**MUST:** Check coverage of the rejection paths, not hours fuzzed. With
Clang, build the replay with `-fprofile-instr-generate -fcoverage-mapping`,
run the corpus, and read the branches:

```bash
clang++ -std=c++20 -fprofile-instr-generate -fcoverage-mapping -Ifuzz replay.cpp -o replay
LLVM_PROFILE_FILE=r.profraw ./replay fuzz/corpus/payload/*
llvm-profdata merge -o r.profdata r.profraw
llvm-cov show ./replay -instr-profile=r.profdata fuzz/parsers.hpp -show-branches=count
```

Doing this on the repo's corpus found two gaps:

- **`payload`:** the input named `too-long` claimed exactly the bytes
  available, so it was accepted, and no saved input reached the
  "claimed length too long" rejection (`Branch (34:9): [True: 0, …]`).
- **`parse_port`:** no input reached the check for values above 65535. The
  `overflow` input fails earlier, in `from_chars`.

Both are fixed: `too-long` now claims one byte more, and `above-max` holds
`65536`. `tests/security/verification_test.cpp` checks that each rejection
has a saved input that takes it.

## Analyse statically

**SHOULD:** Run a static analyser on every change, configured from your real
build (`compile_commands.json`), with the findings blocking. The options:

- clang-tidy's `cppcoreguidelines-*`, `cert-*` and `bugprone-*` checks;
- the Clang Static Analyzer (`scan-build`);
- MSVC's `/analyze`.

GCC's `-fanalyzer` documents itself as "only suitable for use on C code" in
GCC 15. Warnings are the cheapest analysis; the baseline is in
[project conventions](../guidelines/project-conventions.md#warnings-and-static-analysis).

The repo's profile is `.clang-tidy`; run it with
`tools/run_clang_tidy.sh` ([testing](../testing.md#static-analysis-308)).
Turning it on for this section found a polymorphic base that allowed copying
(C.67) and pointer arithmetic in `parse_port`. It also found a mutable global
in a test. Each was fixed. A finding you keep stays in the source as
`// NOLINT(<check>): <why>`, so a reviewer can judge it.

## Test the intended failure behaviour

**MUST:** Test the failure paths, which attackers aim for, as deliberately as
the success paths. Cover:

- malformed input and boundary values (one below, at and one above each
  limit);
- failed allocation and resource exhaustion ([resource limits](resource-limits.md));
- partial I/O and cancellation;
- failed authorization and invalid peer identity ([threat modeling](threat-modeling.md));
- shutdown while work is in flight.

The tests under `tests/security/` do this for each page. Each asserts the
outcome the page promises: rejected, denied, rolled back, stopped.

**MUST:** Check that protections you rely on actually fire. Write a test that
does what the protection must stop and assert that it stops; a flag in the
build file isn't evidence ([hardening](hardening.md#check-the-binary-not-the-build-file)).
The repo's death tests in `tests/security/hardening_test.cpp` index past the
end of a vector, `array`, `span` and `string` under each library's hardening
mode and expect the process to stop.

**MUST:** Give every security defect a regression test, and look for the same
mistake elsewhere: fix the pattern, not only the instance.

## How this repo verifies itself

| Method | Where | Docs |
|---|---|---|
| Unit tests of every claim | `tests/security/` (one file per page) | [testing](../testing.md#running-the-tests) |
| ASan + UBSan, TSan | `*-asan` presets, `clang-sanitizers` CI job | [testing](../testing.md#sanitizers-263) |
| Hardened builds and death tests | `*-hardened` presets, `hardening_test.cpp` | [testing](../testing.md#hardened-builds-305) |
| Fuzzing and corpus replay | `fuzz/`, `fuzz_corpus_test.cpp` | [fuzz/README.md](../../fuzz/README.md) |
| clang-tidy | `.clang-tidy`, `tools/run_clang_tidy.sh` | [testing](../testing.md#static-analysis-308) |
| Every snippet compiled, `bad run` ones caught by a sanitizer | `tools/check_snippets.py` | [testing](../testing.md#checking-the-snippets) |

## See also

- [Tooling](../guidelines/tooling.md) (sanitizer flags, reading reports) ·
  [testing guidelines](../guidelines/testing.md) ·
  [Memory & safety](../guidelines/safety.md)
- [Hardening](hardening.md) · [parsing and validation](parsing-and-validation.md) ·
  [threat modeling](threat-modeling.md)
- Clang: [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html) ·
  [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) ·
  [ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html) ·
  [MemorySanitizer](https://clang.llvm.org/docs/MemorySanitizer.html) ·
  [libFuzzer](https://llvm.org/docs/LibFuzzer.html) ·
  [source-based coverage](https://clang.llvm.org/docs/SourceBasedCodeCoverage.html) ·
  [clang-tidy](https://clang.llvm.org/extra/clang-tidy/)
- [GCC static analyzer](https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html) ·
  [MSVC code analysis](https://learn.microsoft.com/en-us/cpp/code-quality/code-analysis-for-c-cpp-overview)
- Tests: [`tests/security/verification_test.cpp`](../../tests/security/verification_test.cpp) ·
  [`fuzz_corpus_test.cpp`](../../tests/security/fuzz_corpus_test.cpp)

[^ms]: Microsoft Learn, [Build reliable and secure C++ programs](https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs).
[^clang]: Clang User's Manual, [Controlling code generation](https://clang.llvm.org/docs/UsersManual.html#controlling-code-generation): address, thread and memory sanitizers can't be combined in one program. Every behaviour on this page attributed to Clang 22 or GCC 15 was checked locally with Homebrew LLVM 22.1.8 and GCC 15.2.
