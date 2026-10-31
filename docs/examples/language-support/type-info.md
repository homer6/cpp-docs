# typeid, std::type_info, std::type_index

> Run-time type identification: `typeid` yields a `std::type_info` describing a
> type or the dynamic type of a polymorphic object; `std::type_index` wraps it so
> it can key a map. **Header:** `<typeinfo>`, `<typeindex>` · **Since:** C++98;
> `type_index` C++11

## Mental model

Every type the program names with `typeid` has one `std::type_info` object
with static storage duration. `typeid(T)` and `typeid(expr)` return a
reference to it.

```
typeid(int)                  ─▶ type_info for int                  (compile time)
typeid(base_ref)             ─▶ type_info for the DYNAMIC type     (run time, via the vtable)
  └ only if the static type is polymorphic (has a virtual function)
typeid(non_polymorphic_expr) ─▶ static type; expr is NOT evaluated
```

`type_info` is neither copyable nor assignable; `type_index` is a copyable,
ordered, hashable handle to one.

## Declaration

```cpp nocompile
class type_info {
public:
    virtual ~type_info();
    constexpr bool operator==(const type_info&) const noexcept;   // constexpr since C++23
    bool before(const type_info&) const noexcept;                 // implementation-defined order
    std::size_t hash_code() const noexcept;
    const char* name() const noexcept;                            // implementation-defined text
    type_info(const type_info&) = delete;
    type_info& operator=(const type_info&) = delete;
};

class type_index {                                                // <typeindex>
public:
    type_index(const type_info&) noexcept;
    std::size_t hash_code() const noexcept;
    const char* name() const noexcept;
    bool operator==(const type_index&) const noexcept;
    std::strong_ordering operator<=>(const type_index&) const noexcept;
};
template<> struct hash<type_index>;

class bad_typeid : public exception { /* … */ };                  // <typeinfo>
```

`typeid` ignores top-level `const`/`volatile` and references:
`typeid(const int&) == typeid(int)`.

## When to use it

- **Type-keyed registries**: `std::unordered_map<std::type_index, Factory>`
  for plugin systems, serializers, ECS component stores.
- **Diagnostics**: logging the dynamic type of an object.

Anti-fits: dispatching on type with `if (typeid(x) == typeid(A)) … else if …`
(use virtual functions or `std::variant` + `std::visit`); checking
"is-a" relationships (`typeid` is exact — use `dynamic_cast`); and anything
compile-time (use `<type_traits>`, concepts, or `std::is_same_v`).

## Examples

### Static vs dynamic type

```cpp
#include <print>
#include <typeinfo>

struct Base { virtual ~Base() = default; };   // polymorphic
struct Derived : Base {};
struct Plain {};                              // not polymorphic
struct PlainDerived : Plain {};

int main() {
    Derived d;
    Base& b = d;
    PlainDerived pd;
    Plain& p = pd;
    std::println("{}", typeid(b) == typeid(Derived));        // true: dynamic type
    std::println("{}", typeid(p) == typeid(Plain));          // true: static type only
    std::println("{}", typeid(const int&) == typeid(int));   // true: cv/ref ignored
}
```

### A map keyed by type

```cpp
#include <print>
#include <string>
#include <typeindex>
#include <unordered_map>

int main() {
    std::unordered_map<std::type_index, std::string> names{
        {typeid(int), "int"}, {typeid(double), "double"}, {typeid(std::string), "string"},
    };
    std::println("{}", names.at(typeid(int)));    // int
}
```

### `name()` is mangled on GCC and Clang

```cpp
#include <cstdlib>
#include <cxxabi.h>        // GCC/Clang (Itanium ABI) only
#include <memory>
#include <print>
#include <string>
#include <typeinfo>

std::string demangle(const char* mangled) {
    int status = 0;
    std::unique_ptr<char, decltype(&std::free)> p{
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), &std::free};
    return status == 0 ? p.get() : mangled;
}

int main() {
    std::println("{}", typeid(std::string).name());
    std::println("{}", demangle(typeid(std::string).name()));
}
```

GCC 15 (libstdc++) prints `NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE`
and `std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >`;
Clang 22 with libc++ prints the `std::__1::` spelling. MSVC returns a readable
name directly (not verified locally).

### `typeid` on a null pointer throws

```cpp
#include <print>
#include <typeinfo>

struct Base { virtual ~Base() = default; };

int main() {
    Base* p = nullptr;
    try {
        (void)typeid(*p);                       // polymorphic glvalue through null
    } catch (const std::bad_typeid& e) {
        std::println("bad_typeid: {}", e.what());
    }
}
```

## Gotchas

- **`name()` and `before()` are implementation-defined.** `name()` gave `i` for
  `int` on both GCC and Clang here. `typeid(int).before(typeid(double))` was
  `false` with libstdc++ and `true` with libc++. Never persist or compare names
  across builds.
- **The operand is evaluated only for polymorphic glvalues.** `typeid(f())` does
  not call `f` when `f` returns a non-polymorphic type, but does when it returns
  `Base&`. Clang warns: `expression with side effects will be evaluated despite
  being used as an operand to 'typeid'`.
- **Non-polymorphic types give the static type**, silently — `typeid(p)` above
  says `Plain` even though the object is a `PlainDerived`.
- **Compare `type_info` objects, not addresses.** The same type can have
  several `type_info` objects across shared libraries; `operator==` handles it.
- **`-fno-rtti` rejects every `typeid`**, even `typeid(int)`: GCC 15 says
  `cannot use 'typeid' with '-fno-rtti'`, Clang 22 `use of typeid requires
  -frtti`. Libraries that must build without RTTI avoid it entirely.
- **`hash_code()` is not unique** across types and may change between runs; use
  it only through `std::hash<std::type_index>`.

## See also

- [`typeid` — cppreference](https://en.cppreference.com/w/cpp/language/typeid)
- [`std::type_info` — cppreference](https://en.cppreference.com/w/cpp/types/type_info)
- [`std::type_index` — cppreference](https://en.cppreference.com/w/cpp/types/type_index)
- [`std::bad_typeid` — cppreference](https://en.cppreference.com/w/cpp/types/bad_typeid)
- [Itanium C++ ABI demangler (`abi::__cxa_demangle`)](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#demangler)
