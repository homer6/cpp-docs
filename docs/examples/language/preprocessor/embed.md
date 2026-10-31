# `#embed` — resource inclusion (C++26)

> Pull the bytes of a file into the program at compile time, as a
> comma-separated list of integers. **Directive:** `#embed` · **Test:**
> `__has_embed`, `__cpp_pp_embed` (`202502L`) · **Since:** C++26 (C23 in C)

## Mental model

```
logo.png on disk:  89 50 4E 47 0D 0A 1A 0A ...

constexpr unsigned char logo[] = {
#embed "logo.png"                 -->   137, 80, 78, 71, 13, 10, 26, 10, ...
};
```

`#embed` is `#include` for data: the preprocessor replaces the directive with
one `int` integer literal per byte (strictly, per `CHAR_BIT` bits) of the
resource. It replaces `xxd -i` scripts and linker tricks, and compilers can
implement it without materialising millions of tokens, so it's fast for large
files.

## Declaration

```
#embed "resource" parameters...      quote form: searched like "header", then like <resource>
#embed <resource> parameters...      bracket form: implementation-defined search paths
                                     (GCC/Clang: --embed-dir=<dir>, -I is not used)
```

| Parameter | Effect |
|---|---|
| `limit(n)` | at most `n` elements (a `#if`-style constant expression ≥ 0) |
| `offset(n)` | skip the first `n` elements (≥ 0) — **C++29**, see below |
| `prefix(tokens)` | placed before the list — **only if the resource isn't empty** |
| `suffix(tokens)` | placed after the list — **only if the resource isn't empty** |
| `if_empty(tokens)` | replaces the whole directive **only if the resource is empty** |

Each may appear at most once. A resource is *empty* when the count after
`offset` and `limit` is zero, so `limit(0)` always makes it empty.

C++26 (N5050) and C23 standardise `limit`, `prefix`, `suffix` and `if_empty`.
The plain `offset` parameter was added for C++29 by
[P3540R3](https://wg21.link/p3540) (it's in the current working draft at
eel.is) and **no local compiler accepts it yet** — GCC 15/16: "unknown embed
parameter 'offset'"; Clang 22: "unknown embed preprocessor parameter
'offset'". What works today is the vendor spelling it standardises: GCC's
`gnu::offset` and Clang's `clang::offset`. An unsupported parameter makes
`__has_embed` return `0`, so you can test for it.

`__has_embed(resource params…)` is usable in `#if` and evaluates to:

| Macro | Value | Meaning |
|---|---|---|
| `__STDC_EMBED_NOT_FOUND__` | `0` | not found, or a parameter isn't supported |
| `__STDC_EMBED_FOUND__` | `1` | found and not empty |
| `__STDC_EMBED_EMPTY__` | `2` | found but empty (after `offset`/`limit`) |

A missing resource in `#embed` itself is ill-formed (a hard error).

## Compiler support (tested)

| Compiler | `-std=c++26` | `-std=c++26 -pedantic-errors` | `__cpp_pp_embed` | standard `offset` (C++29) |
|---|---|---|---|---|
| GCC 15.2 | ✅ | ✅ | `202502L` | ❌ (`gnu::offset` ✅) |
| GCC 16.2 | ✅ | ✅ | `202502L` | ❌ (`gnu::offset` ✅) |
| Clang 22.1 | ✅ with a warning: "`#embed` is a Clang extension" (`-Wc23-extensions`) | ❌ the warning becomes an error | not defined | ❌ (`clang::offset` ✅) |

All three produce the same values, honour the four C++26 parameters, and give
`__has_embed` the values above. Clang implements `#embed` for C23 and accepts it in C++ as an
extension, but doesn't yet claim the C++26 feature — so test `__has_embed`/`__cpp_pp_embed` rather than the standard
version, and expect `-pedantic-errors` builds on Clang to reject it.

## When to use it

- Shaders, fonts, images, certificates, test vectors, lookup tables — any data
  you want inside the binary and available in `constexpr`.
- Instead of generated `.inc` files (`xxd -i`), `objcopy`/`ld -b binary`
  symbols, or `incbin` assembler tricks: `#embed` is portable, sizes are known
  at compile time, and build systems can track the dependency.
- **Not** for data you want to change without rebuilding.

## Examples

These examples were run with GCC 15, GCC 16 and Clang 22 against a 5-byte
`hello.txt` containing `Hello` and an empty `empty.bin`. Blocks that need those
files are tagged `nocompile`.

### Bytes into an array

```cpp nocompile
constexpr unsigned char hello[] = {
#embed "hello.txt"
};
static_assert(sizeof hello == 5 && hello[0] == 'H');
```

