# std::text_encoding

> Identifies a character encoding by its IANA registry entry, and answers "what
> encoding are my string literals?" and "what encoding does the environment
> use?". **Header:** `<text_encoding>` · **Since:** C++26

## Mental model

`std::text_encoding` is a small value type: an IANA *MIBenum* number
(`id::UTF8` is 106) plus the name it was created from. It **identifies**
encodings; it does not convert between them.

```
literal()       ── compile time ─▶  encoding of "ordinary" string literals (-fexec-charset)
environment()   ── run time ─────▶  encoding of the environment at program start (LANG/LC_ALL)
locale.encoding() ───────────────▶  encoding of a particular std::locale
text_encoding{"utf8"}            ▶  lookup by name (IANA aliases, loose matching)
```

## Declaration

```cpp nocompile
struct text_encoding {
    static constexpr std::size_t max_name_length = 63;
    enum class id : std::int_least32_t { other = 1, unknown = 2, ASCII = 3, ISOLatin1 = 4, /* … */ UTF8 = 106, /* … */ };

    constexpr text_encoding() = default;                          // id::unknown, name ""
    constexpr explicit text_encoding(std::string_view name) noexcept;
    constexpr text_encoding(id i) noexcept;

    constexpr id mib() const noexcept;
    constexpr const char* name() const noexcept;
    constexpr aliases_view aliases() const noexcept;              // all IANA names for it

    friend constexpr bool operator==(const text_encoding&, const text_encoding&) noexcept;
    friend constexpr bool operator==(const text_encoding&, id) noexcept;

    static consteval text_encoding literal() noexcept;
    static text_encoding environment();
    template<id I> static bool environment_is();
};
// and std::locale::encoding() (C++26)
```

- A name the registry doesn't know gives `mib() == id::other`, keeping the name.
- Names are compared the IANA way: case-insensitive, ignoring punctuation and
  leading zeros, so `"utf8"`, `"UTF-8"` and `"csUTF8"` are equal.

## When to use it

- **Refusing to run (or warning) when assumptions don't hold**, e.g. a program
  that writes UTF-8 to the terminal checking that the environment is UTF-8.
- **`static_assert` that literals are UTF-8** before relying on byte values of
  `"é"`.
- **Naming an encoding in an API** with a standard type instead of a string.

Anti-fit: converting text. There is still no standard transcoding facility
(`<codecvt>` is deprecated); use the [multibyte functions](null-terminated-multibyte-strings.md)
or a library such as ICU.

## Examples

### What am I running with?

```cpp
#include <print>
#include <text_encoding>

int main() {
    constexpr auto lit = std::text_encoding::literal();
    static_assert(lit.mib() == std::text_encoding::id::UTF8, "literals must be UTF-8");

    auto env = std::text_encoding::environment();
    std::println("literal: {} | environment: {} (mib {})", lit.name(), env.name(), int(env.mib()));
    std::println("UTF-8 terminal: {}",
                 std::text_encoding::environment_is<std::text_encoding::id::UTF8>());
}
```

Built with `g++-15 -std=c++26 … -lstdc++exp` (see Gotchas):

| Run as | Output (macOS) | Output (Linux/glibc, GCC 16) |
|---|---|---|
| `LANG=en_US.UTF-8` / `LC_ALL=C.UTF-8` | `environment: UTF-8 (mib 106)`, `UTF-8 terminal: true` | same |
| `LC_ALL=C` | `environment: US-ASCII (mib 3)`, `false` | `environment: ANSI_X3.4-1968 (mib 3)`, `false` |

The *name* is whatever the C library reports; compare `mib()` values, not names.

### Looking encodings up by name

```cpp
#include <print>
#include <text_encoding>

int main() {
    std::text_encoding e{"utf8"};
    std::println("{} mib={} isUTF8={}", e.name(), int(e.mib()), e == std::text_encoding::id::UTF8);
    std::println("{}", std::text_encoding{"UTF-8"} == std::text_encoding{"csUTF8"});   // true

    for (const char* alias : std::text_encoding{std::text_encoding::id::UTF8}.aliases())
        std::print("{} ", alias);                                                      // UTF-8 csUTF8
    std::println();

    std::text_encoding made_up{"x-made-up"};
    std::println("'{}' other={}", made_up.name(), made_up.mib() == std::text_encoding::id::other);
}
```

```
utf8 mib=106 isUTF8=true
true
UTF-8 csUTF8
'x-made-up' other=true
```

### The encoding of a locale

```cpp
#include <locale>
#include <print>
#include <text_encoding>

int main() {
    std::println("{}", std::locale::classic().encoding().name());   // US-ASCII (GCC 15 macOS, GCC 16 Linux)
}
```

Like `environment()`, this needs `-lstdc++exp` with libstdc++.

## Gotchas

- **Library support is partial.** libstdc++ has it
  (`__cpp_lib_text_encoding == 202306L` in GCC 15 and 16, verified locally); libc++ in Clang 22
  has no `<text_encoding>` header at all. MSVC status not verified locally.
- **With libstdc++, `environment()`, `environment_is()` and
  `std::locale::encoding()` need `-lstdc++exp`.** Without it, GCC 15 (macOS)
  and GCC 16 (Linux) both fail to link with
  `undefined reference to std::text_encoding::environment()` (or
  `std::locale::encoding() const`). `literal()` and lookups by name or id link
  without it.
- **`environment()` is fixed at program start.** It reflects the environment
  the program was launched with, not later calls to `std::setlocale` or
  `std::locale::global` (verified: still `UTF-8` after both switched to `"C"`). For the current C++ locale ask `locale::encoding()`.
- **`literal()` describes the compiler's execution character set**, which GCC
  lets you change with `-fexec-charset`. It says nothing about wide literals,
  and `u8"…"` literals are always UTF-8.
- **Names are implementation-defined.** Use `mib()` or `operator==` for
  decisions.

## See also

- [`std::text_encoding` — cppreference](https://en.cppreference.com/w/cpp/text/text_encoding)
- [P1885 — Naming text encodings to demystify them](https://wg21.link/p1885)
- [IANA character sets registry](https://www.iana.org/assignments/character-sets/character-sets.xhtml)
- [libstdc++ C++26 status](https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.2026)
- [Localization](localization.md) · [Multibyte strings](null-terminated-multibyte-strings.md)
