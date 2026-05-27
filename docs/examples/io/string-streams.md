# String streams

> Use the stream interface to build or parse `std::string`s in memory — a stream
> whose "device" is a string buffer. **Header:** `<sstream>` · **Since:** C++98

## The three types

| Type | Use |
|---|---|
| `std::ostringstream` | build a string with `<<` |
| `std::istringstream` | parse a string with `>>` |
| `std::stringstream` | both |

## Building strings

```cpp
#include <sstream>

std::ostringstream oss;
oss << "x=" << 42 << ", y=" << 3.14;
std::string s = oss.str();                 // "x=42, y=3.14"
```

> In modern C++, [`std::format`](../text-processing/formatting.md) usually does
> this more clearly and faster: `std::format("x={}, y={}", 42, 3.14)`. Reach for
> `ostringstream` when you need stream semantics (custom `operator<<`, manipulator
> state) across many appends.

## Parsing / tokenizing

```cpp
std::istringstream iss{"10 20 30"};
int a, b, c;
iss >> a >> b >> c;                        // 10, 20, 30

// split a line into words:
std::istringstream line{"the quick brown fox"};
std::vector<std::string> words{std::istream_iterator<std::string>{line},
                               std::istream_iterator<std::string>{}};

// parse with the same failure-checking as any stream:
std::istringstream in{userInput};
int value;
if (in >> value) { /* parsed */ }
```

## Type conversion via streams (legacy)

```cpp
int n;
std::istringstream{"123"} >> n;            // string → int (pre-C++17 idiom)
```

Today prefer [`std::from_chars`](../text-processing/charconv.md) (fast,
locale-independent) or `std::stoi` for this.

## Gotchas

- **`str()` returns a *copy*.** Holding `oss.str().c_str()` dangles — store the
  `std::string` first. (C++20 added a `str() &&` move overload to reclaim the
  buffer.)
- **Reusing a stream:** call `.str("")` to reset contents and `.clear()` to reset
  state flags — both, or stale data/flags bite you.
- **It's locale-affected and relatively slow.** For hot parsing/formatting paths
  use `charconv`/`format`; string streams shine for flexible, stream-style code.
- **`>>` stops at whitespace.** To read a phrase, use `std::getline(iss, s)`.

## See also

- [std::basic_stringstream — cppreference](https://en.cppreference.com/w/cpp/io/basic_stringstream)
- [std::basic_ostringstream::str — cppreference](https://en.cppreference.com/w/cpp/io/basic_ostringstream/str)
