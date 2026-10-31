# Modern C++ idioms (C++17 → C++26)

> The features that change how everyday C++ is written, grouped by standard,
> each with the idiom it enables. **Sources:** cppreference · **Targets:** C++26

Each section links to the full reference page in `docs/examples/`. Prefer the
newest idiom your compiler supports — check with the
[feature-test macros](../examples/feature-test-macros/README.md).

## C++17

### Structured bindings

```cpp
for (const auto& [name, score] : scores) {
    std::println("{}: {}", name, score);
}

auto [it, inserted] = map.try_emplace(key, value);
```

### `if` / `switch` with initializer

Keeps the helper variable scoped to the branch.

```cpp
if (auto it = map.find(key); it != map.end()) {
    use(it->second);
}
```

### `std::optional` — a value that may be absent

```cpp
std::optional<int> find_port(std::string_view name);

int port = find_port("http").value_or(80);
if (auto p = find_port("ssh")) use(*p);
```

Reference: [optional](../examples/utilities/optional.md)

### `std::variant` — one of a fixed set of types

```cpp
using Shape = std::variant<Circle, Square, Triangle>;

template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

double area(const Shape& s) {
    return std::visit(overloaded{
        [](const Circle& c)   { return std::numbers::pi * c.r * c.r; },
        [](const Square& q)   { return q.side * q.side; },
        [](const Triangle& t) { return 0.5 * t.base * t.height; },
    }, s);
}
```

Every alternative must be handled, or it doesn't compile. Use `std::get_if`
rather than an unchecked `std::get`.

Reference: [variant](../examples/utilities/variant.md)

### `std::string_view` — a read-only string parameter

Accepts literals, `std::string`, and slices without allocating. Never store one
past the life of the string it views.

Reference: [string_view](../examples/strings/string_view.md)

### `if constexpr`

```cpp
template <typename T>
auto twice(T value) {
    if constexpr (std::is_integral_v<T>) return value * 2;
    else if constexpr (std::is_floating_point_v<T>) return value * 2.0;
    else static_assert(false, "unsupported type");   // OK since C++23 (DR)
}
```

## C++20

### Concepts

```cpp
template <typename T>
concept Hashable = requires(T t) {
    { std::hash<T>{}(t) } -> std::convertible_to<std::size_t>;
};

void add(Hashable auto const& value);          // abbreviated template

template <typename T>
    requires std::default_initializable<T> && std::movable<T>
T make();
```

Reference: [standard concepts](../examples/concepts/standard-concepts.md)

### Ranges

```cpp
auto squares_of_evens = v
    | std::views::filter([](int x) { return x % 2 == 0; })
    | std::views::transform([](int x) { return x * x; })
    | std::views::take(3);                     // lazy: nothing computed yet

std::ranges::sort(people, {}, &Person::age);   // projection: sort by age
```

Reference: [views](../examples/ranges/views.md) ·
[execution & ranges algorithms](../examples/algorithms/execution-and-ranges.md)

### `std::span` — a view of contiguous elements

```cpp
void scale(std::span<double> xs, double k);

std::vector<double> v{1, 2, 3};
double a[] = {4, 5, 6};
scale(v, 2.0);
scale(a, 2.0);
```

Reference: [span](../examples/containers/views/span.md)

### `consteval` and `constexpr` everywhere

```cpp
consteval int square(int n) { return n * n; }   // must run at compile time

constexpr int s = square(5);    // OK
int n = 5;
int t = square(n);              // error: n isn't a constant
```

`constexpr` functions may now use `std::vector` and `std::string` internally —
but the allocation must be freed before compile-time evaluation ends, so you
can't store one in a `constexpr` variable.

```cpp
constexpr int sum_first(int n) {
    std::vector<int> v;
    for (int i = 1; i <= n; ++i) v.push_back(i);
    return std::ranges::fold_left(v, 0, std::plus{});   // vector freed here
}
static_assert(sum_first(4) == 10);
```

### Three-way comparison

```cpp
struct Version {
    int major, minor, patch;
    auto operator<=>(const Version&) const = default;   // also gives ==
};
```

