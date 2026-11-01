# String streams

> Use the stream interface to build or parse `std::string`s in memory — a stream
> whose "device" is a string buffer. **Header:** `<sstream>` (`<spanstream>` for fixed buffers, C++23) · **Since:** C++98

## The three types

| Type | Use |
|---|---|
| `std::ostringstream` | build a string with `<<` |
| `std::istringstream` | parse a string with `>>` |
| `std::stringstream` | both |

## Building strings

```cpp
#include <sstream>

std::string build() {
    std::ostringstream oss;
    oss << "x=" << 42 << ", y=" << 3.14;
    return oss.str();                      // "x=42, y=3.14"
}
```

> In modern C++, [`std::format`](../text-processing/formatting.md) usually does
> this more clearly and faster: `std::format("x={}, y={}", 42, 3.14)`. Reach for
> `ostringstream` when you need stream semantics (custom `operator<<`, manipulator
> state) across many appends.

## Parsing / tokenizing

```cpp
#include <sstream>

void parse(const std::string& userInput) {
    std::istringstream iss{"10 20 30"};
    int a, b, c;
    iss >> a >> b >> c;                    // 10, 20, 30

    // split a line into words:
    std::istringstream line{"the quick brown fox"};
    std::vector<std::string> words{std::istream_iterator<std::string>{line},
                                   std::istream_iterator<std::string>{}};

    // parse with the same failure-checking as any stream:
    std::istringstream in{userInput};
    int value;
    if (in >> value) { /* parsed */ }
}
```

## Type conversion via streams (legacy)

```cpp body
int n;
std::istringstream{"123"} >> n;            // string → int (pre-C++17 idiom)
```

Today prefer [`std::from_chars`](../text-processing/charconv.md) (fast,
locale-independent) or `std::stoi` for this.

## Fixed buffers: `std::spanstream` (C++23)

`<spanstream>` gives the same three shapes — `std::ospanstream`,
`std::ispanstream`, `std::spanstream` — over a `std::span<char>` you own instead
of a `std::string` the stream owns. Nothing is allocated, which suits stack
buffers and embedded code. GCC 15 and 16 ship it; Clang 22's libc++ doesn't yet.

```cpp
#include <spanstream>

void fixed_buffer() {
    char buf[16];
    std::ospanstream out{std::span{buf}};
    out << "x=" << 42;
    std::span<char> written = out.span();  // the 4 chars written: "x=42"

    out << " and far more than sixteen chars";
    out.fail();                            // true: the buffer is full, it never grows
    out.span().size();                     // 16

    std::ispanstream in{std::string_view{"10 20 30"}};
    int a, b, c;
    in >> a >> b >> c;                     // 10, 20, 30, without copying the text
}
```

`std::strstream` (and `istrstream`, `ostrstream`), the `char*` streams from
`<strstream>`, were deprecated in C++98 and removed in C++26
([P2867](https://wg21.link/P2867R2)): use `spanstream` for a fixed buffer or a
string stream for a growing one.

## Gotchas

- **`str()` returns a *copy*.** Holding `oss.str().c_str()` dangles — store the
  `std::string` first. (C++20 added a `str() &&` move overload to reclaim the
  buffer.)
- **Reusing a stream:** call `.str("")` to reset contents and `.clear()` to reset
  state flags — both, or stale data/flags bite you.
- **It's locale-affected and relatively slow.** For hot parsing/formatting paths
  use `charconv`/`format`; string streams shine for flexible, stream-style code.
- **`>>` stops at whitespace.** To read a phrase, use `std::getline(iss, s)`.
- **A full `ospanstream` sets `failbit` and drops the rest.** Check the stream
  after writing; `span()` holds only what fit.

## See also

- [std::basic_stringstream — cppreference](https://en.cppreference.com/w/cpp/io/basic_stringstream)
- [std::basic_ostringstream::str — cppreference](https://en.cppreference.com/w/cpp/io/basic_ostringstream/str)
- [std::basic_spanstream — cppreference](https://en.cppreference.com/w/cpp/io/basic_spanstream)
- [std::strstream — cppreference](https://en.cppreference.com/w/cpp/io/strstream)
- [P0448R4: A strstream replacement using span\<charT\> as buffer](https://wg21.link/P0448R4)
- [P2867R2: Remove Deprecated strstreams From C++26](https://wg21.link/P2867R2)
