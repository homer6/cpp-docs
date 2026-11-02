# FAQ: Answers to frequently asked questions — examples

> Our examples for the [FAQ entries](../faq.md). Most are questions about
> the Guidelines project and the GSL, not coding rules, so they get no code;
> where an answer is about how a type behaves, the code shows it.

## FAQ.1: What do these guidelines aim to achieve?

*No example: a question about the Guidelines project's goals, not a coding rule.*

## FAQ.2: When and where was this work first announced?

*No example: a question about the project's history, not a coding rule.*

## FAQ.3: Who are the authors and maintainers of these guidelines?

*No example: a question about the project's people, not a coding rule.*

## FAQ.4: How can I contribute?

*No example: a question about contributing to the project, not a coding rule.*

## FAQ.5: How can I become an editor/maintainer?

*No example: a question about the project's governance, not a coding rule.*

## FAQ.6: Have these guidelines been approved by the ISO C++ standards committee? Do they represent the consensus of the committee?

*No example: a question about the project's standing with WG21, not a coding rule.*

## FAQ.7: If these guidelines are not approved by the committee, why are they under `github.com/isocpp`?

*No example: a question about where the project is hosted, not a coding rule.*

## FAQ.8: Will there be a C++98 version of these Guidelines? A C++11 version?

*No example: a question about which standards the project targets, not a coding rule.*

## FAQ.9: Do these guidelines propose new language features?

*No example: a question about the project's scope, not a coding rule.*

## FAQ.10: What version of Markdown do these guidelines use?

*No example: a question about the document's Markdown format, not about C++.*

## FAQ.50: What is the GSL (guidelines support library)?

*No example: a question about what the GSL is; the GSL entries and their examples are in [gsl.md](gsl.md).*

## FAQ.51: Is github.com/Microsoft/GSL the GSL?

*No example: a question about which implementations of the GSL exist, not a coding rule.*

## FAQ.52: Why not supply an actual GSL implementation in/with these guidelines?

*No example: a question about the project's decision not to ship an implementation, not a coding rule.*

## FAQ.53: Why weren't the GSL types proposed through Boost?

*No example: a question about the project's history, not a coding rule.*

## FAQ.54: Has the GSL (guidelines support library) been approved by the ISO C++ standards committee?

*No example: a question about the GSL's standing with WG21, not a coding rule.*

## FAQ.55: If you're using the standard types where available, why is the GSL `span<char>` different from the `string_view` in the Library Fundamentals 1 Technical Specification and C++17 Working Paper? Why not just use the committee-approved `string_view`?

The answer is about access: a `string_view` only reads its characters, while
a span of non-`const` `char` can write them. Pick `std::string_view` to read
text and `std::span<char>` to change it in place.

```cpp compile-fail="read-only|const-qualified|cannot assign"
#include <string_view>
void shout(std::string_view s) {
    if (!s.empty()) s[0] = 'H';   // BAD: a string_view's characters are read-only
}
```

```cpp
#include <span>
void shout(std::span<char> s) {
    if (!s.empty()) s[0] = 'H';   // GOOD: a span<char> can write through
}
```

Tested by [`tests/guidelines/faq_test.cpp`](../../../../tests/guidelines/faq_test.cpp).

## FAQ.56: Is `owner` the same as the proposed `observer_ptr`?

No: they mark opposite things. The GSL's `owner<T*>` is only an alias for
`T*` that tells readers and tools "this pointer owns its object"; the type
doesn't change, so it adds no run-time behaviour. In standard C++ you'd
normally say ownership with a type instead, `std::unique_ptr`.

```cpp nocompile
// BAD: compiles; nothing says that the caller must delete the result.
Widget* make_widget() { return new Widget; }
```

```cpp
#include <memory>
#include <type_traits>
template <class T> using owner = T;                 // what gsl::owner is: just a label
static_assert(std::is_same_v<owner<int*>, int*>);   // the type is unchanged

owner<Widget*> make_widget_raw() { return new Widget; }                    // OK: at least it's labelled
std::unique_ptr<Widget> make_widget() { return std::make_unique<Widget>(); }   // GOOD: ownership in the type
```

Tested by [`tests/guidelines/faq_test.cpp`](../../../../tests/guidelines/faq_test.cpp).

## FAQ.57: Is `stack_array` the same as the standard `array`?

No. A `std::array` holds its elements inside itself, but the `std::array`
object can live anywhere: on the stack, inside another object, or on the heap.
Only where you create it decides where its elements are.

```cpp
#include <array>
#include <memory>
void where() {
    std::array<int, 4> on_stack{};                            // elements on the stack
    auto on_heap = std::make_unique<std::array<int, 4>>();    // the same type, on the heap
    use(on_stack, on_heap);
}
```

Tested by [`tests/guidelines/faq_test.cpp`](../../../../tests/guidelines/faq_test.cpp).

## FAQ.58: Is `dyn_array` the same as `vector` or the proposed `dynarray`?

No: the GSL's `dyn_array` is a heap array whose size is fixed when it's
created, meant to replace `new T[n]`. Standard C++ has no such container; the
nearest are `std::vector` (which can also grow) and
`std::make_unique<T[]>(n)` (fixed size, but it doesn't remember the size).
Either is better than a naked `new[]`.

```cpp nocompile
// BAD: compiles; the delete[] is skipped if fill() throws, and the size travels separately.
void fill(int* p, std::size_t n);
void work(std::size_t n) {
    int* p = new int[n];
    fill(p, n);
    delete[] p;
}
```

```cpp
#include <memory>
#include <span>
#include <vector>
void fill(std::span<int> s);
void work(std::size_t n) {
    std::vector<int> v(n);                       // GOOD: sized at run time, freed for us
    fill(v);
    auto fixed = std::make_unique<int[]>(n);     // GOOD: fixed size; freed for us
    fill(std::span<int>(fixed.get(), n));
}
```

## FAQ.59: Is `Expects` the same as `assert`?

No. `Expects` marks a precondition, a promise about the arguments that
belongs in the function's declaration; `assert` is just a check somewhere in
the body. C++26 adds language contracts, so a precondition can now be written
with `pre`, where callers and tools can see it.

```cpp nocompile
#include <cassert>
// BAD: compiles, but the precondition is hidden in the body, invisible to callers and tools.
double ratio(double a, double b) {
    assert(b != 0);
    return a / b;
}
```

```cpp needs=contracts
double ratio(double a, double b)
    pre(b != 0)                       // GOOD: the precondition is part of the declaration
{
    return a / b;
}
```

## FAQ.60: Is `Ensures` the same as `assert`?

No. `Ensures` marks a postcondition, a promise about the result. C++26
writes it with `post`, which can name the return value, so it sits on the
declaration instead of in front of every `return`.

```cpp nocompile
#include <cassert>
// BAD: compiles, but the promise is checked only on this one return path, inside the body.
int clamp_percent(int x) {
    int r = x < 0 ? 0 : (x > 100 ? 100 : x);
    assert(r >= 0 && r <= 100);
    return r;
}
```

```cpp needs=contracts
int clamp_percent(int x)
    post(r : r >= 0 && r <= 100)      // GOOD: the promise is part of the declaration
{
    return x < 0 ? 0 : (x > 100 ? 100 : x);
}
```
