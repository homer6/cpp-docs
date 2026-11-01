# std::indirect and std::polymorphic

> Heap-allocated owners with **value semantics**: copying one deep-copies the
> object it owns, and `const` reaches through to it.
> **Header:** `<memory>` · **Since:** C++26 (P3019)

> GCC 16 (libstdc++) ships both; GCC 15 and Clang 22 (libc++) don't, so the
> snippets are tagged `needs=indirect`. `__cpp_lib_indirect` and
> `__cpp_lib_polymorphic` are `202502L`.

## Mental model

A `unique_ptr` that behaves like a value. `indirect<T>` owns exactly a `T`;
`polymorphic<T>` owns a `T` or any class derived from it and remembers how to
copy the derived object. Neither has a null state you can ask for: a
default-constructed one holds a value-initialized `T`, and the only empty state
is **valueless after move**.

```
 indirect<T>     ──► [ T ]          copy → a new T, a copy of *this
 polymorphic<B>  ──► [ D : B ]      copy → a new D (not a sliced B)
```

## Declaration

```cpp needs=indirect
template<class T, class Allocator = std::allocator<T>> class indirect;
template<class T, class Allocator = std::allocator<T>> class polymorphic;

// polymorphic names the derived type to build:
//   std::polymorphic<Base> p{std::in_place_type<Derived>, ctor-args...};
```

Both give `operator*` and `operator->` (const-correct: a `const indirect<T>`
yields `const T&`), `valueless_after_move()`, `get_allocator()` and `swap`.
`indirect` also forwards `==`, `<=>` and `std::hash` to `T`; `polymorphic` has
no comparisons.

## When to use it

- **A class member that should copy like a value but live on the heap:** pImpl,
  a large member you want to move cheaply, a recursive type. With
  `unique_ptr` you'd hand-write the copy constructor; `indirect` gives you one.
- **A container of polymorphic objects that copies:** `vector<polymorphic<Shape>>`
  copies each element as its real type, with no `virtual clone()`.
- **Anti-fit:** shared or optional ownership. Use [`shared_ptr`](shared_ptr.md)
  for the first and `std::optional` (or `unique_ptr`) for the second.

## Examples

### indirect: deep copy and value comparison

```cpp needs=indirect
#include <memory>
#include <string>

void demo() {
    std::indirect<std::string> a{"hello"};
    auto b = a;                    // deep copy: a new string on the heap
    *b += "!";                     // a is still "hello"

    bool same = (a == b);          // false: compares the strings, not the pointers
    bool less = (a < b);           // true: "hello" < "hello!"

    std::indirect<int> n;          // holds 0, not null
}
```

### indirect for pImpl

`T` may be incomplete where the member is declared, as with `unique_ptr`.
Declare the special members in the header and default them where `Impl` is
complete; the copy constructor comes for free.

```cpp needs=indirect
#include <memory>
#include <string>

class Gadget {
    struct Impl;
    std::indirect<Impl> impl_;
public:
    Gadget();
    Gadget(const Gadget&);                  // deep-copies Impl
    Gadget& operator=(const Gadget&);
    ~Gadget();
};

struct Gadget::Impl { std::string name = "w"; };   // usually in the .cpp
Gadget::Gadget() = default;
Gadget::Gadget(const Gadget&) = default;
Gadget& Gadget::operator=(const Gadget&) = default;
Gadget::~Gadget() = default;
```

### polymorphic: copying without clone()

```cpp needs=indirect
#include <memory>
#include <vector>

struct Shape  { virtual ~Shape() = default; virtual double area() const = 0; };
struct Square : Shape { double s; explicit Square(double s) : s(s) {}
                        double area() const override { return s * s; } };

void demo() {
    std::vector<std::polymorphic<Shape>> shapes;
    shapes.emplace_back(std::in_place_type<Square>, 3.0);

    auto copy = shapes;            // copies each Square as a Square
    double a = copy[0]->area();    // 9, through the virtual call
}
```

## Gotchas

- **Moved-from means valueless.** After `auto b = std::move(a);`,
  `a.valueless_after_move()` is `true` and dereferencing `a` is undefined
  behaviour. Assign a new value before using it again.
- **Not nullable.** There's no `operator bool` and no `nullptr` state; a
  default-constructed `indirect<T>` allocates a `T`. If "maybe empty" is part
  of the design, use `std::optional` or `unique_ptr`.
- **`polymorphic` has no `==`.** It can't know how to compare two different
  derived types; compare through the base (`*a == *b`) if `Base` defines it.
- **Copying allocates.** Every copy is a heap allocation and a full copy of the
  object; that is the point, but don't copy in a hot loop by accident.

## See also

- [std::indirect — cppreference](https://en.cppreference.com/w/cpp/memory/indirect)
- [std::polymorphic — cppreference](https://en.cppreference.com/w/cpp/memory/polymorphic)
- [P3019R14: indirect and polymorphic](https://wg21.link/P3019R14)
- [`unique_ptr`](unique_ptr.md) · [`shared_ptr`](shared_ptr.md)
