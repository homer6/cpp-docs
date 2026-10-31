# Constraints and concepts (the language)

> The language machinery behind concepts: `requires`-clauses, `requires`-expressions,
> abbreviated function templates, constrained `auto`, and how overloads are ranked
> by their constraints (subsumption). **Since:** C++20

The concepts *library* (`std::integral`, `std::regular`, …) and the four ways to
apply a concept are on [Standard concepts](../../concepts/standard-concepts.md).
This page is about the rules those concepts are built on.

## Mental model

A **constraint** is a compile-time `bool` attached to a template. Before overload
resolution looks at a template, the compiler checks its constraints with the
deduced arguments; an unsatisfied constraint quietly removes the candidate (like
SFINAE, but with a readable diagnostic if nothing is left).

```
template<class T> requires C1<T> && (C2<T> || C3<T>)
                           \___________________________/
   normalized into atomic constraints joined by ∧ / ∨:   C1-body ∧ (C2-body ∨ C3-body)

overload A: P            overload B: P ∧ Q
B's constraints imply A's ⇒ B "subsumes" A ⇒ B is more constrained ⇒ B wins the tie
```

## Declaration

```cpp nocompile
template<class T> concept Name = constraint-expression;      // a named predicate

template<class T> requires constraint void f(T);              // requires-clause after the parameter list
template<class T> void f(T) requires constraint;              // trailing requires-clause
template<Name T> void f(T);                                   // type-constraint
void f(Name auto x);                                          // abbreviated function template
Name auto v = expr;                                           // constrained placeholder

requires (params) { requirements; }                           // requires-expression: yields bool
```

- A **concept** is a named, never-specialized, never-constrained template of a
  `bool` constraint expression. Its value is computed, not looked up.
- A **requires-clause** takes a *primary expression* — a concept-id, `true`,
  a parenthesized expression, or such terms joined with `&&` / `||`. Anything
  else (`sizeof(T) > 4`) needs parentheses.
- A **requires-expression** is an expression of type `bool`: `true` if every
  requirement in its body is well-formed for the given arguments.

## When to use it

- **Constrain every template whose body assumes something about `T`.** The
  error moves from deep inside the instantiation to the call site, phrased as
  "constraint not satisfied".
- **Overload by refinement** (input vs random-access iterator, `Animal` vs `Pet`)
  with subsumption instead of tag dispatch or `enable_if`.
- **Switch members on and off** in a class template (`requires` on a member)
  instead of a specialization or a CRTP base.
- Reach for a **named concept** when the requirement is reused or takes part in
  overloading; a bare `requires (...)` or `if constexpr (requires { … })` is fine
  for a one-off.

## requires-expressions

The body holds four kinds of requirement:

```cpp
#include <concepts>
#include <cstddef>

template<class C>
concept Container = requires(C c, const C cc) {
    c.begin();                                              // simple: must compile
    typename C::value_type;                                 // type: must name a type
    { cc.size() } noexcept -> std::convertible_to<std::size_t>;  // compound
    requires sizeof(typename C::value_type) <= 64;          // nested: must be true
};

static_assert(Container<std::vector<int>>);
static_assert(!Container<int>);
```

- **Simple** — the expression must be valid (it is never evaluated).
- **Type** — `typename X;` the named type must exist.
- **Compound** — `{ e }` must be valid; optional `noexcept` requires it not to
  throw; `-> Concept<Args...>` checks `Concept<decltype((e)), Args...>`. Note the
  first argument is filled in for you, so `-> std::same_as<T>` means
  `std::same_as<decltype((e)), T>`.
- **Nested** — `requires expr;` the expression must be *satisfied* (evaluate to
  `true`), not merely valid. Writing `sizeof(...) <= 64;` without `requires`
  would only check that the comparison compiles.

A requires-expression can also be used directly where a `bool` is expected,
which is handy for a one-off check in a template:

```cpp
#include <iostream>

template<class T>
void describe(const T& x) {
    if constexpr (requires { x.size(); })
        std::cout << "sized: " << x.size() << '\n';
    else
        std::cout << "not sized\n";
}

int main() {
    describe(std::string{"abc"});   // sized: 3
    describe(42);                   // not sized
}
```

## Abbreviated function templates and constrained `auto`

A parameter declared `auto` (optionally with a concept in front) makes the
function a template; each `auto` invents its own template parameter:

```cpp
#include <concepts>

void log_all(const auto&... xs);                 // template<class... Ts> void log_all(const Ts&...)
auto add(std::integral auto a, std::integral auto b) { return a + b; }  // two independent types

// The same as:
template<std::integral A, std::integral B>
auto add2(A a, B b) { return a + b; }

static_assert(std::same_as<decltype(add(1, 2L)), long>);  // A = int, B = long
```

The same constrained placeholder works for variables and return types. The
deduced type is checked as the concept's first argument:

```cpp body
std::integral auto n = 40 + 2;                         // OK: int
std::convertible_to<long> auto m = 'x';                // checks convertible_to<char, long>
auto twice = [](std::floating_point auto x) { return 2 * x; };
use(n, m, twice(1.5));
```

```cpp compile-fail="placeholder constraints|constraints not satisfied|integral"
#include <concepts>
std::integral auto n = 3.0;      // error: deduced double does not satisfy std::integral
```

