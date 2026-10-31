# Modern C++ idioms (C++17 → C++26)

> The features that change how everyday C++ is written, grouped by standard,
> each with the idiom it enables. **Sources:** cppreference · **Targets:** C++26

Each section links to the full reference page in `docs/examples/`. Prefer the
newest idiom your compiler supports — check with the
[feature-test macros](../examples/feature-test-macros/README.md).

## C++17

### Structured bindings

```cpp nocompile
for (const auto& [name, score] : scores) {
    std::println("{}: {}", name, score);
}

auto [it, inserted] = map.try_emplace(key, value);
```

### `if` / `switch` with initializer

Keeps the helper variable scoped to the branch.

```cpp nocompile
if (auto it = map.find(key); it != map.end()) {
    use(it->second);
}
```

### `std::optional` — a value that may be absent

```cpp body
std::optional<int> find_port(std::string_view name);

int port = find_port("http").value_or(80);
if (auto p = find_port("ssh")) use(*p);
```

Reference: [optional](../examples/utilities/optional.md)

### `std::variant` — one of a fixed set of types

```cpp nocompile
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

```cpp nocompile
auto squares_of_evens = v
    | std::views::filter([](int x) { return x % 2 == 0; })
    | std::views::transform([](int x) { return x * x; })
    | std::views::take(3);                     // lazy: nothing computed yet

std::ranges::sort(people, {}, &Person::age);   // projection: sort by age
```

Reference: [views](../examples/ranges/views.md) ·
[execution & ranges algorithms](../examples/algorithms/execution-and-ranges.md)

### `std::span` — a view of contiguous elements

```cpp body
void scale(std::span<double> xs, double k);

std::vector<double> v{1, 2, 3};
double a[] = {4, 5, 6};
scale(v, 2.0);
scale(a, 2.0);
```

Reference: [span](../examples/containers/views/span.md)

### `consteval` and `constexpr` everywhere

```cpp nocompile
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

```cpp nocompile
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

```cpp nocompile
std::expected<int, ParseError> parse_int(std::string_view s);

auto doubled = parse_int(text)
    .transform([](int x) { return x * 2; })
    .or_else([](ParseError) { return std::expected<int, ParseError>{0}; });
```

Reference: [expected](../examples/utilities/expected.md)

### `std::print` / `std::println`

```cpp nocompile
std::println("{} is {:>8.2f}", name, price);
std::println(stderr, "error: {}", msg);
```

Reference: [formatting](../examples/text-processing/formatting.md)

### `std::generator`

```cpp nocompile
std::generator<int> fibonacci() {
    for (int a = 0, b = 1;; std::tie(a, b) = std::pair{b, a + b})
        co_yield a;
}

for (int x : fibonacci() | std::views::take(10)) std::print("{} ", x);
```

### Ranges, finished

```cpp nocompile
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

```cpp needs=contracts
double sqrt_checked(double x)
    pre(x >= 0)
    post(r: r >= 0);

void at(std::size_t index, std::size_t size) {
    contract_assert(index < size);
}
```

Checking is configurable per build (ignore / observe / enforce /
quick-enforce). Use them for preconditions instead of `assert`.

Reference: [contracts](../examples/language/basic-concepts/contracts.md)

### Reflection

```cpp needs=reflection
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

```cpp needs=execution
std::execution::task<int> fetch_and_parse(Url url) {
    auto body = co_await http_get(url);   // any sender can be awaited
    co_return parse(body);
}
```

Large codebases are cautious here: Google allows coroutines only through
libraries approved by project leads, and Chromium bans them for now (see
[industry practice](industry-practice.md)). Choose one async framework for a
codebase before using `task` and senders widely.

The design is still being argued over. Papers from the C++ Alliance (Vinnie
Falco et al.) — written to inform the committee, not to remove `task` — make
these points:

- **Some problems can be fixed later; some can't.**
  [P4007R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4007r3.pdf)
  (May 2026) sorts open issues by whether they can be fixed after C++26 ships.
  Some were already addressed at the Croydon meeting (Mar 2026), which adopted
  P3980R1 (task's allocator use), P3941R4 (scheduler affinity), P3927R2 and
  P4151R1 (per the [libc++ status page](https://libcxx.llvm.org/Status/Cxx26.html)).
  It argues that allocator timing and propagation (for the coroutine
  frame), error return (errors need `co_yield with_error(e)` because
  `co_return` can't carry one), and symmetric transfer (synchronous completions
  can grow the stack) are locked in once `task` ships.
- **The environment parameter limits interoperability.**
  [P4089R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4089r0)
  (Apr 2026) argues that `task<T, Environment>` makes tasks from different
  libraries hard to combine unless every query is known by name.
- **Senders cost more than coroutine-native I/O.**
  [P4123R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4123r0)
  (May 2026) argues that each task-to-task `co_await` must set up per-child
  state through the sender protocol, an overhead the specification mandates.

Practical upshot: `task` is standard, but for performance-critical coroutine
I/O, measure it against a coroutine-native library before standardizing on it.

Reference: [senders/receivers](../examples/execution/senders-receivers.md)

### Smaller language features

```cpp nocompile
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

