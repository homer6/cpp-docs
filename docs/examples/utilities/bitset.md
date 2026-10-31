# std::bitset

> A fixed-size sequence of bits with convenient set/test/count operations and
> bitwise operators. **Header:** `<bitset>` · **Since:** C++98

## Mental model

`bitset<N>` is N bits packed into words, with a friendly interface: index like an
array, do bitwise math like an integer, and convert to/from strings and numbers.
The size N is fixed at compile time.

```
 std::bitset<8> b{0b1011};   →   0 0 0 0 1 0 1 1
                                 7 6 5 4 3 2 1 0   ← bit index (0 = least significant)
```

## Declaration

```cpp
template<std::size_t N> class bitset;
```

| Member | Meaning |
|---|---|
| `bitset(unsigned long long)` | Low N bits of the value; extra bits are dropped |
| `bitset(str, pos, n, zero = '0', one = '1')` | From a `string` / `const char*` of `'0'`/`'1'` characters |
| `operator[]` | Unchecked access (returns a proxy `reference` for non-const) |
| `test`, `set`, `reset`, `flip` | Checked by position — throw `std::out_of_range` |
| `count`, `size`, `all`, `any`, `none` | Popcount, N, predicates |
| `& \| ^ ~ << >>` (and `&=` …) | Bitwise ops between same-size bitsets |
| `to_ulong`, `to_ullong`, `to_string` | Conversions |

Most members are `constexpr` since C++23 (`__cpp_lib_constexpr_bitset`). A
`std::hash<bitset<N>>` specialization exists, so bitsets work as
`unordered_set` keys.

## When to use it

- **Fits:** a fixed set of flags, a set over a small known range (`0..N-1`),
  sieves and bitmaps whose size is a compile-time constant.
- **Anti-fits:** a size known only at run time (`std::vector<bool>` or a
  dynamic bitset library), and bit tricks on one integer (`<bit>`).

## Examples

```cpp
#include <bitset>

void bitset_basics() {
    std::bitset<8> b{0b1011};        // from an integer
    std::bitset<8> c{"00001011"};    // from a string

    b[0];                            // true  (bit 0 set)
    b.test(2);                       // false (bounds-checked; [] is not)
    b.set(4);                        // set bit 4
    b.reset(0);                      // clear bit 0
    b.flip();                        // invert all bits
    b.count();                       // number of set bits  (popcount)
    b.size();                        // 8
    b.any(); b.all(); b.none();      // predicates

    b |= std::bitset<8>{0b0100};     // bitwise ops work between same-size bitsets
    auto n  = b.to_ulong();          // → unsigned long  (throws if it doesn't fit)
    auto s  = b.to_string();         // → "0001..." 
}
```

### Flags from an enum

```cpp
#include <bitset>

enum class Perm { read, write, exec, count_ };

using Perms = std::bitset<static_cast<std::size_t>(Perm::count_)>;

constexpr Perms with(Perms p, Perm f) { return p.set(static_cast<std::size_t>(f)); }

constexpr Perms rw = with(with(Perms{}, Perm::read), Perm::write);
static_assert(rw.count() == 2 && !rw.test(2));   // constexpr since C++23
```

## Gotchas

- **Size is a compile-time constant.** For a runtime-sized bit array, use
  `std::vector<bool>` (bit-packed but a quirky proxy type) or a third-party
  dynamic_bitset.
- **Bit 0 is the least significant**, but string conversion shows the most
  significant first — `to_string()` of `bitset<4>{1}` is `"0001"`.
- **`to_ulong()`/`to_ullong()` throw `std::overflow_error`** if the value doesn't
  fit the target integer.
- **`operator[]` isn't checked; `test()` is.** An out-of-range `b[i]` is
  undefined behaviour, while `b.test(i)` throws `std::out_of_range`.
- **The string constructor throws `std::invalid_argument`** on any character
  other than the zero/one characters, and a short string fills the low bits:
  `bitset<8>{"1010"}` is `00001010`.
- **For manipulating bits of a scalar**, C++20's [`<bit>`](../numerics/README.md)
  (`std::popcount`, `std::rotl`, `std::has_single_bit`, …) is often a better fit
  than constructing a `bitset`.

## See also

- [std::bitset — cppreference](https://en.cppreference.com/w/cpp/utility/bitset)
- [`<bit>` (C++20) — cppreference](https://en.cppreference.com/w/cpp/header/bit)
- [std::vector\<bool\> — cppreference](https://en.cppreference.com/w/cpp/container/vector_bool)
