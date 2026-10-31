# Explicit object parameters ("deducing `this`")

> A member function can name its object as an ordinary first parameter,
> `this Self&& self`, whose type and value category are deduced at the call site.
> **Since:** C++23 · **Feature-test macro:** `__cpp_explicit_this_parameter` (`202110L`)

## Mental model

Every non-static member function has a hidden object parameter: `*this`. Its
type is fixed by the cv- and ref-qualifiers you write (`const`, `&`, `&&`), so
getting all four combinations means writing four overloads. An **explicit object
parameter** makes that hidden parameter visible — and, when it's a template
parameter, *deduced*:

```
obj.get()                 // implicit:  get() const &   — one overload per qualifier set
                          // explicit:  get(this Self&& self)
std::move(cobj).get()     //            Self = const Obj   (rvalue)
cobj.get()                //            Self = const Obj&
derived.get()             //            Self = Derived&    ← the *dynamic-free* real type
```

Inside the function there is no `this`: you use `self.member`, exactly as in a
free function.

## Declaration

```cpp
struct S {
    void f(this S& self);                   // like  void f() &
    void g(this const S& self, int x);      // like  void g(int) const &
    void h(this S self);                    // by value: works on a copy of the object
    template<class Self>
    auto&& get(this Self&& self);           // deduced: one function for &, const&, &&, const&&
    auto&& get2(this auto&& self);          // the same, abbreviated
};
```

- `this` goes on the **first** parameter only.
- The function is not `static`, not `virtual`, and has no cv- or ref-qualifiers
  of its own; it can't use `this` in its body.
- A pointer to it is an ordinary function pointer: `&S::g` has type
  `void (*)(const S&, int)`, called as `pg(s, 1)`, not `(s.*pg)(1)`.
- `Self` is deduced from the object expression, so in a base-class member
  called on a derived object it is the **derived** type.

GCC 14 and Clang 18 were the first releases to implement it (the examples here are checked with GCC 15.2 and Clang 22).

## When to use it

- **De-duplicating const/non-const (and `&&`) overloads** of accessors —
  `get()`, `operator[]`, `value()` — that differ only in qualifiers.
- **Recursive lambdas**, without `std::function` or a Y-combinator.
- **Replacing CRTP** when the base only needs the derived type inside member
  functions.
- Anti-fit: anything needing `virtual` dispatch; ordinary members that only have
  one qualifier set (the implicit form is shorter and familiar).

## Examples

### One accessor for every value category

```cpp
#include <string>
#include <utility>

class Name {
    std::string s_;
public:
    explicit Name(std::string s) : s_{std::move(s)} {}

    // Replaces four overloads: & / const& / && / const&&
    template<class Self>
    auto&& str(this Self&& self) { return std::forward<Self>(self).s_; }
};

Name n{"Ada"};
const Name cn{"Grace"};
static_assert(std::is_same_v<decltype(n.str()), std::string&>);
static_assert(std::is_same_v<decltype(cn.str()), const std::string&>);
static_assert(std::is_same_v<decltype(std::move(n).str()), std::string&&>);
```

`std::forward<Self>(self).s_` keeps the object's value category on the member,
so `std::move(n).str()` hands back an rvalue you can move from. C++23's
[`std::forward_like`](https://en.cppreference.com/w/cpp/utility/forward_like)
does the same for something that isn't a direct member (e.g. `*self.ptr_`).

### Recursive lambdas

A lambda's explicit object parameter is the closure itself, so it can call
itself:

```cpp
#include <iostream>

int main() {
    auto fib = [](this const auto& self, int n) -> int {
        return n < 2 ? n : self(n - 1) + self(n - 2);
    };
    std::cout << fib(10) << '\n';   // 55
}
```

### Replacing CRTP

The classic CRTP base needs a template parameter and a `static_cast` to reach
the derived class. With deducing `this`, the base is an ordinary class and
`Self` *is* the derived type:

```cpp
#include <iostream>

struct Greeter {                               // no template<class Derived>
    void greet(this const auto& self) {
        std::cout << "hello from " << self.name() << '\n';
    }
};

struct Dog : Greeter { const char* name() const { return "Dog"; } };
struct Cat : Greeter { const char* name() const { return "Cat"; } };

int main() {
    Dog{}.greet();   // hello from Dog
    Cat{}.greet();   // hello from Cat
}
```

This is still static dispatch: through a `Greeter&`, `Self` is `Greeter` and
`self.name()` won't compile. Use `virtual` when you need runtime dispatch.

### By-value `self`

Taking `self` by value works on a copy — useful for small types and for
"return a modified copy" members:

```cpp
#include <iostream>

struct Point {
    int x = 0, y = 0;
    Point moved_by(this Point self, int dx, int dy) {   // self is a copy
        self.x += dx; self.y += dy;
        return self;
    }
};

int main() {
    Point p{1, 2};
    Point q = p.moved_by(10, 0);
    std::cout << p.x << ' ' << q.x << '\n';   // 1 11 — p is untouched
}
```

## Gotchas

- **No `this`, no implicit member access.** Write `self.x`, not `x`; a bare
  `x` in the body is looked up as if in a free function.

  ```cpp compile-fail="this|not declared|invalid use"
  struct S {
      int x = 0;
      int get(this const S& self) { return x; }   // error: no implicit object
  };
  ```

- **`Self` can be a derived type.** In a template like `str(this Self&& self)`,
  `self.s_` is looked up in the *derived* class first, so a derived member with
  the same name shadows it — and private members of the base are inaccessible
  if `Self` comes from a privately derived class. Cast when that matters:
  `static_cast<const Name&>(self).s_` — or take `this const Name& self` when
  you don't need deduction.
- **`this auto self` copies — including a derived object.** Called on a
  `Derived`, it deduces `Derived` and copies the whole thing.
- **Can't be `virtual` or `static`**, and can't be combined with `const`/`&`
  qualifiers after the parameter list.
- **Pointer-to-member syntax changes.** `&S::f` of an explicit-object function
  is a plain function pointer, which breaks code that expects
  `R (S::*)(Args...)`.
- **Capturing lambdas:** the explicit object parameter must be the closure type,
  a class publicly derived from it, or a reference to one of those — a capture
  can't be read from an unrelated object. GCC 15 rejects
  `[k](this Other) {…}` at the declaration; Clang 22 accepts it silently, so
  don't rely on the diagnostic.

## See also

- [Member functions — explicit object member functions — cppreference](https://en.cppreference.com/w/cpp/language/member_functions#Explicit_object_member_functions)
- [Lambda expressions — cppreference](https://en.cppreference.com/w/cpp/language/lambda)
- [`std::forward_like` — cppreference](https://en.cppreference.com/w/cpp/utility/forward_like)
- [P0847R7 — Deducing this](https://wg21.link/p0847r7)
- In this repo: [Members & access](members-access.md) (ref-qualified members),
  [Lambdas](../functions/lambdas.md), [Modern idioms](../../../guidelines/modern-idioms.md)