Reference: [comparison](../examples/language-support/comparison.md)

### Designated initializers and `using enum`

```cpp
Config cfg{ .width = 1920, .height = 1080, .fullscreen = true };

switch (color) {
    using enum Color;
    case red:   break;
    case green: break;
    case blue:  break;
}
```

### Lambdas

```cpp
auto p = std::make_unique<Widget>();
auto task = [w = std::move(p)] { w->run(); };          // init capture (C++14)

auto size_of = []<typename T>(const std::vector<T>& v) { return v.size(); };

template <typename... Args>
auto bind_later(Args... args) {
    return [...args = std::move(args)] { return call(args...); };   // pack capture
}
```

Reference: [lambdas](../examples/language/functions/lambdas.md)

### Coroutines and modules

Coroutines (`co_await`, `co_yield`, `co_return`) are a language mechanism; you
normally use them through a library type such as `std::generator` (C++23).
Modules replace headers with `export module m;` / `import m;`.

Reference: [coroutines](../examples/language/coroutines/coroutines.md) ·
[modules](../examples/language/modules/modules.md)

## C++23

### `std::expected` — a value or an error

```cpp
std::expected<int, ParseError> parse_int(std::string_view s);

auto doubled = parse_int(text)
    .transform([](int x) { return x * 2; })
    .or_else([](ParseError) { return std::expected<int, ParseError>{0}; });
```

Reference: [expected](../examples/utilities/expected.md)

### `std::print` / `std::println`

```cpp
std::println("{} is {:>8.2f}", name, price);
std::println(stderr, "error: {}", msg);
```

Reference: [formatting](../examples/text-processing/formatting.md)

### `std::generator`

```cpp
std::generator<int> fibonacci() {
    for (int a = 0, b = 1;; std::tie(a, b) = std::pair{b, a + b})
        co_yield a;
}

for (int x : fibonacci() | std::views::take(10)) std::print("{} ", x);
```

### Ranges, finished

```cpp
auto names = people
    | std::views::transform(&Person::name)
    | std::ranges::to<std::vector>();            // materialize a pipeline

auto total = std::ranges::fold_left(prices, 0.0, std::plus{});
bool has_admin = std::ranges::contains(roles, Role::admin);

for (auto [i, x] : std::views::enumerate(v)) { /* ... */ }
for (auto [a, b] : std::views::zip(xs, ys))  { /* ... */ }
```

### Deducing `this`

One member function instead of `const`/non-`const`/`&&` overloads; also enables
recursive lambdas.

```cpp
struct Buffer {
    std::vector<char> data;
    auto&& bytes(this auto&& self) { return std::forward<decltype(self)>(self).data; }
};

auto fact = [](this auto self, int n) -> int { return n <= 1 ? 1 : n * self(n - 1); };
```

### Also

- `if consteval { … } else { … }` — pick a compile-time or run-time path.
- `import std;` — the whole standard library as one module.
- `std::flat_map` / `std::flat_set`, `std::mdspan`, `std::stacktrace`.

## C++26

### Contracts

```cpp
double sqrt_checked(double x)
    pre(x >= 0)
    post(r: r >= 0);

contract_assert(index < size);
```

Checking is configurable per build (ignore / observe / enforce /
quick-enforce). Use them for preconditions instead of `assert`.

Reference: [contracts](../examples/language/basic-concepts/contracts.md)

### Reflection

```cpp
template <typename E>
    requires std::is_enum_v<E>
constexpr std::string_view to_string(E value) {
    template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E)))
        if (value == [:e:]) return std::meta::identifier_of(e);
    return "<unknown>";
}
```

`^^` turns an entity into a value you can query; `[: … :]` splices it back into
code; `template for` expands over a compile-time range. This replaces most
macro and template-metaprogramming tricks for enums, serialization, and
boilerplate.

Reference: [reflection (language)](../examples/language/expressions/reflection.md) ·
[`<meta>`](../examples/metaprogramming/reflection.md)

### `std::execution` (senders/receivers)

