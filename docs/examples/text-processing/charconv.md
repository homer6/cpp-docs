# std::to_chars / std::from_chars

> The fastest, locale-independent, allocation-free way to convert between numbers
> and text. **Header:** `<charconv>` · **Since:** C++17

## Mental model

Low-level primitives that write/read digits into a caller-provided `char` buffer.
No locale, no allocation, no exceptions — they report success through a small
result struct. These are the building blocks under `std::format` and JSON/CSV
parsers.

```
 from_chars: "  123abc"  →  reads "123", value = 123, ptr → "abc"
 to_chars:   value 255   →  writes "255" into your buffer, ptr → one past
```

## Signatures

```cpp
struct to_chars_result   { char* ptr;       std::errc ec; };
struct from_chars_result { const char* ptr; std::errc ec; };

std::to_chars  (char* first, char* last, T value, ...);          // number → text
std::from_chars(const char* first, const char* last, T& value, ...); // text → number
```

- `T` is any integer or floating-point type.
- Optional trailing args: an `int base` (integers) or a
  `std::chars_format` (floats: `scientific`, `fixed`, `hex`, `general`), plus
  precision for `to_chars` floats.

## Reading the result

- `ec == std::errc{}` → success.
- `from_chars`: `std::errc::invalid_argument` (no parse) or
  `std::errc::result_out_of_range` (overflow). `ptr` points just past the last
  consumed character — great for tokenizing.
- `to_chars`: `std::errc::value_too_large` if the buffer was too small;
  otherwise `ptr` is one past the last written character (not null-terminated).

## Examples

### Number → text

```cpp
#include <charconv>

char buf[32];
auto [ptr, ec] = std::to_chars(buf, buf + sizeof buf, 42);
if (ec == std::errc{}) {
    std::string_view text{buf, ptr};      // "42"  (note: NOT null-terminated)
}

// floats with control over format/precision:
std::to_chars(buf, buf + sizeof buf, 3.14159, std::chars_format::fixed, 2); // "3.14"
```

### Text → number (and tokenizing)

```cpp
std::string_view s = "123 456 789";
int value;
const char* p = s.data();
const char* end = s.data() + s.size();
while (p < end) {
    auto [next, ec] = std::from_chars(p, end, value);
    if (ec != std::errc{}) break;
    // use `value`
    p = next;
    while (p < end && *p == ' ') ++p;     // skip separator
}
```

## Gotchas

- **Output is not null-terminated.** Build a `string`/`string_view` from
  `[buf, ptr)`, or write the terminator yourself.
- **`from_chars` does not skip leading whitespace or a leading `+`** for
  integers (it does accept a leading `-`). You handle surrounding format.
- **Locale-independent — always.** That's the point: `"1,234"` won't parse as
  1234, and a German locale won't turn `.` into `,`. Use this for
  data interchange; use locales/`iostream` only for human display.
- **Float support landed later in some standard libraries.** `to_chars`/
  `from_chars` for floating point was the last piece implementations shipped;
  on older toolchains integer support may be present but float missing.

## See also

- [std::to_chars — cppreference](https://en.cppreference.com/w/cpp/utility/to_chars)
- [std::from_chars — cppreference](https://en.cppreference.com/w/cpp/utility/from_chars)
- [std::chars_format — cppreference](https://en.cppreference.com/w/cpp/utility/chars_format)
