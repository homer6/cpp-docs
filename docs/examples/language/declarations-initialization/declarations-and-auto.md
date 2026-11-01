# Declarations, `auto` & type deduction

> Naming things with types — and letting the compiler deduce the type for you.

## auto — deduce the type from the initializer

```cpp body
using namespace std::string_literals;    // for the ""s suffix
int x = 0;
std::map<std::string, int> m;

auto i = 42;                 // int
auto d = 3.14;               // double
auto s = "hi"s;              // std::string
auto& r = x;                 // reference
const auto& cr = x;          // const reference (no copy)
auto* p = &x;                // pointer

for (const auto& [k, v] : m) {}   // with structured bindings
auto lambda = [](int x){ return x*x; };   // lambdas have no spellable type — auto is required
```

`auto` strips top-level `const` and references **unless you write them**:
`auto x = cref;` copies (drops const/ref); `auto& x = cref;` keeps them.

## decltype — the declared type of an expression

```cpp
int x = 0;
decltype(x)   a;        // int
decltype((x)) b = x;    // int&  — extra parens → the expression is an lvalue
decltype(auto) f();      // deduce return type *preserving* ref/const (vs plain auto)
```

`decltype(auto)` is the tool for perfect-forwarding return types (keeps
references that `auto` would strip).

## Trailing return types & deduced returns

```cpp
auto add(int a, int b) -> int;          // trailing return type
auto multiply(int a, int b) { return a*b; }   // return type deduced (C++14)

template<class T, class U>
auto sum(T t, U u) -> decltype(t + u);   // return type depends on params
```

## Structured bindings (C++17)

Decompose a tuple/pair/array/aggregate into named pieces:

```cpp
struct Point { int x, y; };
Point pt{1, 2};

std::pair<int, std::string> p{1, "one"};
auto [id, name] = p;                       // id=1, name="one"
auto& [a, b] = pt;                         // bind by reference to mutate
auto [x, y, z] = std::array{1, 2, 3};
```

A structured binding can also be the condition of an `if`, `while` or `switch`
(C++26); see [selection statements](../statements/selection.md#structured-binding-as-a-condition-c26).

### `_` — the placeholder name (C++26)

A variable named `_` says "I don't need this". Unlike any other name, `_` can be
declared again in the same scope, and an unused one draws no warning:

```cpp body
auto [x, _] = std::pair{1, 2.0};    // ignore the second element
auto [y, _] = std::pair{3, 4.0};    // a second `_` in the same scope: fine
static std::mutex m;
std::lock_guard _{m};               // keep a guard alive without naming it
```

Once a scope holds two `_`s, using `_` is ambiguous:

```cpp body compile-fail="ambiguous"
int _ = 1;
int _ = 2;
int z = _;       // ERROR: which `_`?
```

### Binding a pack (C++26)

`...name` binds any number of elements (zero or more) to a pack you can expand.
It's allowed only inside a template (a function template or generic lambda), and
needs GCC 16 or Clang 22; GCC 15 rejects it.

```cpp needs=structured-binding-packs
template <class Tuple>
constexpr auto sum(const Tuple& t) {
    auto [...xs] = t;                  // every element
    return (xs + ... + 0);
}
static_assert(sum(std::tuple{1, 2, 3}) == 6);

constexpr auto head_and_count = [](auto t) {
    auto [head, ...rest] = t;          // the first, then the rest
    return head * 100 + sizeof...(rest);
};
static_assert(head_and_count(std::tuple{1, 2, 3}) == 102);
```

### `constexpr` structured bindings (C++26)

The bindings become constants. A `constexpr auto&` binding must refer to an
object with static storage. GCC 16 and Clang 22 have it; GCC 15 doesn't.

```cpp needs=constexpr-structured-bindings
struct Point { int x, y; };
constexpr Point origin{3, 4};
constexpr auto [ox, oy] = origin;
static_assert(ox + oy == 7);

constexpr std::array table{1, 2, 3};
constexpr auto& [t0, t1, t2] = table;  // refers to a static object
static_assert(t2 == 3);
```

## Gotchas

- **`auto` drops references and top-level const.** `auto x = vec[0];` copies; use
  `auto&` / `const auto&` to bind without copying.
- **`auto&&` is a forwarding reference** in deduced context (binds to lvalue or
  rvalue) — not the same as a plain rvalue ref. See
  [value categories](../expressions/value-categories.md).
- **`decltype(x)` vs `decltype((x))` differ:** the parenthesized form yields a
  reference type because `(x)` is an lvalue expression. Easy to trip over.
- **Don't overuse `auto` where the type aids readability** — `auto total = 0;`
  hides that it's an `int` (and can cause integer/float surprises). It shines for
  iterators, lambdas, and verbose template types.
- **A structured binding pack works only inside a template.** At namespace
  scope or in a plain function, `auto [...xs] = t;` is an error.
- **`_` is only special when it's declared again.** A single `_` is an ordinary
  variable you can read; after a second `_` in the same scope, naming it is an error.
- **Almost-Always-Auto vs clarity** is a style call; be consistent with the
  codebase.

## See also

- [auto specifier — cppreference](https://en.cppreference.com/w/cpp/language/auto)
- [decltype — cppreference](https://en.cppreference.com/w/cpp/language/decltype)
- [Structured bindings — cppreference](https://en.cppreference.com/w/cpp/language/structured_binding)
- [P2169: a nice placeholder with no name](https://wg21.link/P2169) ·
  [P1061: structured bindings can introduce a pack](https://wg21.link/P1061) ·
  [P2686: constexpr structured bindings](https://wg21.link/P2686)