**Compiler support as of 2026-10-30**, from each vendor's status page
([GCC](https://gcc.gnu.org/projects/cxx-status.html),
[Clang](https://clang.llvm.org/cxx_status.html),
[MSVC](https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance),
last updated 2026-10-16):

| Feature | GCC | Clang | MSVC |
|---|---|---|---|
| Contracts (P2900) | 16 (`__cpp_contracts`) | No | not listed |
| Reflection (P2996) | 16, needs `-freflection` | No | not listed |
| Expansion statements, `template for` (P1306) | 16 | 23, partial (iterating expansion statements not yet supported) | not listed |
| Erroneous behaviour (P2795) | 16 | No | not listed |

GCC still labels its C++26 support experimental, and Clang's overall C++2c
status is "Partial" (`-std=c++2c`). Microsoft's conformance page has no C++26
core-language rows yet — its tables stop at C++23 (enabled with
`/std:c++latest`) — so "not listed" means no published support, not a
confirmed "no".

**Library support:**

| Facility | libstdc++ (GCC 16) | libc++ | MSVC STL |
|---|---|---|---|
| `std::optional<T&>` | yes (experimental) | complete | not checked |
| `std::function_ref`, `std::copyable_function` | yes (experimental) | `function_ref` complete | not checked |
| `std::inplace_vector` | yes (experimental) | not checked | not checked |
| `std::indirect` / `std::polymorphic` | yes (experimental) | not marked complete | not checked |
| `std::simd` | yes (experimental) | not checked | not checked |
| Hardening (P3471) | not listed | not marked complete | partial since VS 2022 17.14 (May 2025): opt-in with `_MSVC_STL_HARDENING=1`, off by default |
| Reflection `<meta>`, `std::execution::task` | not listed | not marked complete | not checked |

Sources: [GCC 16 release notes](https://gcc.gnu.org/gcc-16/changes.html)
("improved experimental support for C++26"),
[libc++ C++26 status](https://libcxx.llvm.org/Status/Cxx26.html). Microsoft
tracks its C++26 library work on the
[STL C++26 Features project board](https://github.com/orgs/microsoft/projects/1143/views/2),
not on the conformance page. A "done" card there isn't a shipping version;
use the [STL release notes](https://github.com/microsoft/STL/releases) or the
[changelog](https://github.com/microsoft/STL/wiki/Changelog) for that.

**MSVC STL hardening.** The
[VS 2022 17.14 release](https://github.com/microsoft/STL/releases/tag/vs-2022-17.14)
merged P3471 as a *partial* C++26 feature: off by default, enabled for any
`/std` mode by defining `_MSVC_STL_HARDENING` to `1` project-wide. Because
MSVC doesn't implement C++26 contracts yet
([microsoft/STL#5286](https://github.com/microsoft/STL/issues/5286), open), a
violated hardened precondition calls `__fastfail()`. Per-container macros
(`_MSVC_STL_HARDENING_VECTOR`, …) let you opt individual classes in or out
([STL Hardening wiki](https://github.com/microsoft/STL/wiki/STL-Hardening)).
VS 2026 18.0 (MSVC Build Tools 14.50) added P3697R1 (minor additions to
hardening) and made the checks cheaper. Microsoft plans to turn hardening on by
default in a later release; check the changelog for the version you use.

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
