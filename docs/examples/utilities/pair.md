# std::pair

> Two values of possibly different types, bundled together.
> **Header:** `<utility>` · **Since:** C++98

## Mental model

A struct with exactly two public members, `first` and `second`. It's the
2-element case of [`tuple`](tuple.md), and it's what `map`/`unordered_map`
iteration yields (`pair<const Key, T>`).

## Examples

```cpp
#include <utility>

std::pair<int, std::string> p{1, "one"};
p.first;                        // 1
p.second;                       // "one"

auto q = std::make_pair(2, 3.5);          // deduces pair<int, double>
std::pair r{2, 3.5};                       // CTAD, same thing (C++17)

auto [n, name] = p;             // structured bindings (C++17)
```

### As function return / map element

```cpp
std::pair<bool, int> parse(std::string_view);   // (ok?, value)

std::map<std::string, int> m;
auto [it, inserted] = m.insert({"k", 1});         // insert returns a pair
for (const auto& [key, value] : m) { /* ... */ }   // each element is a pair
```

### Comparison

`pair` compares lexicographically (`first`, then `second`) and provides
`operator<=>` (C++20), so pairs are usable as map/set keys and sortable out of
the box.

```cpp
std::pair{1, 2} < std::pair{1, 3};       // true
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
