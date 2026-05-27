# I/O manipulators

> Objects you insert into a stream to change how subsequent values are formatted —
> width, precision, base, fill. **Headers:** `<ios>`, `<iomanip>`

## Mental model

A manipulator flips a stream's formatting state. Most are **sticky** (they persist
until changed) — `std::setw` is the notable exception (it applies to the next
item only).

## Common manipulators

```cpp
#include <iostream>
#include <iomanip>

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
```

## Quoting (C++14)

```cpp
std::cout << std::quoted("he said \"hi\"");    // writes: "he said \"hi\""
std::string s;
std::istringstream{"\"a b c\""} >> std::quoted(s);   // s == "a b c"  (round-trips)
```

`std::quoted` round-trips strings containing spaces/quotes through streams — handy
for simple serialization.

## Gotchas

- **`std::setw` is NOT sticky** — it resets after one insertion. The base/fill/
  justification/precision manipulators *are* sticky and silently affect later
  output until you change them back.
- **Sticky state leaks.** Setting `std::hex` or `std::setprecision` on `std::cout`
  affects every later use. Save/restore with `std::ios::fmtflags` or a guard, or
  avoid the problem entirely by using [`std::format`](../text-processing/formatting.md),
  where each `{}` spec is local.
- **`std::fixed`/`std::scientific` change what `setprecision` means** (decimal
  places vs. significant digits) — set them together intentionally.

## See also

- [I/O manipulators — cppreference](https://en.cppreference.com/w/cpp/io/manip)
- [std::setprecision — cppreference](https://en.cppreference.com/w/cpp/io/manip/setprecision)
- [std::quoted — cppreference](https://en.cppreference.com/w/cpp/io/manip/quoted)
