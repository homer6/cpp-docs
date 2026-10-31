# Type traits

> Compile-time queries and transformations on types: "is this an integer?",
> "strip the const", "pick a type conditionally." **Header:** `<type_traits>` ·
> **Since:** C++11

## Mental model

Each trait is a template that yields a compile-time **value** or **type**:

- Value traits expose `::value` (and a `_v` shortcut): `std::is_integral_v<T>`.
- Type traits expose `::type` (and a `_t` shortcut): `std::remove_const_t<T>`.

```cpp
static_assert(std::is_integral<int>::value);   // true   (verbose)
static_assert(std::is_integral_v<int>);        // true   (C++17 _v helper — prefer this)
static_assert(std::is_same_v<std::remove_const<const int>::type, int>);  // int (verbose)
static_assert(std::is_same_v<std::remove_const_t<const int>, int>);      // int (C++14 _t helper — prefer this)
```

## The main families

**Type categories** — what kind of type is it:
```cpp
using std::is_void, std::is_integral, std::is_floating_point, std::is_array,
      std::is_pointer, std::is_enum, std::is_class, std::is_function,
      std::is_reference, std::is_arithmetic, std::is_fundamental;
```

**Type properties** — qualifiers and capabilities:
```cpp
using std::is_const, std::is_volatile, std::is_signed, std::is_unsigned,
      std::is_trivially_copyable, std::is_standard_layout, std::is_empty,
      std::is_polymorphic, std::is_default_constructible,
      std::is_copy_constructible, std::is_move_constructible,
      std::is_nothrow_move_constructible, std::is_destructible,
      std::has_virtual_destructor;
```

**Type relationships**:
```cpp
using std::is_same;          // is_same<A, B>
using std::is_base_of;       // is_base_of<Base, Derived>
using std::is_convertible;   // is_convertible<From, To>
using std::is_invocable;     // is_invocable<F, Args...>
using std::is_invocable_r;   // is_invocable_r<R, F, Args...>
```

**Transformations** — produce a new type:
```cpp
using std::remove_const, std::remove_reference, std::remove_cv,
      std::add_pointer, std::add_lvalue_reference;
using std::decay;            // array→ptr, function→ptr, strips cv/ref (as by-value pass)
using std::remove_cvref;     // strip cv AND reference (C++20) — very common
using std::conditional;      // conditional<B, T, F>: B ? T : F at type level
using std::common_type;      // common_type<Ts...>: the type a set of types converts to
using std::underlying_type;  // an enum's integer type
using std::enable_if;        // enable_if<B, T>: present only if B is true (SFINAE)
```

## Examples

### Constraining with concepts/`if constexpr` (modern) vs `enable_if` (legacy)

```cpp
// C++20: a concept (clearest)
template<class T> requires std::is_integral_v<T>
T half(T x) { return x / 2; }

// C++17: compile-time branch
template<class T>
auto describe(T) {
    if constexpr (std::is_floating_point_v<T>) return "float";
    else if constexpr (std::is_integral_v<T>)  return "int";
    else                                        return "other";
}

// C++11/14: SFINAE with enable_if (you'll see this in older code)
template<class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
T legacy_half(T x) { return x / 2; }
```

### static_assert for clear contracts

```cpp
template<class T>
void store(const T&) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "store() requires a trivially copyable type");
}
```

### Normalizing a deduced type

```cpp
template<class T>
void f(T&& x) {
    using Bare = std::remove_cvref_t<T>;     // strip ref + const/volatile (C++20)
    Bare copy = x;
}
```

## Gotchas

- **Use the `_v` / `_t` helpers** (`is_integral_v`, `remove_cv_t`), not the
  verbose `::value` / `::type` — clearer and the modern norm.
- **Prefer concepts and `if constexpr` over `enable_if`** in C++17/20. SFINAE is
  hard to read and gives terrible errors; reserve `enable_if` for maintaining old
  code.
- **`std::decay_t` vs `std::remove_cvref_t`:** `decay` also does array→pointer and
  function→pointer decay (what happens to by-value parameters); `remove_cvref`
  only strips cv and reference. Pick deliberately.
- **Traits answer compile-time questions only** — they don't run at runtime and
  can't inspect values, just types.

## See also

- [Type support / type_traits — cppreference](https://en.cppreference.com/w/cpp/header/type_traits)
- [std::enable_if — cppreference](https://en.cppreference.com/w/cpp/types/enable_if)
- [std::conditional — cppreference](https://en.cppreference.com/w/cpp/types/conditional)