The standard model for asynchronous and parallel work: describe work as
composable senders, run it on a scheduler. C++26 also adds
`std::execution::task` (P3552), a coroutine type that is itself a sender, so
you can write async code with `co_await` and still compose it with other
senders.

```cpp
std::execution::task<int> fetch_and_parse(Url url) {
    auto body = co_await http_get(url);   // any sender can be awaited
    co_return parse(body);
}
```

Large codebases are cautious here: Google allows coroutines only through
libraries approved by project leads, and Chromium bans them for now (see
[industry practice](industry-practice.md)). Choose one async framework for a
codebase before using `task` and senders widely.

Reference: [senders/receivers](../examples/execution/senders-receivers.md)

### Smaller language features

```cpp
auto [_, inserted] = set.insert(x);          // `_` placeholder, reusable
void f(int) = delete("use f(long) instead"); // reason in the error message
template <typename... Ts> using First = Ts...[0];   // pack indexing
if (auto [ok, value] = try_parse(s)) { }     // structured binding as condition
constexpr unsigned char icon[] = {
    #embed "icon.png"                        // binary file as data
};
```

### Library

| Facility | Use it for |
|---|---|
| `std::optional<T&>` | an optional reference, instead of `T*` |
| `std::inplace_vector<T, N>` | a vector with fixed capacity and no heap |
| `std::function_ref` | a non-owning callable parameter (cheaper than `std::function`) |
| `std::indirect<T>` / `std::polymorphic<T>` | value-semantic heap objects (copyable, unlike `unique_ptr`) |
| `std::views::concat` | join several ranges lazily |
| `std::add_sat`, `std::sub_sat`, … | arithmetic that saturates instead of overflowing |
| Hardened library | checked `operator[]` and friends — see [safety](safety.md#c26-hardened-standard-library) |

Reference: [inplace_vector](../examples/containers/sequence/inplace_vector.md)

**Not in C++26:** trivial relocatability (P2786,
`trivially_relocatable_if_eligible`) was voted in in Feb 2025 and then
**reverted** at Kona in Nov 2025 (P3920). Don't use that spelling.

## Adopting C++26 today

A compiler's `-std=c++26` switch doesn't mean every C++26 feature works.
Language support and library support move separately, and a feature can be
present but experimental.

**Compiler support as of 2026-10-30** (GCC from its
[status page](https://gcc.gnu.org/projects/cxx-status.html)):

| Feature | GCC | Notes |
|---|---|---|
| Contracts (P2900) | 16 | `__cpp_contracts` |
| Reflection (P2996) | 16 | needs `-freflection` |
| Expansion statements, `template for` (P1306) | 16 | |
| Erroneous behaviour (P2795) | 16 | |

GCC still labels its C++26 support experimental. Clang and MSVC weren't
checked for this table — see their status pages before relying on a feature.

**Library support** moves more slowly. libc++'s
[C++26 status page](https://libcxx.llvm.org/Status/Cxx26.html) lists
`std::optional<T&>` and `std::function_ref` as complete, while contracts
support, reflection's `<meta>`, hardening (P3471), `indirect`/`polymorphic`,
and `std::execution::task` were not yet marked complete.

**A sensible adoption order:**

1. **Library hardening — now.** It works in C++20/23 builds and needs no code
   changes (see [safety](safety.md#hardening-in-practice)).
2. **Small library additions** (`optional<T&>`, `function_ref`,
   `inplace_vector`) once your standard library ships them — they're easy to
   swap back out.
3. **Contracts** — once your toolchain supports them, with a build-wide policy
   for which semantic each build uses.
4. **Reflection** — behind a small, contained interface (e.g. one
   `to_string(enum)` helper) while support is experimental.
5. **`std::execution` / `task`** — after choosing your async framework; it
   shapes every async API you write.

Gate each one with its feature-test macro, and keep features your users'
toolchains may not have out of public headers.

## See also

- [Core Guidelines digest](core-guidelines.md) ·
  [Design principles](design-principles.md)
- [C++26 — cppreference](https://en.cppreference.com/w/cpp/26) ·
  [C++23](https://en.cppreference.com/w/cpp/23) ·
  [C++20](https://en.cppreference.com/w/cpp/20)
