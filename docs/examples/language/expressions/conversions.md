# Conversions & casts

> Implicit conversions happen silently; explicit casts say exactly what you mean.
> Prefer the named C++ casts over the C-style `(T)x`.

## Implicit conversions

The compiler inserts these automatically — sometimes helpfully, sometimes
dangerously:

```cpp
int i = 3.9;          // double → int: TRUNCATES to 3 (narrowing)
double d = i;         // int → double: safe (widening)
long L = i;           // integral promotion
if (ptr) {}           // pointer → bool
Derived* p; Base* b = p;   // derived → base pointer (upcast): safe
```

Use **brace-init to ban narrowing**:

```cpp
int x{3.9};           // ERROR: narrowing conversion not allowed in {}
```

## The four named casts

```cpp
static_cast<T>(x)        // well-defined conversions: numeric, up/down-cast (no runtime check),
                         // void* → T*, enum ↔ int, explicit-constructor conversions
dynamic_cast<T>(x)       // safe down-cast in a polymorphic hierarchy; checks at RUNTIME
                         //   → nullptr (pointers) or throws std::bad_cast (references) on failure
const_cast<T>(x)         // add/remove const/volatile (only safe to remove const you know isn't really const)
reinterpret_cast<T>(x)   // bit-level reinterpretation; almost always UB to then read — last resort
```

```cpp
double d = static_cast<double>(a) / b;        // force floating division
if (auto* dog = dynamic_cast<Dog*>(animal)) { /* it really is a Dog */ }
```

## Explicit constructors & conversion operators

```cpp
struct Meters {
    explicit Meters(double);          // explicit → no silent double → Meters
    explicit operator double() const; // explicit → no silent Meters → double
};
Meters m{5.0};                        // ok (direct)
double x = static_cast<double>(m);     // must ask explicitly
```

`explicit` (and C++20's `explicit(bool)`) prevents surprising implicit
conversions through your constructors/operators.

## Gotchas

- **Avoid C-style casts `(T)x`.** They silently pick whichever of
  static/const/reinterpret "works," hiding intent and danger. Use a named cast so
  the reader (and compiler) knows what you meant.
- **`reinterpret_cast` then dereference is usually UB.** To reinterpret bits of a
  trivially-copyable type, use [`std::bit_cast`](../../numerics/bit.md); to view
  raw bytes as an object, [`std::start_lifetime_as`](../../memory/raw-memory.md).
- **`dynamic_cast` needs a polymorphic type** (at least one virtual function) and
  costs a runtime check — don't use it where `static_cast` or virtual dispatch
  fits.
- **Narrowing in `()` init is silent; in `{}` it's an error.** Prefer `{}` to
  catch `int x{someLong}` truncation at compile time.
- **`const_cast`-ing away const and then writing to a truly-const object is UB.**
  Only legitimate when the underlying object isn't actually const.

## See also

- [Implicit conversions — cppreference](https://en.cppreference.com/w/cpp/language/implicit_conversion)
- [static_cast / dynamic_cast / etc. — cppreference](https://en.cppreference.com/w/cpp/language/expressions#Conversions)
- [explicit specifier — cppreference](https://en.cppreference.com/w/cpp/language/explicit)
