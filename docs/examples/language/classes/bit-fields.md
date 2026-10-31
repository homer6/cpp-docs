# Bit-fields

> A data member declared with an explicit width in bits, `unsigned flag : 1;`,
> so several small fields can share storage. **Since:** C++98 · default member
> initializers since C++20

## Mental model

The compiler packs consecutive bit-fields into *allocation units* of their
declared type, but **almost every layout detail is implementation-defined**:
the order of bits within a unit, whether a field may straddle two units, and
alignment of the unit.

```
struct Flags { unsigned ready:1, dirty:1, mode:3; unsigned :0; unsigned counter:4; };

one possible layout (GCC/Clang, Itanium ABI, little-endian):
unit 0: [ready][dirty][mode mode mode][........ 27 unused bits ........]
unit 1: [counter x4][................ 28 unused bits ...................]
                ↑ ":0" forces counter into a new unit           sizeof == 8
```

## Declaration

```cpp nocompile
type name : width ;                       // width: integral constant expression
type name : width = initializer ;         // C++20: default member initializer
type name : width { initializer } ;       // C++20
type      : width ;                       // unnamed: padding
type      : 0 ;                           // unnamed, zero width: start a new allocation unit
```

- `type` is an integral or (scoped or unscoped) enumeration type, or `bool`.
- `width` may exceed the number of bits in `type`; the extra bits are padding.
- Unnamed bit-fields aren't members: they're skipped by aggregate
  initialization and can't have initializers.
- A field holds values in the range of a `width`-bit integer of the field's
  signedness. Since C++20, assigning an out-of-range value keeps the low
  `width` bits (two's-complement wraparound, also for signed fields).

## When to use it

- **Compact flag sets and small counters** inside objects you keep millions of,
  where the memory saving outweighs slower read-modify-write access.
- **Internal packing** where the exact bit positions don't matter.
- Anti-fit: **hardware registers, file formats, network headers** — the layout
  is implementation-defined, so portable code uses shifts and masks on a
  fixed-width integer (or [`std::bitset`](../../utilities/bitset.md) for a set of
  flags).
- Anti-fit: fields that different threads write — neighbouring bit-fields share
  a memory location (see Gotchas).

## Examples

### Flags with default member initializers (C++20)

```cpp
#include <iostream>

struct Flags {
    unsigned ready   : 1 = 0;
    unsigned dirty   : 1 = 1;
    unsigned mode    : 3 {5};
    unsigned         : 0;          // counter starts a new unit
    unsigned counter : 4 = 0;
};

int main() {
    Flags f;
    std::cout << f.ready << f.dirty << f.mode << f.counter << '\n';  // 0150
    std::cout << sizeof(Flags) << '\n';                              // 8 with GCC and Clang on arm64 macOS
}
```

Before C++20 a bit-field couldn't have a default member initializer; you had
to set it in every constructor.

### Wraparound on overflow

```cpp
#include <iostream>

struct Small {
    unsigned u : 4;   // 0..15
    int      s : 2;   // -2..1
};

int main() {
    Small x{};
    x.u = 17;         // keeps the low 4 bits: 1
    x.s = 3;          // 0b11 as a 2-bit signed value: -1 (C++20: guaranteed)
    std::cout << x.u << ' ' << x.s << '\n';   // 1 -1
}
```

GCC and Clang warn about constant truncation (`-Woverflow`,
`-Wbitfield-constant-conversion`); a run-time value is truncated silently.

### Enumerations in bit-fields

```cpp
enum class Color : unsigned { red, green, blue };   // fits in 2 bits

struct Pixel {
    Color    c     : 2;
    unsigned alpha : 6;
};

static_assert(Pixel{Color::blue, 63}.c == Color::blue);
```

Give the enum an **unsigned** underlying type, or a signed one with an extra
bit: with `enum class Color : int`, a 2-bit field holds only −2..1, so storing
`blue` (2) reads back as −2 and `p.c == Color::blue` is `false`.

### `[[no_unique_address]]` is a different tool

A bit-field shrinks *integers* to a few bits. `[[no_unique_address]]` (C++20) lets
an *empty* member (a stateless allocator, comparator, or policy) take no space
at all:

```cpp
struct Empty {};

struct WithEmpty { int i; Empty e; };                         // e needs its own byte → padded
struct WithNUA   { int i; [[no_unique_address]] Empty e; };   // e may overlap i

// Results on GCC and Clang (Itanium ABI):
static_assert(sizeof(WithEmpty) == 2 * sizeof(int));
static_assert(sizeof(WithNUA)   == sizeof(int));
```

An implementation is allowed to ignore the attribute. cppreference notes that
MSVC does (for ABI stability) and provides `[[msvc::no_unique_address]]`
instead — not checked locally.

## Gotchas

- **No address, no non-const reference.** `&f.ready` and `unsigned& r = f.ready`
  don't compile; a `const unsigned&` binds to a *temporary copy*, which doesn't
  track later writes. So no pointers, no `std::atomic` bit-fields, and no
  `std::span`/`std::ref` over one. (Structured bindings *can* name bit-fields:
  `auto& [a, b] = flags;` binds `a` to the bit-field itself.)

  ```cpp compile-fail="bit-field"
  struct F { unsigned a : 1; };
  unsigned* p(F& f) { return &f.a; }   // error: address of bit-field
  ```

- **Promotion surprises.** In arithmetic, a bit-field narrower than `int`
  promotes to `int` even if it is declared `unsigned` (`+b.x` is an `int` for
  `unsigned x : 4`). `auto y = b.x;` deduces the declared type, `unsigned`.
- **Adjacent bit-fields are one memory location.** Two threads writing `ready`
  and `dirty` concurrently is a data race, even though they're different members.
  A zero-width bit-field (`unsigned : 0;`) separates memory locations.
- **Layout isn't portable.** Bit order, straddling, and the size of the
  enclosing struct differ between ABIs (the GCC/Clang numbers on this page are
  Itanium-ABI results; MSVC packs differently). Don't `memcpy` bit-field structs to the
  wire or overlay them on device registers in portable code.
- **Reads and writes cost more.** Each access is a load, shift, mask (and a store
  for writes) of the whole unit. Measure before packing hot data.
- **`sizeof` and `offsetof`** can't be applied to a bit-field member.

## See also

- [Bit-field — cppreference](https://en.cppreference.com/w/cpp/language/bit_field)
- [Non-static data members (default member initializers) — cppreference](https://en.cppreference.com/w/cpp/language/data_members)
- [`[[no_unique_address]]` — cppreference](https://en.cppreference.com/w/cpp/language/attributes/no_unique_address)
- [Memory model (memory locations) — cppreference](https://en.cppreference.com/w/cpp/language/memory_model)
- [\[class.bit\] Bit-fields — C++ draft](https://eel.is/c++draft/class.bit)
- [P0683R1 — Default member initializers for bit-fields](https://wg21.link/p0683r1)
- In this repo: [Members & access](members-access.md),
  [Attributes](../declarations-initialization/attributes.md),
  [Fundamental types](../basic-concepts/fundamental-types.md)
