# GSL: Guidelines support library — examples

> Our examples for the [GSL entries](../gsl.md): the small library the
> Guidelines lean on. Only one entry has a numbered ID; it describes the
> pointer concepts that the smart-pointer rules are written against.

## GSL.ptr: Smart pointer concepts

The Guidelines describe three kinds of pointer-like type: a *pointer* (can be
dereferenced, compared and default-constructed to null), a *unique pointer*
(a pointer that can be moved but not copied) and a *shared pointer* (a
pointer that can be copied). With C++20 concepts we can write those
descriptions down and let the compiler check a type against them. A function
that takes ownership should ask for the right kind.

```cpp compile-fail="constraints not satisfied|no matching function|not satisfied"
#include <concepts>
#include <memory>
template <class P>
concept Pointer = std::default_initializable<P> && std::equality_comparable<P> &&
                  requires(P p) { *p; p.operator->(); };
template <class P>
concept Unique_pointer = Pointer<P> && std::movable<P> && !std::copyable<P>;

template <Unique_pointer P> void adopt(P) {}
void caller() {
    auto shared = std::make_shared<int>(1);
    adopt(shared);                // BAD: a shared_ptr is copyable, so it isn't a unique pointer
}
```

```cpp
#include <concepts>
#include <memory>
template <class P>
concept Pointer = std::default_initializable<P> && std::equality_comparable<P> &&
                  requires(P p) { *p; p.operator->(); };
template <class P>
concept Unique_pointer = Pointer<P> && std::movable<P> && !std::copyable<P>;
template <class P>
concept Shared_pointer = Pointer<P> && std::copyable<P>;

static_assert(Unique_pointer<std::unique_ptr<int>>);   // GOOD: the standard types fit
static_assert(Shared_pointer<std::shared_ptr<int>>);

template <Unique_pointer P> void adopt(P) {}
void caller() { adopt(std::make_unique<int>(1)); }
```

Tested by [`tests/guidelines/gsl_test.cpp`](../../../../tests/guidelines/gsl_test.cpp).
