# Bit manipulation (`<bit>`)

> Type-safe, well-defined bit operations — counting, rotating, reinterpreting
> bits — replacing error-prone hand-rolled tricks. **Header:** `<bit>` · **Since:** C++20

## std::bit_cast — reinterpret bits, safely

The defined way to reinterpret an object's bits as another type of the same size,
**without** the UB of `reinterpret_cast` or a `union` punning trick. `constexpr`.

```cpp
#include <bit>

float  f = 1.5f;
std::uint32_t bits = std::bit_cast<std::uint32_t>(f);   // raw IEEE-754 bits
float back = std::bit_cast<float>(bits);                 // round-trips exactly
```

Both types must be trivially copyable and the same size.

## Counting and inspecting bits

```cpp
#include <bit>

void demo(unsigned x) {
    std::popcount(0b1011u);          // 3   — number of set bits
    std::countl_zero(0b0001'0000u);  // leading zero bits
    std::countr_zero(0b0001'0000u);  // trailing zero bits  (here 4)
    std::countl_one(x);  std::countr_one(x);   // leading / trailing one bits
    std::bit_width(13u);             // 4   — bits needed to represent (floor(log2)+1)
}
```

## Power-of-two helpers

```cpp
#include <bit>

static_assert(std::has_single_bit(16u));     // true  — is it a power of two?
static_assert(std::bit_ceil(17u) == 32);     // 32    — smallest power of two >= n
static_assert(std::bit_floor(17u) == 16);    // 16    — largest power of two <= n
```

Handy for sizing hash tables, buffers, and alignment.

## Rotates

```cpp
#include <bit>

void demo(std::uint32_t x) {
    std::rotl(x, 3);                 // rotate left by 3 (bits wrap around)
    std::rotr(x, 3);                 // rotate right
}
```

Real rotates — no UB from over-shifting, unlike `(x << n) | (x >> (W - n))`.

## Endianness & byte swap

```cpp
#include <bit>

void demo() {
    if constexpr (std::endian::native == std::endian::little) { /* ... */ }
    std::byteswap(std::uint16_t{0x1234});   // 0x3412 — reverse byte order (C++23)
    std::byteswap(0x1234u);                  // 0x34120000 — all 4 bytes of a 32-bit unsigned
}
```

## Gotchas

- **The counting/rotate functions require *unsigned* integer types.** Pass
  `unsigned`/`uint32_t`, not `int` — signed types are rejected.
- **`bit_cast` requires equal size and trivially-copyable types.** It's not a
  general conversion; it copies bits verbatim.
- **Prefer these over manual bit tricks.** `popcount`, `rotl`, `countr_zero` map
  to single CPU instructions and avoid the UB lurking in shift-based equivalents.
- For a fixed-size **bit array** with set/test semantics, see
  [`std::bitset`](../utilities/bitset.md); `<bit>` is for operating on the bits
  of a scalar.

## See also

- [`<bit>` header — cppreference](https://en.cppreference.com/w/cpp/header/bit)
- [std::bit_cast — cppreference](https://en.cppreference.com/w/cpp/numeric/bit_cast)
- [std::popcount — cppreference](https://en.cppreference.com/w/cpp/numeric/popcount)
- [std::endian — cppreference](https://en.cppreference.com/w/cpp/types/endian)
