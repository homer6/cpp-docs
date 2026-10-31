# Standard concepts

> The built-in concepts in `<concepts>`, plus how to apply and define your own.
> **Header:** `<concepts>` · **Since:** C++20

## The standard library concepts

**Core language / relationships:**
```cpp
struct Base {};
struct Derived : Base {};

static_assert(std::same_as<int, int>);                      // same_as<T, U>
static_assert(std::derived_from<Derived, Base>);            // derived_from<D, B>
static_assert(std::convertible_to<int, double>);            // convertible_to<From, To>
static_assert(std::common_with<int, double>);               // common_with<T, U>
static_assert(std::common_reference_with<int&, const int&>);// common_reference_with<T, U>
static_assert(std::assignable_from<std::string&, const char*>); // assignable_from<L, R>
static_assert(std::swappable<std::string>);                 // swappable<T>
```

**Type categories:**
```cpp
static_assert(std::integral<char>);
static_assert(std::signed_integral<int>);
static_assert(std::unsigned_integral<unsigned>);
static_assert(std::floating_point<double>);
```

**Comparison:**
```cpp
static_assert(std::equality_comparable<std::string>);
static_assert(std::equality_comparable_with<std::string, const char*>);
static_assert(std::totally_ordered<int>);
static_assert(std::totally_ordered_with<int, double>);
static_assert(std::three_way_comparable<std::string>);
```

**Object / lifetime:**
```cpp
static_assert(std::destructible<std::string>);
static_assert(std::constructible_from<std::string, std::size_t, char>);  // <T, Args...>
static_assert(std::default_initializable<std::vector<int>>);
static_assert(std::move_constructible<std::unique_ptr<int>>);
static_assert(std::copy_constructible<std::string>);
static_assert(std::movable<std::unique_ptr<int>>);
static_assert(!std::copyable<std::unique_ptr<int>>);       // move-only
static_assert(std::semiregular<std::string>);               // copyable + default_initializable
static_assert(std::regular<int>);                           // semiregular + equality_comparable
```

**Callable:**
```cpp
inline constexpr auto is_even = [](int x) { return x % 2 == 0; };
using IsEven = decltype(is_even);

static_assert(std::invocable<IsEven, int>);                 // invocable<F, Args...>
static_assert(std::regular_invocable<IsEven, int>);         // regular_invocable<F, Args...>
static_assert(std::predicate<IsEven, int>);                 // predicate<F, Args...>
static_assert(std::relation<std::less<>, int, long>);       // relation<R, T, U>
static_assert(std::strict_weak_order<std::less<>, int, double>); // strict_weak_order<R, T, U>
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
int compute();
void use_value() {
    std::integral auto value = compute();
}
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
