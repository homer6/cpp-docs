# Enum: Enumerations — examples

> Our examples for the [Enum rules](../enum.md): named, scoped, typed sets of
> values instead of loose integers and macros.

## Enum.1: Prefer enumerations over macros

A macro is text substitution: it ignores scopes and types, can silently
change unrelated code that uses the same name, and is gone before the
debugger sees it. An enumeration is a real type with scoped names.

```cpp bad
// colors.h
#define RED   0xFF0000
#define BLUE  0x0000FF
// shirt.h
#undef BLUE
#define BLUE  2                 // BAD: another header's BLUE replaces the first
int shirt = BLUE;               // 2: probably not the colour anyone meant
```

```cpp
enum class Web_color { red = 0xFF0000, blue = 0x0000FF };
enum class Shirt { red, blue };              // GOOD: no clash, each name is scoped
Web_color page = Web_color::blue;
Shirt shirt = Shirt::blue;
```

Tested by [`tests/guidelines/enum_test.cpp`](../../../../tests/guidelines/enum_test.cpp).

## Enum.2: Use enumerations to represent sets of related named constants

Constants that belong together (states, modes, colours) should be one
enumeration: the type says they're related, a parameter of that type accepts
only them, and compilers can warn when a `switch` misses one.

```cpp bad
constexpr int idle = 0, running = 1, stopped = 2;   // BAD: just ints
void set_state(int s);
void caller() { set_state(42); }                    // compiles
```

```cpp
enum class State { idle, running, stopped };        // GOOD: one type
const char* name(State s) {
    switch (s) {                 // -Wswitch warns if a State is missing
    case State::idle:    return "idle";
    case State::running: return "running";
    case State::stopped: return "stopped";
    }
    return "?";
}
```

Tested by [`tests/guidelines/enum_test.cpp`](../../../../tests/guidelines/enum_test.cpp).

## Enum.3: Prefer class enums over "plain" enums

A plain `enum` converts to `int` silently and spills its enumerator names
into the surrounding scope, so values from unrelated enumerations can be
mixed and compared. An `enum class` does neither.

```cpp bad
enum Color { red, green };
enum Fruit { apple, banana };
void set_brightness(int level);
void caller() {
    set_brightness(green);      // BAD: a colour used as a number
    int score = banana * 10;    // and a fruit used in arithmetic
    use(score);
}
```

```cpp
enum class Color { red, green };
enum class Fruit { apple, banana };
void set_brightness(int level);
void caller() {
    set_brightness(static_cast<int>(Color::green));   // GOOD: a deliberate conversion
}
```

With `enum class`, the mistake doesn't compile:

```cpp compile-fail="cannot convert|no viable conversion|no matching function|could not convert"
enum class Color { red, green };
void set_brightness(int level);
void caller() { set_brightness(Color::green); }       // error: no implicit conversion to int
```

Tested by [`tests/guidelines/enum_test.cpp`](../../../../tests/guidelines/enum_test.cpp).

## Enum.4: Define operations on enumerations for safe and simple use

If code needs to step, combine or print enumeration values, write those
operations once, next to the enumeration, instead of casting at every use.
The casts then live in one reviewed place, with the edge cases handled.

```cpp bad
enum class Day { mon, tue, wed, thu, fri, sat, sun };
void caller(Day d) {
    d = static_cast<Day>(static_cast<int>(d) + 1);   // BAD: after sun, a value with no name
}
```

```cpp
#include <utility>
enum class Day { mon, tue, wed, thu, fri, sat, sun };
constexpr Day& operator++(Day& d) {                  // GOOD: wraps from sun to mon
    d = d == Day::sun ? Day::mon : static_cast<Day>(std::to_underlying(d) + 1);
    return d;
}
```

`std::to_underlying` is C++23; before it, `static_cast<std::underlying_type_t<Day>>(d)`.

Tested by [`tests/guidelines/enum_test.cpp`](../../../../tests/guidelines/enum_test.cpp).

## Enum.5: Don't use `ALL_CAPS` for enumerators

`ALL_CAPS` is the convention for macros. An enumerator spelled the same way
as a macro from some header is replaced by the preprocessor, and the
enumeration stops compiling, or worse, changes meaning.

```cpp compile-fail="expected identifier"
#define RED 0xFF0000            // from some header
enum class Color { RED, GREEN, BLUE };   // BAD: becomes { 0xFF0000, GREEN, BLUE }
```

```cpp
#define RED 0xFF0000
enum class Color { red, green, blue };   // GOOD: no clash with macros
```

Tested by [`tests/guidelines/enum_test.cpp`](../../../../tests/guidelines/enum_test.cpp).

## Enum.6: Avoid unnamed enumerations

An unnamed `enum` used only to get integer constants groups values that
aren't related, and gives them an anonymous enumeration type instead of the
type you meant. Use `constexpr` variables with the right types.

```cpp bad
enum { buffer_size = 4096, max_users = 8, verbose = 1 };   // BAD: unrelated values
```

```cpp
#include <cstddef>
constexpr std::size_t buffer_size = 4096;   // GOOD: each with its own type
constexpr int max_users = 8;
constexpr bool verbose = true;
```

Tested by [`tests/guidelines/enum_test.cpp`](../../../../tests/guidelines/enum_test.cpp).

## Enum.7: Specify the underlying type of an enumeration only when necessary

The default is simplest (an `enum class` is `int` underneath). Name the type
only when it matters: to save space, to fix the size for a file format or
bit flags, or to forward-declare a plain `enum`.

```cpp bad
#include <cstdint>
enum class Weekday : std::int32_t { mon, tue, wed };   // BAD: compiles, but says nothing new
```

```cpp
#include <cstdint>
enum class Weekday { mon, tue, wed };                   // GOOD: the default
enum class Direction : std::uint8_t { n, e, s, w };     // GOOD: stored in one byte
enum Permissions : std::uint32_t;                       // GOOD: needed to forward-declare a plain enum
void check(Permissions p);
enum Permissions : std::uint32_t { can_read = 1, can_write = 2 };
```

Tested by [`tests/guidelines/enum_test.cpp`](../../../../tests/guidelines/enum_test.cpp).

## Enum.8: Specify enumerator values only when necessary

Without explicit values, enumerators count up from zero with no gaps and no
duplicates. Write values only when they mean something: a conventional
numbering, bit flags, or values fixed by a protocol.

```cpp bad
enum class Level { low = 1, medium = 2, high = 2 };    // BAD: a typo makes two levels equal
```

```cpp
enum class Level { low, medium, high };                // GOOD: 0, 1, 2
enum class Month { jan = 1, feb, mar, apr, may, jun,
                   jul, aug, sep, oct, nov, dec };     // GOOD: conventional numbering
enum class Open : unsigned { read = 1, write = 1 << 1, append = 1 << 2 };   // GOOD: bit flags
```

Tested by [`tests/guidelines/enum_test.cpp`](../../../../tests/guidelines/enum_test.cpp).
