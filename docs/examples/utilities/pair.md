# std::pair

> Two values of possibly different types, bundled together.
> **Header:** `<utility>` · **Since:** C++98

## Mental model

A struct with exactly two public members, `first` and `second`. It's the
2-element case of [`tuple`](tuple.md), and it's what `map`/`unordered_map`
iteration yields (`pair<const Key, T>`).

## Declaration

```cpp
template<class T1, class T2> struct pair;   // { T1 first; T2 second; }
```

| Piece | Meaning |
|---|---|
| `first`, `second` | The two public data members |
| `pair(a, b)`, `pair(piecewise_construct, tuple, tuple)` | Construct from values, or build each member in place from argument tuples |
| `make_pair(a, b)` | Factory; decays argument types (and unwraps `std::ref`) |
| `== <=>` | Lexicographic comparison, `first` then `second` (`<=>` since C++20) |
| `swap`, `std::get<0>`/`get<1>`/`get<T>` | Swap; tuple-style access |
| `tuple_size`, `tuple_element` | Make it work with structured bindings and `std::apply` |

## When to use it

- **Fits:** generic code and library interop — map elements, `insert`
  results, `std::minmax`, `std::equal_range`, two values returned from a
  template.
- **Anti-fits:** domain data whose members have meaning — a named `struct`
  documents itself and still supports structured bindings. More than two values
  → [`tuple`](tuple.md) (or, again, a struct).

## Examples

```cpp body
#include <utility>

std::pair<int, std::string> p{1, "one"};
p.first;                        // 1
p.second;                       // "one"

auto q = std::make_pair(2, 3.5);          // deduces pair<int, double>
std::pair r{2, 3.5};                       // CTAD, same thing (C++17)

auto [n, name] = p;             // structured bindings (C++17)
```

### As function return / map element

```cpp body
std::pair<bool, int> parse(std::string_view);   // (ok?, value)

std::map<std::string, int> m;
auto [it, inserted] = m.insert({"k", 1});         // insert returns a pair
for (const auto& [key, value] : m) { /* ... */ }   // each element is a pair
```

### Comparison

`pair` compares lexicographically (`first`, then `second`) and provides
`operator<=>` (C++20), so pairs are usable as map/set keys and sortable out of
the box.

```cpp body
std::pair{1, 2} < std::pair{1, 3};       // true
```

### Library algorithms that return one

```cpp body
#include <algorithm>

auto [lo, hi] = std::minmax({4, 1, 7});  // pair<int, int>{1, 7}
```

## Gotchas

- **Prefer a named `struct` when the members mean something.** `first`/`second`
  are anonymous; `struct Size { int w, h; };` reads far better than
  `pair<int,int>`. Reserve `pair` for genuinely generic code and library
  interop.
- **`make_pair` decays and is mostly obsolete.** Since C++17, CTAD (`std::pair{a,
  b}`) does the same with clearer types — though `make_pair` still applies
  reference-decay rules you occasionally want.
- **The map key is `const`:** elements are `pair<const Key, T>`, so you can't
  reassign `.first` through an iterator.

## See also

- [std::pair — cppreference](https://en.cppreference.com/w/cpp/utility/pair)
- [std::tuple — cppreference](https://en.cppreference.com/w/cpp/utility/tuple)
- [std::make_pair — cppreference](https://en.cppreference.com/w/cpp/utility/pair/make_pair)
