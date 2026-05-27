# std::integer_sequence / index_sequence

> A compile-time pack of integers, used to "unpack" tuples and arrays by
> generating `0, 1, …, N-1` as a parameter pack. **Header:** `<utility>` ·
> **Since:** C++14

## Mental model

`std::index_sequence<0, 1, 2>` is a type carrying a pack of indices. You generate
it with `std::make_index_sequence<N>`, then deduce the indices into a helper so
you can write `things[Is]...` / `get<Is>(t)...` — turning a count into an
expandable pack.

```cpp
std::make_index_sequence<3>     // → std::index_sequence<0, 1, 2>
```

## The canonical pattern: call a function with tuple elements

(This is what `std::apply` does internally.)

```cpp
#include <utility>
#include <tuple>

template<class F, class Tuple, std::size_t... Is>
auto apply_impl(F&& f, Tuple&& t, std::index_sequence<Is...>) {
    return std::forward<F>(f)(std::get<Is>(std::forward<Tuple>(t))...);  // expand!
}

template<class F, class Tuple>
auto my_apply(F&& f, Tuple&& t) {
    constexpr auto n = std::tuple_size_v<std::remove_cvref_t<Tuple>>;
    return apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
                      std::make_index_sequence<n>{});
}

my_apply([](int a, int b, int c){ return a+b+c; }, std::tuple{1,2,3});  // 6
```

The `std::get<Is>(t)...` expansion is the whole point: `Is` becomes `0,1,2`, so
the call is `f(get<0>(t), get<1>(t), get<2>(t))`.

## Building an array from a generator

```cpp
template<std::size_t... Is>
constexpr auto squares_impl(std::index_sequence<Is...>) {
    return std::array{ (Is * Is)... };       // {0, 1, 4, 9, ...}
}
constexpr auto squares = squares_impl(std::make_index_sequence<5>{});  // {0,1,4,9,16}
```

## Gotchas

- **It exists to drive pack expansion, not to be iterated.** You consume the
  `Is...` in a single `...` expansion; there's no runtime loop.
- **Prefer the ready-made tools first.** `std::apply` (call with a tuple) and
  `std::make_from_tuple` (construct from a tuple) cover the common cases — reach
  for `index_sequence` only when writing your own such helper.
- **`std::integer_sequence<T, ...>`** is the general form (any integer type);
  `index_sequence` is the `std::size_t` specialization you'll use 99% of the time.

## See also

- [std::integer_sequence — cppreference](https://en.cppreference.com/w/cpp/utility/integer_sequence)
- [std::apply — cppreference](https://en.cppreference.com/w/cpp/utility/apply)
