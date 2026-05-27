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

## Examples

```cpp
#include <bitset>

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
```

## Gotchas

- **Size is a compile-time constant.** For a runtime-sized bit array, use
  `std::vector<bool>` (bit-packed but a quirky proxy type) or a third-party
  dynamic_bitset.
- **Bit 0 is the least significant**, but string conversion shows the most
  significant first — `to_string()` of `bitset<4>{1}` is `"0001"`.
- **`to_ulong()`/`to_ullong()` throw `std::overflow_error`** if the value doesn't
  fit the target integer.
- **For manipulating bits of a scalar**, C++20's [`<bit>`](../numerics/README.md)
  (`std::popcount`, `std::rotl`, `std::has_single_bit`, …) is often a better fit
  than constructing a `bitset`.

## See also

- [std::bitset — cppreference](https://en.cppreference.com/w/cpp/utility/bitset)
- [`<bit>` (C++20) — cppreference](https://en.cppreference.com/w/cpp/header/bit)
