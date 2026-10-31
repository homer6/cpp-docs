# Lambdas

> Anonymous function objects with captured state — the workhorse of modern C++
> callbacks, algorithm predicates, and ad-hoc closures. **Since:** C++11 (refined
> through C++23)

## Anatomy

```cpp body
int factor = 3;           // a local: only automatic variables can be captured
//      captures   params      -> ret    body
auto f = [factor]  (int x)     -> int   { return x * factor; };
//       └ capture clause: what state the lambda holds
```

A lambda is sugar for a compiler-generated class with an `operator()`. The capture
clause becomes its members.

## Captures

```cpp body
auto ptr = std::make_unique<int>(7);
int a = 1, b = 2;
[a]        { return a; };        // capture a by value (copy)
[&a]       { a = 5; };           // capture a by reference (can mutate caller's a)
[a, &b]    { };                  // mix
[=]        { return a + b; };    // capture everything used, by value
[&]        { a = b; };           // capture everything used, by reference
[=, &b]    { };                  // by value, except b by reference
[x = a + 1]{ return x; };         // init-capture (C++14): a new member
[p = std::move(ptr)]{ };          // move-capture a unique_ptr (C++14)
```

In a member function, `[this]` captures the object pointer; `[*this]` (C++17)
captures a **copy** of the object.

Only local variables with automatic storage are captured. Globals and `static`
locals are used directly, so a lambda that touches them shares their state.
C++20 adds a few more forms:

```cpp
struct Counter {
    int n = 0;
    auto get() { return [=, this] { return n; }; }   // C++20: spell out `this` with [=]
};

template<class... Ts>
auto sum_later(Ts... xs) {
    return [...xs = std::move(xs)] { return (xs + ...); };   // C++20: pack init-capture
}

void bindings() {
    auto [x, y] = std::pair{1, 2};
    auto add = [x, y] { return x + y; };              // C++20: capture structured bindings
    add();
}
```

## Generic, mutable, constexpr

```cpp
auto add = [](auto a, auto b) { return a + b; };       // generic (C++14): templated operator()
auto g   = []<class T>(T x) { return x; };              // explicit template params (C++20)

auto counter = [n = 0]() mutable { return ++n; };       // mutable: modify by-value captures

constexpr auto square = [](int x) { return x * x; };    // usable at compile time
```

## Where they shine

```cpp
struct Player { std::string name; int score; };

void examples(std::vector<Player>& players, std::vector<int>& v, std::map<std::string, int>& m) {
    std::sort(players.begin(), players.end(), [](auto& a, auto& b){ return a.score > b.score; });
    std::ranges::for_each(v, [total = 0](int x) mutable { total += x; });
    std::erase_if(m, [](const auto& kv){ return kv.second == 0; });
}
```

Store one in [`std::function`](../../utilities/functional.md) (type-erased) or
`std::move_only_function` (C++23) when you need a uniform callable type.

## Gotchas

- **`[&]` capture-by-reference dangles** if the lambda outlives the captured
  locals (stored in a `std::function`, returned, run on another thread). Capture
  by value (or move) for anything that escapes the current scope.
- **`[=]` captures `this` by pointer in member functions**, not a copy of
  members — a subtle dangling trap. Since C++20 the implicit capture is
  deprecated (GCC and Clang warn); write `[=, this]` to keep it, `[*this]`
  (C++17) to copy the object, or capture members explicitly.
- **By-value captures are `const` unless `mutable`.** Forgetting `mutable` on a
  stateful counter lambda is a compile error.
- **Each lambda has a unique, unnamable type** — store in `auto`, or
  `std::function`/`move_only_function` when you need a named type or container.
- **Prefer init-capture (`[x = expr]`) to capture computed values or move-only
  types** rather than capturing then reassigning.

## See also

- [Lambda expressions — cppreference](https://en.cppreference.com/w/cpp/language/lambda)
- [std::function — cppreference](https://en.cppreference.com/w/cpp/utility/functional/function)
