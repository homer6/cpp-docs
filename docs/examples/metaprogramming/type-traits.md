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
using std::type_identity;    // type_identity<T>::type is T: blocks deduction (C++20)
using std::void_t;           // void_t<Ts...> is void: the detection idiom (C++17)
```

**Logical operations** (C++17) — combine value traits, short-circuiting:
```cpp
using std::conjunction, std::disjunction, std::negation;
```

The [constant evaluation context](constant-evaluation.md) queries,
`std::is_constant_evaluated` and `std::is_within_lifetime`, are in
`<type_traits>` too.

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

### `std::declval`: a value you may only name in unevaluated contexts

`std::declval<T>()` (`<utility>`, C++11) pretends to produce a `T&&`, so you can
ask what an expression *would* be, even for a type with no default constructor.
It has no definition: use it only inside `decltype`, `sizeof`, `noexcept`,
`requires` and other unevaluated operands.

```cpp
struct NoDefault { NoDefault(int); int size() const; };

using R = decltype(std::declval<NoDefault>().size());   // int; NoDefault{} wouldn't compile
static_assert(std::is_same_v<R, int>);
static_assert(std::is_same_v<decltype(std::declval<int>()), int&&>);   // T&&, not T
static_assert(std::is_same_v<decltype(std::declval<int&>()), int&>);   // reference collapsing
```

Evaluating it is ill-formed, and both libraries say so with a `static_assert`:

```cpp compile-fail="declval|unevaluated"
int main() { auto x = std::declval<int>(); (void)x; }
```

### `std::void_t` and the detection idiom vs a `requires` expression

`std::void_t<Ts...>` (C++17) is always `void`, but only if every `Ts` is a
valid type. Put an expression's `decltype` in it and a partial specialization
matches only when that expression compiles:

```cpp
// C++17 detection idiom: the primary says "no", the specialization says "yes"
template<class T, class = void>
struct has_size : std::false_type {};
template<class T>
struct has_size<T, std::void_t<decltype(std::declval<const T&>().size())>> : std::true_type {};

// C++20: the same question as a concept
template<class T>
concept HasSize = requires(const T& t) { t.size(); };

static_assert(has_size<std::vector<int>>::value && !has_size<int>::value);
static_assert(HasSize<std::string> && !HasSize<int>);
```

Write new code with `requires`: it needs no `declval`, reads as the expression
itself, and gives better diagnostics. You'll meet `void_t` in C++17 libraries.

### `std::type_identity` (C++20): keep a parameter out of deduction

A parameter of type `std::type_identity_t<T>` is a *non-deduced context*: `T`
comes from the other arguments, and this one just converts.

```cpp
template<class T>
T clamp_to(T v, std::type_identity_t<T> lo, std::type_identity_t<T> hi) {
    return v < lo ? lo : hi < v ? hi : v;
}
auto r = clamp_to(1.5, 0, 1);   // T = double from v; 0 and 1 convert. r == 1.0
```

Without it, `T` is deduced from all three and the call is a conflict:

```cpp compile-fail="no matching function"
template<class T> T clamp_bad(T v, T lo, T hi) { return v < lo ? lo : hi < v ? hi : v; }
auto r = clamp_bad(1.5, 0, 1);   // T = double or int?
```

### `conjunction`, `disjunction`, `negation`: short-circuiting traits

`std::conjunction<B...>` derives from the first `Bi` whose `value` is false (or
the last one), and stops there: later traits are never instantiated.
`disjunction` stops at the first true one. A fold such as `(Ts::value && ...)`
names every `Ts::value`, so it instantiates all of them.

```cpp
template<class T> struct is_big : std::bool_constant<(sizeof(T) > 4)> {};  // ill-formed for void

static_assert(!std::conjunction_v<std::is_class<int>, is_big<void>>);  // stops at is_class<int>
static_assert(std::disjunction_v<std::is_integral<int>, is_big<void>>); // stops at is_integral<int>
static_assert(std::negation_v<std::is_pointer<int>>);

template<class... Ts> constexpr bool all_ints = std::conjunction_v<std::is_integral<Ts>...>;
static_assert(all_ints<int, long> && !all_ints<int, double>);
```

```cpp compile-fail="sizeof"
template<class T> struct is_big : std::bool_constant<(sizeof(T) > 4)> {};
template<class... Ts> constexpr bool all = (Ts::value && ...);
static_assert(!all<std::is_class<int>, is_big<void>>);   // instantiates is_big<void>: error
```

### `is_layout_compatible` and `is_corresponding_member` (C++20)

`std::is_layout_compatible_v<A, B>` says two standard-layout types have the same
sequence of member types, and `std::is_corresponding_member(&A::m, &B::n)` says
two members sit at the same place in that common initial sequence. GCC 15/16
have both; Clang 22's libc++ has neither yet (no
`__cpp_lib_is_layout_compatible`).

```cpp
struct A { int x; char c; };
struct B { int y; char d; };
struct C { int z; double e; };

static_assert(std::is_layout_compatible_v<A, B>);
static_assert(!std::is_layout_compatible_v<A, C>);
static_assert(std::is_corresponding_member(&A::c, &B::d));
static_assert(std::is_corresponding_member(&A::x, &C::z));   // common initial sequence: int
static_assert(!std::is_corresponding_member(&A::c, &C::e));
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
- **`std::declval` is for unevaluated operands only.** Calling it in code that
  runs, or in a constant expression, fails to compile.
- **A fold over `::value` doesn't short-circuit instantiation.** Use
  `std::conjunction` / `std::disjunction` when a later trait would be
  ill-formed for the types an earlier one rules out.
- **Traits answer compile-time questions only** — they don't run at runtime and
  can't inspect values, just types.

## See also

- [Type support / type_traits — cppreference](https://en.cppreference.com/w/cpp/header/type_traits)
- [std::enable_if — cppreference](https://en.cppreference.com/w/cpp/types/enable_if)
- [std::conditional — cppreference](https://en.cppreference.com/w/cpp/types/conditional)
- [std::declval — cppreference](https://en.cppreference.com/w/cpp/utility/declval)
- [std::void_t — cppreference](https://en.cppreference.com/w/cpp/types/void_t)
- [std::type_identity — cppreference](https://en.cppreference.com/w/cpp/types/type_identity)
- [std::conjunction — cppreference](https://en.cppreference.com/w/cpp/types/conjunction)
- [std::is_layout_compatible — cppreference](https://en.cppreference.com/w/cpp/types/is_layout_compatible)
- [std::is_corresponding_member — cppreference](https://en.cppreference.com/w/cpp/types/is_corresponding_member)
- [Constant evaluation context](constant-evaluation.md) — `is_constant_evaluated`, `if consteval`, `is_within_lifetime`
- [Requires expressions](../language/templates/constraints.md) — the C++20 replacement for the detection idiom
