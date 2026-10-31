# Operator overloading

> Giving a class its own meaning for `+`, `==`, `[]`, `()`, `<<` and friends by
> declaring functions named `operator@`. **Since:** C++98 · multi-argument `[]`
> and `static operator()` / `operator[]` since C++23

Precedence, associativity and evaluation order are the built-in operators' and
can't be changed — they're on [Operators](operators.md).
Comparisons (`==`, `<=>`, defaulted or custom) have their own page:
[Three-way comparison](../../language-support/comparison.md). This page covers
the rest of the mechanics.

## Mental model

An operator expression on a class or enum operand is a function call in
disguise. The compiler gathers candidates from three places and runs normal
overload resolution:

```
a + b   →   a.operator+(b)        member candidates   (a's class)
        →   operator+(a, b)       non-member candidates (ordinary lookup + ADL)
        →   built-in  int + int…  built-in candidates  (after conversions)

a == b  →   also rewritten/reversed candidates from == and <=> (C++20)
```

## Declaration

```cpp nocompile
R T::operator@(Arg) [const];        // binary, member: left operand is *this
R operator@(const T&, Arg);         // binary, non-member: both operands are parameters
R T::operator@() [const];           // unary, member
R operator@(const T&);              // unary, non-member
T& T::operator++();                 // prefix ++
T  T::operator++(int);              // postfix ++ (the int is a tag, always 0)
explicit T::operator U() const;     // conversion function, no return type written
```

| Rule | Detail |
|---|---|
| Must be members | `=`, `()`, `[]`, `->`, and conversion functions |
| Can't be overloaded | `::` `.` `.*` `?:` `sizeof` `alignof` `typeid` `noexcept` and the casts |
| Operand types | at least one parameter of class or enum type — `operator+(int, int)` is ill-formed |
| Arity | fixed by the operator, except `()` and (since C++23) `[]`, which take any number of parameters and may have default arguments |
| `static` | `operator()` and `operator[]` may be `static` (C++23) |
| Also overloadable | `new`/`delete` (allocation functions), `co_await`, user-defined literals `operator""_x` |

User-defined literals are covered on [Literals](literals.md).

## When to use it

- **The type models something whose operators are conventional**: numbers,
  vectors, matrices, iterators, smart pointers, strings, function objects.
- **Keep the built-in meaning.** `+` adds and doesn't mutate, `==` is an
  equivalence, `[]` indexes, `<<` on a stream inserts. If the meaning needs a
  comment, use a named function.
- Anti-fit: `&&`, `||` and `,` (they lose short-circuiting), unary `&`, and
  "clever" DSL-style overloads that surprise readers.

## Examples

### Arithmetic: compound assignment + symmetric hidden friend

Implement `@=` as a member, then `@` in terms of it as a *hidden friend* — a
`friend` defined in the class, found only by ADL. Both operands of the friend
take part in implicit conversion, so `1 + m` and `m + 1` both work if `T` is
constructible from `int`.

```cpp
#include <iostream>

class Money {
    long cents_ = 0;
public:
    constexpr Money() = default;
    constexpr explicit Money(long c) : cents_{c} {}
    constexpr long cents() const { return cents_; }

    constexpr Money& operator+=(Money o) { cents_ += o.cents_; return *this; }
    constexpr Money  operator-() const   { return Money{-cents_}; }

    friend constexpr Money operator+(Money a, Money b) { return a += b; }
    friend constexpr bool  operator==(Money, Money) = default;
    friend std::ostream& operator<<(std::ostream& os, Money m) {
        return os << m.cents_ << "c";
    }
};

int main() {
    Money a{150}, b{275};
    std::cout << a + b << ' ' << -a << ' ' << (a + b == Money{425}) << '\n';  // 425c -150c 1
}
```

`operator<<` for a stream must be a non-member: the stream is the left
operand, and you can't add members to `std::ostream`.

### Increment and decrement

```cpp
#include <iostream>

struct Counter {
    int n = 0;
    Counter& operator++()    { ++n; return *this; }                     // prefix: returns *this
    Counter  operator++(int) { Counter old = *this; ++*this; return old; } // postfix: returns old value
};

int main() {
    Counter c;
    Counter before = c++;
    ++c;
    std::cout << before.n << ' ' << c.n << '\n';   // 0 2
}
```

Write postfix in terms of prefix. Prefer `++it` in generic code — postfix has to
make a copy.

### Subscript, including multi-dimensional (C++23)

```cpp
#include <cstddef>
#include <iostream>
#include <vector>

template<class T>
class Matrix {
    std::size_t cols_;
    std::vector<T> data_;
public:
    Matrix(std::size_t rows, std::size_t cols) : cols_{cols}, data_(rows * cols) {}

    // One body for const and non-const via deducing this (C++23):
    template<class Self>
    auto&& operator[](this Self&& self, std::size_t r, std::size_t c) {
        return std::forward<Self>(self).data_[r * self.cols_ + c];
    }
};

int main() {
    Matrix<int> m(2, 3);
    m[1, 2] = 7;                         // C++23: operator[] with two arguments
    const auto& cm = m;
    std::cout << cm[1, 2] << '\n';       // 7
}
```

