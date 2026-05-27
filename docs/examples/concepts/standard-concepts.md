# Standard concepts

> The built-in concepts in `<concepts>`, plus how to apply and define your own.
> **Header:** `<concepts>` · **Since:** C++20

## The standard library concepts

**Core language / relationships:**
```cpp
std::same_as<T, U>
std::derived_from<D, B>
std::convertible_to<From, To>
std::common_with<T, U>,  std::common_reference_with<T, U>
std::assignable_from<L, R>
std::swappable<T>
```

**Type categories:**
```cpp
std::integral, std::signed_integral, std::unsigned_integral
std::floating_point
```

**Comparison:**
```cpp
std::equality_comparable<T>,  std::equality_comparable_with<T, U>
std::totally_ordered<T>,      std::totally_ordered_with<T, U>
std::three_way_comparable<T>
```

**Object / lifetime:**
```cpp
std::destructible, std::constructible_from<T, Args...>,
std::default_initializable, std::move_constructible, std::copy_constructible,
std::movable, std::copyable, std::semiregular, std::regular
```

**Callable:**
```cpp
std::invocable<F, Args...>,  std::regular_invocable<F, Args...>
std::predicate<F, Args...>,  std::relation<R, T, U>, std::strict_weak_order<R, T, U>
```

(The [iterators](../iterators/categories.md) and [ranges](../ranges/range-concepts.md)
libraries add many more: `input_iterator`, `range`, `view`, …)

## Four ways to apply a concept

```cpp
// 1. as a constrained template parameter:
template<std::integral T> void f(T);

// 2. with a requires-clause:
template<class T> requires std::integral<T> void g(T);

// 3. as a constrained `auto` (abbreviated function template):
void h(std::integral auto x);

// 4. constraining a return / variable:
std::integral auto value = compute();
```

## Writing your own

```cpp
template<class T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::same_as<T>;        // a+b must be valid AND yield T
};

template<Addable T>
T sum(T a, T b) { return a + b; }
```

A `requires` expression lists expressions that must compile; the `-> concept`
part further constrains their result type.

## Overloading by constraint

```cpp
void process(std::integral auto x);        // chosen for ints
void process(std::floating_point auto x);  // chosen for floats
```

The compiler picks the **most constrained** viable overload — cleaner than tag
dispatch or `enable_if`.

## Gotchas

- **Prefer concepts to SFINAE/`enable_if`.** Same effect, vastly better error
  messages, and natural overloading.
- **`requires requires` is legal but reads oddly** — a `requires`-clause followed
  by a `requires`-expression. Name the concept instead for clarity.
- **A concept is a compile-time predicate, not a base class** — it constrains, it
  doesn't add virtual dispatch or runtime cost.
- **`std::regular`/`semiregular`** bundle the common "behaves like a built-in
  value type" requirements — handy shorthands when designing value types.

## See also

- [Concepts library — cppreference](https://en.cppreference.com/w/cpp/concepts)
- [requires expression — cppreference](https://en.cppreference.com/w/cpp/language/requires)
- [Constraints and concepts — cppreference](https://en.cppreference.com/w/cpp/language/constraints)
