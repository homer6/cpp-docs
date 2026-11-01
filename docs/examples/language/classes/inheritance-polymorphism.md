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
struct Shape {                                // as above, abbreviated
    virtual ~Shape() = default;
    virtual double area() const = 0;
    virtual std::string name() const { return "shape"; }
};
struct Circle : Shape {
    double r;
    explicit Circle(double r) : r{r} {}
    double area() const override { return 3.14159 * r * r; }
    std::string name() const override { return "circle"; }
};

void render(const Shape& sh) { sh.area(); }   // works for any Shape subtype

void demo() {
    std::unique_ptr<Shape> s = std::make_unique<Circle>(2.0);
    s->area();          // calls Circle::area() — resolved at runtime via the vtable
    s->name();          // "circle"
    render(*s);
}
```

Calling a `virtual` function through a base reference/pointer dispatches to the
most-derived override. Through a *value*, it doesn't (see slicing).

## override and final

```cpp
class Base {
public:
    virtual ~Base() = default;
    virtual void f();
    virtual void g();
};
class Derived : public Base {
    void f() override;          // ERROR if Base::f isn't virtual / signature mismatch
    void g() final;             // no further class may override g
};
class Sealed final {};          // no class may derive from Sealed
```

Always write `override` — it turns "oops, I didn't actually override anything"
(typo, const mismatch) into a compile error.

### Covariant return types

An override may return a pointer or reference to a class **derived from** the
base version's return type. Callers that know the derived type get it without a
cast:

```cpp
struct Shape {
    virtual ~Shape() = default;
    virtual Shape* clone() const { return new Shape(*this); }
};
struct Circle : Shape {
    Circle* clone() const override { return new Circle(*this); }   // covariant: Circle*, not Shape*
};

void demo(const Circle& c, const Shape& s) {
    Circle* c2 = c.clone();       // no cast needed
    Shape*  s2 = s.clone();       // through the base: dynamic dispatch, a Shape*
    delete c2; delete s2;
}
```

Only raw pointers and references are covariant, and the derived class must be
complete at the override:

```cpp compile-fail="covariant|different return type"
struct Base { virtual std::unique_ptr<Base> clone() const; virtual ~Base() = default; };
struct Derived : Base { std::unique_ptr<Derived> clone() const override; };   // ERROR
```

For smart pointers, use a non-virtual public `clone()` that wraps a covariant (or
plain) virtual one.

## Abstract base classes & interfaces

A class with a pure virtual (`= 0`) function can't be instantiated; it defines an
interface derived classes must implement. An interface is typically all pure
virtuals plus a virtual destructor.

## Multiple inheritance & virtual bases (the diamond)

With plain multiple inheritance, a shared base appears **once per path**, and
naming its members is ambiguous:

```cpp compile-fail="ambiguous|multiple"
struct Device { int id = 0; };
struct Scanner : Device {};
struct Printer : Device {};
struct Copier  : Scanner, Printer {};          // two Device subobjects

int get(Copier& c) { return c.id; }            // error: which Device::id?
```

Inheriting `virtual`ly shares one base subobject among all paths. The
**most-derived class** constructs it, and the intermediate classes'
initializers for it are skipped:

```cpp
struct Device { int id; explicit Device(int i = 0) : id{i} {} };
struct Scanner : virtual Device { Scanner() : Device{1} {} };
struct Printer : virtual Device { Printer() : Device{2} {} };
struct Copier  : Scanner, Printer { Copier() : Device{3} {} };

int id = Copier{}.id;                          // 3: one Device, built by Copier
```

Virtual bases cost an extra indirection to reach, and you can't `static_cast`
down from one (use `dynamic_cast`). Reserve them for interface-like bases that
genuinely appear on several paths.

## Gotchas

- **A virtual base is initialized by the most-derived class.** If it lacks a
  default constructor, every non-abstract class below it must name it in its
  member-init list.
- **A polymorphic base needs a `virtual` destructor.** `delete`ing a `Derived`
  through a `Base*` with a non-virtual destructor is UB (the derived destructor
  won't run). Give every base with virtuals a `virtual ~Base() = default;`.
- **Object slicing.** Assigning/copying a `Derived` into a `Base` *value* copies
  only the base part and loses the override — pass/store by reference or smart
  pointer, never by base value.
- **Always use `override`.** Without it, a signature mismatch silently creates a
  *new* function instead of overriding, and virtual dispatch quietly calls the
  base.
- **Covariance works only for raw pointers and references.** An override returning
  `std::unique_ptr<Derived>` where the base returns `std::unique_ptr<Base>` is an
  error.
- **Don't call virtuals from constructors/destructors** expecting derived
  behavior — during base construction, the object *is* a base, so the base version
  is called.
- **Prefer composition / `std::variant` / templates** when you don't truly need
  runtime polymorphism — virtual dispatch has a cost and couples hierarchies.

## See also

- [Derived classes — cppreference](https://en.cppreference.com/w/cpp/language/derived_class)
- [virtual functions — cppreference](https://en.cppreference.com/w/cpp/language/virtual)
- [override / final — cppreference](https://en.cppreference.com/w/cpp/language/override)
- [Virtual base classes — cppreference](https://en.cppreference.com/w/cpp/language/derived_class#Virtual_base_classes)
