# std::random

> Proper random number generation: separate the **engine** (source of bits) from
> the **distribution** (shape of the output). **Header:** `<random>` · **Since:** C++11

## Mental model

Two pieces you compose:

1. A **seed source** + an **engine** produce raw pseudo-random bits.
2. A **distribution** maps those bits onto the numbers you actually want (a die
   roll, a normal sample, etc.).

```
 seed → engine (mt19937) → raw bits → distribution (uniform_int<1..6>) → 4
```

This separation is why `<random>` replaces `rand() % n` — the latter is biased,
low-quality, and not thread-safe.

## The standard recipe

```cpp
#include <random>

std::random_device rd;                       // non-deterministic seed source
std::mt19937 gen{rd()};                       // Mersenne Twister engine, seeded
std::uniform_int_distribution<int> die{1, 6}; // inclusive range [1, 6]

int roll = die(gen);                          // call distribution with the engine
```

Reuse `gen` and the distribution; don't recreate them per call.

## Engines & distributions

**Engines:** `std::mt19937` (32-bit, the common default), `std::mt19937_64`,
`std::minstd_rand` (small/fast LCG), `std::default_random_engine`
(implementation's pick).

**Distributions:**
```cpp
#include <random>

std::uniform_int_distribution<int>    d1{0, 9};      // ints, inclusive
std::uniform_real_distribution<double> d2{0.0, 1.0}; // reals, [lo, hi)
std::normal_distribution<double>      d3{0.0, 1.0};  // mean, stddev
std::bernoulli_distribution           d4{0.25};      // bool, P(true)=0.25
// also: binomial, poisson, exponential, discrete_distribution, ...
```

## Examples

### Shuffle and sample

```cpp
#include <algorithm>
#include <random>

void demo(std::vector<int>& v, const std::vector<int>& in, std::vector<int>& out) {
    std::mt19937 gen{std::random_device{}()};
    std::shuffle(v.begin(), v.end(), gen);                       // random permutation
    std::sample(in.begin(), in.end(), std::back_inserter(out), 5, gen);  // pick 5
}
```

### Reproducible runs

```cpp
#include <random>

std::mt19937 gen{12345};      // fixed seed → same sequence every run (tests/debug)
```

### Counter-based streams: `std::philox_engine` (C++26)

`philox_engine` (P2075) is a **counter-based** engine: each block of `n`
outputs is a pure function of a key (from the seed) and a counter. Jumping
anywhere in the sequence is just setting the counter, so parallel workers can
each take their own stream from one seed, without sharing state.
`std::philox4x32` and `std::philox4x64` are the predefined engines (4 words,
10 rounds); the state is a few words (88 bytes in libstdc++), against 5000 for
`mt19937`.

```cpp needs=philox
#include <random>

int main() {
    std::philox4x32 gen{42};                 // a normal engine: works with every distribution
    std::uniform_int_distribution<int> die{1, 6};
    int roll = die(gen);

    std::philox4x32 a{42}, b{42};
    a.discard(4 * 1000);                     // skip 1000 blocks of 4 outputs...
    b.set_counter({0, 0, 0, 1000});          // ...or jump there: last element is the low word
    bool same = a() == b();                  // true

    std::philox4x32 worker1{42};
    worker1.set_counter({0, 1, 0, 0});       // a disjoint stream for a second worker
}
```

GCC 16 has it; GCC 15 and Clang 22 (libc++) don't (`__cpp_lib_philox_engine`,
`202406L`).

## Gotchas

- **Don't use `rand() % n`.** It's biased toward low values, low quality, and
  global. `<random>` engines + distributions fix all three.
- **`std::random_device` may be weak or deterministic** on some platforms/older
  libstdc++ (notably MinGW historically returned a constant!). For
  cryptographic randomness, use an OS/crypto API, not `<random>` — it's designed
  for simulation, not security.
- **Seed with enough entropy.** A single 32-bit `rd()` under-seeds `mt19937`
  (which has a huge state); for serious work seed via a `std::seed_seq` from
  several `rd()` values.
- **Engines and distributions are stateful and not thread-safe.** Give each
  thread its own `gen`, or synchronize.
- **Distribution ranges differ:** `uniform_int_distribution{a,b}` is **inclusive**
  of `b`; `uniform_real_distribution{a,b}` is `[a, b)`.
- **`set_counter` takes the counter most-significant word first.** For
  `philox4x32`, `set_counter({0, 0, 0, k})` is block `k`; `{k, 0, 0, 0}` is a
  very different place. It also discards any buffered outputs.

## See also

- [Pseudo-random number generation — cppreference](https://en.cppreference.com/w/cpp/numeric/random)
- [std::mt19937 — cppreference](https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine)
- [std::uniform_int_distribution — cppreference](https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution)
- [std::philox_engine — cppreference](https://en.cppreference.com/w/cpp/numeric/random/philox_engine)
- [P2075R6: Philox as an extension of the C++ RNG engines](https://wg21.link/P2075R6)
- [[rand.eng.philox] in the draft](https://eel.is/c++draft/rand.eng.philox)
