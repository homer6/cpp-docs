# I/O manipulators

> Objects you insert into a stream to change how subsequent values are formatted —
> width, precision, base, fill. **Headers:** `<ios>`, `<iomanip>`

## Mental model

A manipulator flips a stream's formatting state. Most are **sticky** (they persist
until changed) — `std::setw` is the notable exception (it applies to the next
item only).

```
 cout << hex << 255 << ' ' << setw(4) << 1 << 2;   →  "ff    12"
         └──── sticky: hex stays on ────┘ └─ setw: next item only
```

## Declaration

Manipulators are functions (or objects returned by functions) that
`operator<<` / `operator>>` apply to the stream:

| Manipulator | Header | Effect | Sticky? |
|---|---|---|---|
| `boolalpha` / `noboolalpha` | `<ios>` | `true`/`false` instead of `1`/`0` | yes |
| `dec` / `hex` / `oct` | `<ios>` | Integer base | yes |
| `showbase`, `uppercase`, `showpos` | `<ios>` | `0x` prefix, `0XFF`/`1E+00`, `+` sign | yes |
| `left` / `right` / `internal` | `<ios>` | Where padding goes (`internal`: after the sign or base) | yes |
| `fixed` / `scientific` / `hexfloat` / `defaultfloat` | `<ios>` | Floating-point notation | yes |
| `setprecision(n)` | `<iomanip>` | Significant digits, or digits after the point with `fixed`/`scientific` | yes |
| `setfill(c)` | `<iomanip>` | Padding character | yes |
| `setw(n)` | `<iomanip>` | Minimum field width | **no — next item only** |
| `quoted(s)` (C++14) | `<iomanip>` | Write/read a string in quotes, escaping embedded quotes | — |
| `skipws` / `noskipws`, `ws` | `<ios>`, `<istream>` | Input: skip leading whitespace / eat it now | yes / — |
| `endl`, `flush`, `ends` | `<ostream>` | Newline + flush, flush, write `'\0'` | — |

## When to use it

- **Fits:** existing iostream code, `std::istream` parsing (`std::hex`,
  `std::quoted`, `std::noskipws`), and quick formatting in a local
  `std::ostringstream` that nobody else shares.
- **Anti-fits:** new output code — [`std::format` / `std::print`](../text-processing/formatting.md)
  keep each `{}` spec local, so there's no state to leak.

## Examples

### Common manipulators

Each comment shows the output on a freshly constructed stream; on one shared
stream the sticky settings accumulate (see Gotchas).

```cpp
#include <iostream>
#include <iomanip>

void demo() {
std::cout << std::boolalpha << true;          // "true" instead of "1"  (sticky)
std::cout << std::hex << 255;                  // "ff"   (sticky: dec/oct/hex)
std::cout << std::showbase << std::hex << 255; // "0xff"
std::cout << std::setw(8) << 42;               // "      42" — width 8, NEXT item only
std::cout << std::setfill('0') << std::setw(4) << 7;   // "0007"
std::cout << std::left << std::setw(8) << "hi";        // "hi      " (left-justified)

double pi = 3.14159265;
std::cout << std::fixed << std::setprecision(2) << pi; // "3.14"
std::cout << std::scientific << pi;                     // "3.141593e+00"
std::cout << std::setprecision(10) << pi;               // more digits
}
```

### Quoting (C++14)

```cpp
#include <iomanip>
#include <sstream>

void demo() {
std::cout << std::quoted("he said \"hi\"");    // writes: "he said \"hi\""
std::string s;
std::istringstream{"\"a b c\""} >> std::quoted(s);   // s == "a b c"  (round-trips)
}
```

`std::quoted` round-trips strings containing spaces/quotes through streams — handy
for simple serialization.

### Save and restore the state

```cpp
#include <iomanip>
#include <ostream>

void print_hex(std::ostream& os, int v) {
    std::ios saved(nullptr);
    saved.copyfmt(os);                         // flags, precision, width, fill
    os << std::hex << std::setfill('0') << std::setw(4) << v;
    os.copyfmt(saved);                         // caller's formatting is back
}
```

## Gotchas

- **`std::setw` is NOT sticky** — it resets after one insertion. The base/fill/
  justification/precision manipulators *are* sticky and silently affect later
  output until you change them back.
- **Sticky state leaks.** Setting `std::hex` or `std::setprecision` on `std::cout`
  affects every later use. Save/restore with `std::ios::fmtflags` or a guard, or
  avoid the problem entirely by using [`std::format`](../text-processing/formatting.md),
  where each `{}` spec is local. Run the "Common manipulators" lines one after
  another on one stream and the later ones come out as `"    0x2a"`, `"00x7"`
  and `"3.14e+00"`: the earlier `hex`, `showbase` and `setprecision(2)` are still
  in effect.
- **`std::fixed`/`std::scientific` change what `setprecision` means** (decimal
  places vs. significant digits) — set them together intentionally.
- **`setw` never truncates.** `std::setw(2) << "hello"` still prints `hello`.

## See also

- [I/O manipulators — cppreference](https://en.cppreference.com/w/cpp/io/manip)
- [std::setprecision — cppreference](https://en.cppreference.com/w/cpp/io/manip/setprecision)
- [std::quoted — cppreference](https://en.cppreference.com/w/cpp/io/manip/quoted)
- [std::ios_base::fmtflags — cppreference](https://en.cppreference.com/w/cpp/io/ios_base/fmtflags)