Initialising `unsigned char` (or `std::byte`-sized integers) from the `int`
literals is not narrowing: each value is a constant that fits.

### A null-terminated string: `suffix`

```cpp nocompile
constexpr char text[] = {
#embed "hello.txt" suffix(, 0)
};
static_assert(sizeof text == 6 && text[5] == '\0');   // std::puts(text) prints "Hello"
```

### `limit`, and skipping bytes today

```cpp nocompile
constexpr unsigned char magic[] = {
#embed "hello.txt" limit(2)             // 'H', 'e'
};
static_assert(sizeof magic == 2 && magic[1] == 'e');
```

Skipping a header with the vendor `offset` (accepted by GCC 15/16 and
Clang 22; `offset` is applied first, then `limit`):

```cpp nocompile
constexpr unsigned char middle[] = {
#if defined(__clang__)
#embed "hello.txt" clang::offset(1) limit(2)
#else
#embed "hello.txt" gnu::offset(1) limit(2)
#endif
};
static_assert(sizeof middle == 2 && middle[0] == 'e' && middle[1] == 'l');
```

With C++29 that becomes `#embed "hello.txt" offset(1) limit(2)`.

### `if_empty`, `prefix`, `suffix`

`prefix` and `suffix` are dropped for an empty resource; `if_empty` is used
only then. Together they keep an initializer valid either way:

```cpp nocompile
constexpr unsigned char data[] = {
#embed "empty.bin" prefix(0xAA, ) suffix(, 0xBB) if_empty(0xFF)
};
static_assert(sizeof data == 1 && data[0] == 0xFF);   // with hello.txt: 7 bytes, AA ... BB
```

### Optional resources with `__has_embed`

This one compiles anywhere: it falls back when the file isn't there.

```cpp
#if defined(__has_embed)
#  if __has_embed("config.json") == __STDC_EMBED_FOUND__
#    define HAVE_CONFIG 1
#  endif
#endif

#ifdef HAVE_CONFIG
constexpr char config[] = {
#embed "config.json" suffix(, 0)
};
#else
constexpr char config[] = "{}";
#endif

static_assert(sizeof config >= 1);
```

### A missing resource is an error

```cpp compile-fail="no-such-resource"
constexpr unsigned char nope[] = {
#embed "no-such-resource.bin"
};
```

## Gotchas

- **The elements are `int` literals, not bytes.** Into `unsigned char` they
  always fit. Into a plain `char` (signed on x86 and ARM64 macOS), a byte ≥ 0x80
  is a narrowing conversion: GCC 15/16 reject it (`narrowing conversion of '255'
  from 'int' to 'char'`), Clang 22 accepts it silently. Use `unsigned char` or
  `std::uint8_t` for binary data.
- **No terminator unless you ask.** A text file embedded into `char[]` has no
  `'\0'`; add `suffix(, 0)` (and for an empty file, `if_empty(0)`).
- **`prefix`/`suffix` vanish on empty input.** If the code after `#embed`
  depends on a trailing comma from `suffix(,)`, provide `if_empty` too.
- **Bracket form doesn't search `-I`.** GCC and Clang use `--embed-dir=` for
  `#embed <…>`; the quote form searches relative to the current file first.
- **Paths are relative to the source file**, not the working directory, and
  `#line` doesn't change that.
- **`limit(0)` still needs the file to exist.** The resource is located (and
  must be processable) even when nothing is read.
- **`limit(n)` with a `constexpr` variable is silently `limit(0)`.** The
  argument is a `#if`-style expression, where an unknown identifier is `0`, so
  the resource becomes empty (verified with GCC and Clang). Use a macro.

## See also

- [Resource inclusion (`#embed`) — cppreference](https://en.cppreference.com/w/cpp/preprocessor/embed)
- [\[cpp.embed\]](https://eel.is/c++draft/cpp.embed),
  [\[cpp.cond\]](https://eel.is/c++draft/cpp.cond) (`__has_embed`) — C++ working draft
- [N5050](https://wg21.link/n5050) — C++26 draft (`[cpp.embed]`; no `offset`)
- [P3540R3 — `#embed` offset parameter (C++29)](https://wg21.link/p3540)
- [P1967 — `#embed`, a simple, scannable preprocessor-based resource acquisition method](https://wg21.link/p1967)
- [GCC: `--embed-dir`](https://gcc.gnu.org/onlinedocs/gcc/Directory-Options.html) ·
  [Clang: C/C++ language status](https://clang.llvm.org/cxx_status.html)
- [The preprocessor](preprocessor.md)