## Constraining members of class templates

A non-template member of a class template can carry a trailing requires-clause.
The classic use is a *conditionally trivial* type — the defaulted destructor
wins when it is viable because it is more constrained:

```cpp
#include <type_traits>
#include <string>

template<class T>
struct Holder {
    ~Holder() requires std::is_trivially_destructible_v<T> = default;
    ~Holder() { /* destroy the T we hold */ }
};

static_assert(std::is_trivially_destructible_v<Holder<int>>);
static_assert(!std::is_trivially_destructible_v<Holder<std::string>>);
```

Members can also be switched off entirely: `void reserve(std::size_t)
requires Growable<T>;` simply doesn't exist for other `T`.

## Partial ordering by constraints (subsumption)

When two overloads are otherwise equally good, the **more constrained** one
wins. "More constrained" is decided by *subsumption*, a purely logical check on
the normalized constraints — the compiler does not evaluate anything to decide it.

```cpp
#include <iostream>
#include <iterator>
#include <list>
#include <vector>

template<class T> concept Animal = requires(T t) { t.speak(); };
template<class T> concept Pet    = Animal<T> && requires(T t) { t.name(); };

struct Cat  { void speak(); const char* name(); };
struct Lion { void speak(); };

const char* who(Animal auto) { return "some animal"; }
const char* who(Pet auto)    { return "a pet"; }   // Pet = Animal ∧ … ⇒ subsumes Animal

template<std::input_iterator I>         const char* algo(I) { return "input"; }
template<std::random_access_iterator I> const char* algo(I) { return "random-access"; }

int main() {
    std::cout << who(Cat{}) << ", " << who(Lion{}) << '\n';   // a pet, some animal
    std::vector<int> v; std::list<int> l;
    std::cout << algo(v.begin()) << ", " << algo(l.begin()) << '\n';  // random-access, input
}
```

Both `who` overloads are viable for `Cat`; the `Pet` one is chosen only because
`Pet`'s definition literally contains `Animal<T>`. The standard iterator
concepts are built the same way (`random_access_iterator` refines
`bidirectional_iterator` refines … `input_iterator`), which is why `algo` works —
a direct replacement for iterator-tag dispatch.

### Subsumption only sees through named concepts

Normalization expands concept-ids, `&&` and `||`, and stops there. Everything
else is an **atomic constraint**, and two atomic constraints are the same only
if they are *the same expression at the same place in the source*. Writing the
same type trait twice does not count:

```cpp compile-fail="ambiguous"
#include <type_traits>

template<class T> requires std::is_integral_v<T>
const char* kind(T) { return "integral"; }

template<class T> requires (std::is_integral_v<T> && std::is_signed_v<T>)
const char* kind(T) { return "signed integral"; }

auto k = kind(1);   // error: ambiguous — the two is_integral_v<T> are unrelated atoms
```

Wrap the trait in a concept and refer to the concept from both places
(`std::integral` and `std::signed_integral` do exactly this), and the second
overload subsumes the first.

## Gotchas

- **Parenthesize non-trivial requires-clauses.** `requires sizeof(T) > 4` is a
  syntax error; write `requires (sizeof(T) > 4)`.
- **An atomic constraint must be exactly `bool`.** `requires (sizeof(T))` doesn't
  convert to `bool` — it makes the template unusable ("constraint expression
  does not have type `bool`").
- **Traits don't subsume; concepts do.** See above — for overloads that refine
  each other, build the refined concept *from* the base concept.
- **Negation is atomic.** `!C<T>` written in two declarations is two different
  atoms, so `requires (!C<T>)` vs `requires (!C<T> && D<T>)` is ambiguous. Name
  the negation (`concept NotC = !C<T>;`) and use the name.
- **A requires-expression checks validity, not meaning.** `{ a + b }` says `+`
  compiles, nothing about what it does. Concepts carry semantic requirements in
  their documentation only (see `std::regular`, `std::totally_ordered`).
- **Nested vs simple requirement.** Inside a requires-expression,
  `requires N > 0;` tests the value; `N > 0;` only tests that the expression is
  valid — almost always a bug.
- **Concepts can't be specialized or constrained**, and a concept can't refer
  to itself recursively.
- **`requires requires`** is a requires-clause whose expression is a
  requires-expression. It's legal; a named concept is usually clearer.

## See also

- [Constraints and concepts — cppreference](https://en.cppreference.com/w/cpp/language/constraints)
- [requires expression — cppreference](https://en.cppreference.com/w/cpp/language/requires)
- [Abbreviated function templates — cppreference](https://en.cppreference.com/w/cpp/language/function_template#Abbreviated_function_template)
- [Placeholder type specifiers (`auto`) — cppreference](https://en.cppreference.com/w/cpp/language/auto)
- [\[temp.constr\] Template constraints — C++ draft](https://eel.is/c++draft/temp.constr)
- [P0734R0 — Wording Paper, C++ extensions for Concepts](https://wg21.link/p0734r0)
- In this repo: [Standard concepts](../../concepts/standard-concepts.md),
  [Template basics](templates-basics.md), [Iterator categories](../../iterators/categories.md)
