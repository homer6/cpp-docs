# Specialization & CTAD

> Customizing a template for particular arguments, and letting the compiler deduce
> class template arguments.

## Full specialization

Provide a completely different implementation for a specific type:

```cpp
template<class T>
struct Serializer { static std::string to_text(const T&); };   // primary

template<>                                       // full specialization for bool
struct Serializer<bool> {
    static std::string to_text(bool b) { return b ? "true" : "false"; }
};
```

`std::hash`, `std::formatter`, and friends are customized exactly this way — you
fully specialize the library template for your type.

## Partial specialization (class templates only)

Specialize for a *pattern* of arguments (e.g. "any pointer", "any vector"):

```cpp
template<class T> struct IsPointer       { static constexpr bool value = false; };
template<class T> struct IsPointer<T*>   { static constexpr bool value = true;  };   // partial

template<class T> struct Traits;             // primary
template<class T> struct Traits<std::vector<T>> { using element = T; };               // partial
```

Function templates **cannot** be partially specialized — overload them instead.

## CTAD — class template argument deduction (C++17)

Construct a class template without spelling the type arguments; the compiler
deduces them from the constructor arguments:

```cpp
std::vector v{1, 2, 3};            // std::vector<int>
std::pair  p{1, 2.5};               // std::pair<int, double>
std::lock_guard lk{mtx};            // std::lock_guard<std::mutex>
std::array  a{1, 2, 3};             // std::array<int, 3>
```

### Deduction guides — help CTAD when needed

```cpp
template<class T> struct Box { Box(T); };
Box(const char*) -> Box<std::string>;        // guide: deduce std::string, not const char*

Box b{"hi"};                                  // → Box<std::string>
```

The standard library ships guides so e.g. `std::vector(first, last)` deduces the
element type from the iterators.

## Gotchas

- **Function templates can't be partially specialized.** Want type-pattern
  behavior for a function? Add an overload or delegate to a partially-specialized
  class trait.
- **Full specializations must appear before first use** of the template for that
  type, and live in the *same namespace* as the primary (this matters when
  specializing `std::hash`/`std::formatter` — open `namespace std`).
- **CTAD ignores explicitly-provided default template args** and can deduce
  surprising types (`std::pair p{1, "x"}` → `pair<int, const char*>`, not
  `std::string`). Provide a deduction guide or name the type when it matters.
- **CTAD doesn't apply to aliases pre-C++20** and not to member initialization in
  every context — when in doubt, write the type.

## See also

- [Template specialization — cppreference](https://en.cppreference.com/w/cpp/language/template_specialization)
- [Partial specialization — cppreference](https://en.cppreference.com/w/cpp/language/partial_specialization)
- [Class template argument deduction (CTAD) — cppreference](https://en.cppreference.com/w/cpp/language/class_template_argument_deduction)
