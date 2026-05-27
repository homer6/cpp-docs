# std::format and std::print

> Type-safe, Python-style string formatting (`std::format`, C++20) and direct
> printing (`std::print` / `std::println`, C++23).
> **Headers:** `<format>`, `<print>`

## Mental model

A compile-time-checked format string with `{}` replacement fields. Arguments are
formatted by type via `std::formatter` specializations — no varargs, no `%d`
mismatches, no manual `<<` chaining.

```
 std::format("{} = {:.2f}", "pi", 3.14159)   →   "pi = 3.14"
                    │    └ format spec: fixed, 2 decimals
                    └ positional default: argument 0, then 1, ...
```

## The core functions

| Function | Since | Returns / does |
|---|---|---|
| `std::format(fmt, args...)` | C++20 | a `std::string` |
| `std::format_to(out, fmt, args...)` | C++20 | writes through an output iterator |
| `std::format_to_n(out, n, ...)` | C++20 | writes at most n chars |
| `std::formatted_size(fmt, args...)` | C++20 | chars that would be written |
| `std::vformat` | C++20 | runtime (non-`consteval`) format string |
| `std::print(fmt, args...)` | C++23 | writes to `stdout` (or a `FILE*`/stream) |
| `std::println(...)` | C++23 | like `print`, adds a newline |

The format string is checked **at compile time** in `std::format`/`std::print`;
a malformed string or wrong arg count is a compile error. Use `std::vformat`
(with `std::make_format_args`) when the string is only known at runtime.

## Format spec mini-reference

Inside `{}`: `{[index][:[[fill]align][sign][#][0][width][.precision][type]]}`

```cpp
std::format("{:>8}", "hi");     // "      hi"  right-align, width 8
std::format("{:*<8}", "hi");    // "hi******"  left-align, fill '*'
std::format("{:^8}", "hi");     // "   hi   "  center
std::format("{:+d}", 42);       // "+42"       force sign
std::format("{:08.3f}", 3.14);  // "0003.140"  zero-pad, width 8, 3 decimals
std::format("{:#x}", 255);      // "0xff"      alternate form, hex
std::format("{:b}", 5);         // "101"       binary
std::format("{0} {1} {0}", "a", "b");          // "a b a"  explicit indices
std::format("{:{}}", "x", 5);   // width taken from a nested argument
```

## Examples

```cpp
#include <format>
#include <print>

std::string s = std::format("{} items, {:.1f}% done", 3, 66.6);

std::print("Hello, {}!\n", "world");      // C++23, no trailing newline added
std::println("x = {}, y = {}", 1, 2);      // C++23, newline added

// Format into an existing buffer / container:
std::string buf;
std::format_to(std::back_inserter(buf), "[{}]", 42);
```

### Making your own type formattable

```cpp
struct Point { int x, y; };

template<>
struct std::formatter<Point> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const Point& p, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};

std::print("{}\n", Point{3, 4});           // (3, 4)
```

To also accept a nested format spec, parse it in `parse()` and forward it — or
delegate to an existing formatter (e.g. inherit from `std::formatter<int>`).

## Gotchas

- **Literal braces are doubled:** `"{{"` → `{`, `"}}"` → `}`.
- **`std::print` to a non-Unicode terminal:** on Windows it handles UTF-8 →
  console properly; mixing `std::print` with `printf`/`std::cout` can interleave
  oddly because of separate buffering — flush or pick one.
- **Compile-time check needs a literal format string.** A `std::string` format
  argument won't compile with `std::format`; route runtime strings through
  `std::vformat(str, std::make_format_args(args...))`.
- **It's locale-independent by default** (unlike `iostream`/`to_string`). Use the
  `L` spec / `std::format(std::locale, ...)` overloads when you actually want
  locale formatting.

## See also

- [std::format — cppreference](https://en.cppreference.com/w/cpp/utility/format/format)
- [std::print — cppreference](https://en.cppreference.com/w/cpp/io/print)
- [Standard format specification — cppreference](https://en.cppreference.com/w/cpp/utility/format/spec)
- [std::formatter — cppreference](https://en.cppreference.com/w/cpp/utility/format/formatter)
