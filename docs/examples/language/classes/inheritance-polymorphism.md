# Inheritance & polymorphism

> Building types from base types, and dispatching behavior at runtime through
> `virtual` functions.

## Inheritance

```cpp
class Shape {
public:
    virtual ~Shape() = default;              // virtual destructor — essential for base classes
    virtual double area() const = 0;          // pure virtual → Shape is abstract
    virtual std::string name() const { return "shape"; }   // virtual with a default
};

class Circle : public Shape {
    double r_;
public:
    explicit Circle(double r) : r_{r} {}
    double area() const override { return 3.14159 * r_ * r_; }   // override checked by compiler
    std::string name() const override { return "circle"; }
};
```

`public` inheritance models **"is-a"**: a `Circle` is a `Shape`.

## Dynamic dispatch

```cpp
std::unique_ptr<Shape> s = std::make_unique<Circle>(2.0);
s->area();              // calls Circle::area() — resolved at runtime via the vtable
s->name();              // "circle"

void render(const Shape& sh) { sh.area(); }   // works for any Shape subtype
```

Calling a `virtual` function through a base reference/pointer dispatches to the
most-derived override. Through a *value*, it doesn't (see slicing).

## override and final

```cpp
class Derived : public Base {
    void f() override;          // ERROR if Base::f isn't virtual / signature mismatch
    void g() final;             // no further class may override g
};
class Sealed final {};          // no class may derive from Sealed
```

Always write `override` — it turns "oops, I didn't actually override anything"
(typo, const mismatch) into a compile error.

## Abstract base classes & interfaces

A class with a pure virtual (`= 0`) function can't be instantiated; it defines an
interface derived classes must implement. An interface is typically all pure
virtuals plus a virtual destructor.

## Gotchas

- **A polymorphic base needs a `virtual` destructor.** `delete`ing a `Derived`
  through a `Base*` with a non-virtual destructor is UB (the derived destructor
  won't run). Give every base with virtuals a `virtual ~Base() = default;`.
- **Object slicing.** Assigning/copying a `Derived` into a `Base` *value* copies
  only the base part and loses the override — pass/store by reference or smart
  pointer, never by base value.
- **Always use `override`.** Without it, a signature mismatch silently creates a
  *new* function instead of overriding, and virtual dispatch quietly calls the
  base.
- **Don't call virtuals from constructors/destructors** expecting derived
  behavior — during base construction, the object *is* a base, so the base version
  is called.
- **Prefer composition / `std::variant` / templates** when you don't truly need
  runtime polymorphism — virtual dispatch has a cost and couples hierarchies.

## See also

- [Derived classes — cppreference](https://en.cppreference.com/w/cpp/language/derived_class)
- [virtual functions — cppreference](https://en.cppreference.com/w/cpp/language/virtual)
- [override / final — cppreference](https://en.cppreference.com/w/cpp/language/override)
