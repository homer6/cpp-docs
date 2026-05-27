# Numerics library

Random numbers, complex arithmetic, math functions and constants, bit
manipulation, and (C++26) data-parallel & linear-algebra facilities. Mirrors
[cppreference's numerics library](https://en.cppreference.com/w/cpp/numeric).

| Page | Header | Since | What |
|---|---|---|---|
| [Random](random.md) | `<random>` | C++11 | engines + distributions, done right |
| [`complex`](complex.md) | `<complex>` | C++98 | complex-number arithmetic |
| [Math functions & constants](math.md) | `<cmath>`, `<numbers>` | C++98 / C++20 | `sqrt`, `pow`, special functions, `std::numbers::pi` |
| [Bit manipulation](bit.md) | `<bit>` | C++20 | `bit_cast`, `popcount`, `rotl`, endianness |
| [`valarray`](valarray.md) | `<valarray>` | C++98 | numeric array (legacy) |
| [Data-parallel types (SIMD)](simd.md) | `<simd>` | C++26 | portable explicit vectorization |
| [Linear algebra](linalg.md) | `<linalg>` | C++26 | BLAS-style operations over `mdspan` |

> Numeric *algorithms* (`accumulate`, `reduce`, scans, `gcd`/`lcm`) live in
> [`<numeric>`](../algorithms/numeric.md) under Algorithms.
