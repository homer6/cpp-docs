# Compiler and library hardening

> Ship with the standard library's precondition checks and the compiler's and linker's protections turned on, choose them for the toolchain and library you actually use, and check the final binary rather than the build file.

## Mental model

Hardening is the layer that catches what the source-level rules missed. It
comes from three places, each with its own switches, and none of them is
turned on by `-std=` or, in most toolchains, by default:

| Layer | What it does | Example switches |
|---|---|---|
| Standard library | turns preconditions (`v[i]` in range, `optional` engaged) into checks that stop the program | `_GLIBCXX_ASSERTIONS`, `_LIBCPP_HARDENING_MODE`, `_MSVC_STL_HARDENING` |
| Compiler | adds checks and changes code generation | stack protector, `_FORTIFY_SOURCE`, zero-initialized locals, control-flow protection |
| Linker / binary | makes the loaded program harder to exploit | PIE/ASLR, full RELRO, non-executable stack, CFG/CET on Windows |

A hardening check firing is evidence of a bug: the program stops instead of
reading or writing past an object. Fix the bug; hardening doesn't replace the
check that should have rejected the input ([bounds](bounds.md),
[error handling](error-handling.md#stop-on-broken-invariants)).

## Turn on standard-library hardening

**MUST:** Enable the hardening of the standard library you actually link,
chosen by the library, not by the compiler's name (Clang can use libstdc++ or
libc++):

| Library | Switch | Default | Notes |
|---|---|---|---|
| libstdc++ (GCC; Clang on most Linux) | `-D_GLIBCXX_ASSERTIONS` | on in **unoptimized** builds since GCC 15, off with `-O1` and up | checks `operator[]`, `front()`/`back()`, `optional`/`expected` access and more |
| libc++ (LLVM; Apple; Clang with `-stdlib=libc++`) | `-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST` (or `_EXTENSIVE`, `_DEBUG`) | chosen by whoever built the library (Homebrew's LLVM 22: none) | *fast* is the documented production mode; *extensive* adds checks at more cost |
| MS STL (MSVC, clang-cl) | `/D_MSVC_STL_HARDENING=1` | off | VS 2022 17.14 and later |

Sources: GCC 15 release notes, libc++'s hardening documentation, the MS STL
wiki; checked locally in [language versions](language-versions.md#c26-contracts-and-library-hardening).
The repo's death tests ([`hardening_test.cpp`](../../tests/security/hardening_test.cpp))
show each of these checks firing: an out-of-range `operator[]` on `vector`,
`array`, `span`, `string` and `string_view`, `front()` on an empty vector,
and `*` on an empty `optional` or an `expected` holding an error all stop the
program under libstdc++ 15/16 and libc++ 22 when hardening is on.

**MUST NOT** ship the *debug* modes in place of hardening.
`_GLIBCXX_DEBUG` "changes the sizes and behavior of standard class templates
such as `std::vector`", so code built with it can't be mixed with code built
without it; libc++'s *debug* mode and MSVC's `_ITERATOR_DEBUG_LEVEL=2` are for
testing too. The hardening switches above don't change the ABI.

## Turn on compiler and linker protections

**MUST:** Keep an approved hardening profile per platform, and check it in
the artifact. On GCC and Clang (Linux unless noted):

| Flag | Protects against |
|---|---|
| `-fstack-protector-strong` | stack buffer overflows overwriting the return address |
| `-D_FORTIFY_SOURCE=3` | overflows in `memcpy`, `strcpy`, `printf` and friends with known sizes (glibc; needs optimization; Apple's SDK enables its own) |
| `-ftrivial-auto-var-init=zero` | reads of uninitialized locals leaking old stack data |
| `-fstack-clash-protection` | stack growth jumping over the guard page |
| `-fcf-protection=full` (x86), `-mbranch-protection=standard` (arm64) | return-oriented and jump-oriented programming |
| PIE (`-fPIE -pie`) | fixed addresses (enables full ASLR) |
| `-Wl,-z,relro -Wl,-z,now` | overwriting the GOT and other relocated data |
| `-Wl,-z,noexecstack` | executing code from the stack |

GCC 14 and later on GNU/Linux bundle most of these as `-fhardened`
(`g++ --help=hardened` lists them: `_FORTIFY_SOURCE=3`, `_GLIBCXX_ASSERTIONS`,
`-ftrivial-auto-var-init=zero`, PIE, `-z now -z relro`,
`-fstack-protector-strong`, `-fstack-clash-protection`, `-fcf-protection=full`).
It warns that `_FORTIFY_SOURCE` isn't enabled without optimization, and GCC 15
on macOS reports `-fhardened` "not supported for this target"; Clang has no
`-fhardened`. The OpenSSF *Compiler Options Hardening Guide* has the full,
per-version list.[^ossf]

On MSVC: `/GS` (on by default), `/sdl` (extra security checks; security
warnings become errors), `/guard:cf` passed to both compiler and linker
(Control Flow Guard), `/CETCOMPAT` on x64 (shadow-stack compatibility), and
the linker's `/DYNAMICBASE` (ASLR, on by default) and `/NXCOMPAT` (DEP).[^ms]
One of the warnings `/sdl` makes an error is C4996, which deprecates standard
C functions such as `fopen`, `gmtime`, `strncpy` and `sscanf` in favour of
Microsoft's `_s` versions (C11's optional Annex K, which glibc doesn't
provide: glibc 2.41 doesn't define `__STDC_LIB_EXT1__`). Portable code that calls the standard functions defines
`_CRT_SECURE_NO_WARNINGS` next to `/sdl`, which silences only that warning;
this repo's hardened MSVC build does
([#380](https://github.com/homer6/cpp-docs/issues/380)).

**Check each flag on your toolchain.** Measured in this repo: with Homebrew
GCC 15 on Apple silicon, `-mbranch-protection=pac-ret` (part of `standard`)
breaks C++ exception unwinding, so every `throw` aborts; Clang 22 with the same
flag is fine ([#327](https://github.com/homer6/cpp-docs/issues/327)). A
flag the build file mentions isn't a flag that worked.

## Check the binary, not the build file

**MUST:** Confirm the protections in what you ship:

```bash
# Library hardening and fortification in effect for a translation unit
g++ -std=c++20 -O2 -D_GLIBCXX_ASSERTIONS -dM -E -include vector -x c++ /dev/null \
    | grep -E '_GLIBCXX_ASSERTIONS|_FORTIFY_SOURCE'

# ELF: PIE, RELRO + BIND_NOW, non-executable stack, fortified calls
readelf -h app | grep Type              # DYN (Position-Independent Executable file)
readelf -l app | grep -A1 GNU_STACK     # flags RW, not RWE
readelf -d app | grep -E 'BIND_NOW|FLAGS'
nm -D app | grep -E '__[a-z]+_chk'      # __printf_chk, __memcpy_chk ...

# Windows: Dynamic base, NX compatible, Guard CF
dumpbin /headers app.exe | findstr /i "Dynamic NX Guard"
```

The repo's hardened GCC 16 build shows exactly that: `DYN (Position-Independent
Executable file)`, `GNU_STACK … RW`, a `GNU_RELRO` segment, `FLAGS BIND_NOW`
and `FLAGS_1 NOW PIE`, and calls to `__printf_chk`, `__fread_chk` and
`__fprintf_chk`. Then run a test that does something hardening must stop and
check that it stops (the death tests above).

## How this repo builds hardened

`CPPDOCS_HARDEN=ON` (the `gcc-15-hardened`, `clang-22-hardened` and
`gcc-16-hardened` presets, and hardened CI jobs on Linux, macOS and Windows)
applies the library switch for the detected library and the flags above,
probing each one first, and records the flags actually used in the results;
the platform report gives each hardened job its own page. The whole suite
passes hardened on GCC 15 (1514 tests), Clang 22 with libc++ (1514) and GCC 16
on Linux (1517). See [`docs/testing.md`](../testing.md#hardened-builds-305).

## Evaluate stronger mitigations for high-risk code

**SHOULD:** For parsers and other exposed components, evaluate Clang's
control-flow integrity (`-fsanitize=cfi`, which needs LTO and has visibility
and shared-library constraints), shadow call stacks, and hardware memory
tagging where the platform has it. Test them; don't assume they apply to every
call.

**MUST:** Keep the source-level prevention. Mitigations make exploitation
harder; they don't make a known bug acceptable.

## See also

- [Language versions](language-versions.md) (what `-std=` does and doesn't turn on) ·
  [bounds](bounds.md) · [verification](verification.md)
- [Memory & safety: hardening in practice](../guidelines/safety.md#hardening-in-practice) ·
  [build and packaging](../guidelines/build-and-packaging.md)
- [libc++ hardening modes](https://libcxx.llvm.org/Hardening.html) ·
  [libstdc++ debug mode](https://gcc.gnu.org/onlinedocs/libstdc++/manual/debug_mode_using.html) ·
  [MS STL hardening](https://github.com/microsoft/STL/wiki/STL-Hardening)
- Tests: [`tests/security/hardening_test.cpp`](../../tests/security/hardening_test.cpp)

[^ossf]: OpenSSF, [*Compiler Options Hardening Guide for C and C++*](https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.md). `-fhardened`'s list and warnings checked with GCC 16.2 (Linux) and GCC 15.2 (macOS).
[^ms]: Microsoft Learn: [/sdl](https://learn.microsoft.com/en-us/cpp/build/reference/sdl-enable-additional-security-checks), [/guard](https://learn.microsoft.com/en-us/cpp/build/reference/guard-enable-guard-checks), [/GS](https://learn.microsoft.com/en-us/cpp/build/reference/gs-buffer-security-check), [Security best practices for C++](https://learn.microsoft.com/en-us/cpp/security/security-best-practices-for-cpp).