Before C++23, `m[1, 2]` was the comma operator (deprecated in C++20) and you'd
write `m(1, 2)` instead. The const/non-const pair collapses into one function
with an [explicit object parameter](../classes/explicit-object-parameter.md).

### Function call and `static operator()` (C++23)

`operator()` makes a *function object*. A stateless one can be `static`, so
no object pointer is passed:

```cpp
#include <algorithm>
#include <vector>

struct ByAbs {
    static bool operator()(int a, int b) { return (a < 0 ? -a : a) < (b < 0 ? -b : b); }
};

std::vector<int> v{3, -1, -4, 2};
bool sorted = (std::ranges::sort(v, ByAbs{}), std::ranges::is_sorted(v, ByAbs{}));
bool direct = ByAbs::operator()(1, -2);   // callable without an object
```

### Conversions: `explicit operator bool`

```cpp
struct Handle {
    int* p = nullptr;
    constexpr explicit operator bool() const { return p != nullptr; }
};

constexpr bool test() {
    Handle h;
    if (h) return false;          // OK: contextual conversion in if/while/!/&&/||/?:
    return !h;
}
static_assert(test());
```

```cpp compile-fail="cannot convert|no viable conversion"
struct Handle { explicit operator bool() const { return true; } };
bool b = Handle{};               // error: explicit conversion isn't implicit
int  n = Handle{} + 1;           // (and so this can't silently compile either)
```

`explicit` stops a handle from turning into `1` in arithmetic or comparing
equal to unrelated handles.

### Pointer-like types: `->` and `*`

```cpp
#include <iostream>
#include <memory>
#include <string>

class Boxed {
    std::unique_ptr<std::string> p_ = std::make_unique<std::string>("arrow");
public:
    std::string* operator->() const { return p_.get(); }   // a->m  ⇒  (a.operator->())->m
    std::string& operator*()  const { return *p_; }
};

int main() {
    Boxed b;
    std::cout << b->size() << ' ' << *b << '\n';   // 5 arrow
}
```

`operator->` is applied again to whatever it returns until a raw pointer is
reached ("drill-down"), which is how proxies and iterators chain.

## Gotchas

- **A member operator doesn't convert its left operand.** With
  `M M::operator+(M) const`, `m + 1` may compile through a converting
  constructor but `1 + m` never does. Use a non-member (hidden friend) for
  symmetric binary operators.

  ```cpp compile-fail="no match for|invalid operands"
  struct M {
      M(int) {}
      M operator+(M) const { return *this; }
  };
  M ok  = M{2} + 1;    // fine: 1 converts to M
  M bad = 1 + M{2};    // error: int has no member operator+
  ```

- **Overloaded `&&`, `||` lose short-circuiting.** Both operands are always
  evaluated (C++17 does sequence the left one first). `mk(false) && mk(true)`
  with an overloaded `&&` calls both.
- **Unary `operator&` breaks `&x`.** Generic code must use `std::addressof(x)`,
  which always gives the real address.
- **Return conventions matter.** Assignment and compound assignment return
  `T&` (`*this`) so `a = b = c` works; arithmetic returns a new `T` by value;
  returning a reference to a local from `operator+` dangles.
- **Self-assignment.** A hand-written `operator=` must handle `a = a`; copy-and-swap
  or the rule of zero sidesteps it (see [Special members](../classes/special-members.md)).
- **Operator lookup is ADL-driven.** An operator declared in a different namespace
  from its operand type is only found if that namespace is open at the call site
  — a classic "no match for `operator<<`" in templates. Keep operators next to
  (or inside, as hidden friends) the type.
- **Don't overload operators for types you don't own** (e.g. `operator<<` for
  `std::vector<int>`). ADL looks in `std`, not your namespace, so it isn't found
  from generic code elsewhere; and if two libraries both add one at global
  scope, the program violates the one-definition rule. Wrap the type instead.

## See also

- [Operator overloading — cppreference](https://en.cppreference.com/w/cpp/language/operators)
- [Overload resolution (operator candidates) — cppreference](https://en.cppreference.com/w/cpp/language/overload_resolution#Call_to_an_overloaded_operator)
- [Default comparisons — cppreference](https://en.cppreference.com/w/cpp/language/default_comparisons)
- [User-defined conversion function — cppreference](https://en.cppreference.com/w/cpp/language/cast_operator)
- [\[over.oper\] Overloaded operators — C++ draft](https://eel.is/c++draft/over.oper)
- [P2128R6 — Multidimensional subscript operator](https://wg21.link/p2128r6),
  [P1169R4 — static operator()](https://wg21.link/p1169r4),
  [P2589R1 — static operator[]](https://wg21.link/p2589r1)
- In this repo: [Operators](operators.md),
  [Three-way comparison](../../language-support/comparison.md),
  [Literals](literals.md),
  [Explicit object parameters](../classes/explicit-object-parameter.md)
