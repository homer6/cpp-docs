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
std::uniform_int_distribution<int>    d1{0, 9};      // ints, inclusive
std::uniform_real_distribution<double> d2{0.0, 1.0}; // reals, [lo, hi)
std::normal_distribution<double>      d3{0.0, 1.0};  // mean, stddev
std::bernoulli_distribution           d4{0.25};      // bool, P(true)=0.25
// also: binomial, poisson, exponential, discrete_distribution, ...
```

## Examples

### Shuffle and sample

```cpp
std::mt19937 gen{std::random_device{}()};
std::shuffle(v.begin(), v.end(), gen);                       // random permutation
std::sample(in.begin(), in.end(), std::back_inserter(out), 5, gen);  // pick 5
```

### Reproducible runs

```cpp
std::mt19937 gen{12345};      // fixed seed → same sequence every run (tests/debug)
```

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

## See also

- [Pseudo-random number generation — cppreference](https://en.cppreference.com/w/cpp/numeric/random)
- [std::mt19937 — cppreference](https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine)
- [std::uniform_int_distribution — cppreference](https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution)
