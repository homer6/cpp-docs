# Literals

> How to write constant values in source, and how to define your own suffixes.

## Numeric literals

```cpp
42        // int
42u  42U  // unsigned
42L  42LL // long / long long
0x2A      // hex (42)
0b1010    // binary (C++14)
052       // octal (leading 0!) = 42
3.14      // double
3.14f     // float
3.14L     // long double
1'000'000 // digit separators (C++14) — ignored, just readability
1e9  6.022e23
```

## Character & string literals

```cpp
'A'        // char
u8'A'      // char8_t (C++20)
L"wide"    // const wchar_t[]
u8"utf8"   // const char8_t[]  (C++20)
u"utf16"   // const char16_t[]
U"utf32"   // const char32_t[]

R"(raw \n no escapes)"        // raw string: backslashes are literal
R"delim(can contain )" here)delim"   // custom delimiter when ")" appears
```

Adjacent string literals concatenate: `"foo" "bar"` is `"foobar"`.

## Standard library suffix literals

Opt in with the right `using namespace`:

```cpp
using namespace std::string_literals;     // "..."s  → std::string
using namespace std::chrono_literals;     // 100ms, 2h → durations
using namespace std::complex_literals;    // 1i → std::complex

auto s = "hello"s;        // std::string, not const char*
auto t = 500ms;            // std::chrono::milliseconds
```

## User-defined literals (UDLs)

Define your own suffix by writing `operator""`:

```cpp
constexpr long double operator""_km(long double v) { return v * 1000.0; }  // → meters
auto d = 5.0_km;          // 5000.0

// the standard library uses this mechanism for s, ms, i, etc.
```

(Suffixes not starting with `_` are reserved for the standard library.)

## Gotchas

- **A leading `0` means octal**: `int x = 012;` is 10, not 12. Surprising in
  zero-padded constants.
- **`"abc"` is `const char[4]`** (includes `'\0'`) and decays to `const char*` —
  not a `std::string`. Use the `""s` literal when you want a `std::string`.
- **Raw strings `R"(...)"` skip escapes** — ideal for regex and Windows paths
  (`R"(C:\temp)"`); use a custom delimiter if the body contains `)"`.
- **UDL suffixes must start with `_`** (user space); unprefixed suffixes belong to
  the standard library.

## See also

- [Integer / floating / string literals — cppreference](https://en.cppreference.com/w/cpp/language/integer_literal)
- [User-defined literals — cppreference](https://en.cppreference.com/w/cpp/language/user_literal)
