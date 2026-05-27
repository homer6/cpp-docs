# Template basics

> Writing code once that works for many types (and compile-time values).

## Function templates

```cpp
template<class T>
T max_of(T a, T b) { return a > b ? a : b; }

max_of(3, 7);          // T = int, deduced from arguments
max_of(2.5, 1.0);      // T = double
max_of<long>(3, 7);    // T explicitly long
```

The compiler **instantiates** a concrete function per type used. Argument types
are usually deduced; you can supply them explicitly when needed.

## Class templates

```cpp
template<class T>
class Box {
    T value_;
public:
    explicit Box(T v) : value_{std::move(v)} {}
    const T& get() const { return value_; }
};

Box<int> b{42};
Box bs{"hi"s};         // CTAD deduces Box<std::string> (C++17) — see CTAD page
```

## Non-type template parameters (NTTPs)

Templates can be parameterized on **values**, not just types:

```cpp
template<class T, std::size_t N>
struct Array { T data[N]; };          // N is a compile-time size (this is how std::array works)

template<int Base>
int scaled(int x) { return x * Base; }
scaled<10>(5);                         // 50

// C++20: floating-point and class-type NTTPs (with structural types) are allowed
```

## Constraining templates (C++20)

Prefer [concepts](../../concepts/standard-concepts.md) over unconstrained
templates — better errors, intentful overloading:

```cpp
template<std::integral T>              // only integers
T half(T x) { return x / 2; }

template<class T> requires std::floating_point<T>
T half(T x) { return x / 2; }
```

## Templates live in headers

Because the compiler needs the full definition to instantiate, template
definitions normally go in headers (not split into a .cpp). Heavy templating
increases compile times and can bloat binaries (code per instantiation).

## Gotchas

- **`typename` and `template` disambiguators.** Inside templates, dependent names
  need help: `typename T::value_type x;` (it's a type) and
  `obj.template method<U>();` (it's a template). Omitting them is a common compile
  error.
- **Definitions in headers.** Splitting a template's declaration and definition
  across .h/.cpp causes "undefined reference" at link time unless you explicitly
  instantiate.
- **Error messages can be brutal.** Constrain with concepts (C++20) to get
  readable diagnostics instead of pages of instantiation backtrace.
- **Each instantiation is separate code.** Many type/value combinations ⇒ bigger
  binaries and slower builds; factor type-independent logic into non-template
  helpers.

## See also

- [Templates — cppreference](https://en.cppreference.com/w/cpp/language/templates)
- [Template parameters — cppreference](https://en.cppreference.com/w/cpp/language/template_parameters)
- [Dependent names (`typename`/`template`) — cppreference](https://en.cppreference.com/w/cpp/language/dependent_name)
