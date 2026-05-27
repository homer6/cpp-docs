# Namespaces & name lookup

> Group names to avoid collisions, and understand how the compiler finds the name
> you wrote — including the surprising-but-essential **ADL**.

## Declaring and using

```cpp
namespace app {
    int version();
    namespace net { void connect(); }      // nested
}

app::version();                              // qualified name
app::net::connect();

namespace fs = std::filesystem;              // namespace alias (shorten long names)

using app::version;                          // using-declaration: bring ONE name in
using namespace app;                          // using-directive: bring everything in (use sparingly)
```

## Anonymous (unnamed) namespaces — internal linkage

```cpp
namespace {
    int helper() { return 42; }              // visible only in this translation unit
}
```

The modern replacement for file-scope `static` — and it works for types too. See
[scope & linkage](../basic-concepts/scope-storage-linkage.md).

## inline namespaces — versioning

```cpp
namespace lib {
    inline namespace v2 { void api(); }      // lib::api() resolves to lib::v2::api()
    namespace v1         { void api(); }      // still reachable as lib::v1::api()
}
lib::api();                                   // picks the inline (v2) by default
```

Members of an `inline namespace` act as if they're in the enclosing namespace —
the mechanism behind ABI versioning (and `std::literals` sub-namespaces).

## Name lookup & ADL

To resolve an unqualified name, the compiler searches enclosing scopes — and for
**function calls**, also **argument-dependent lookup (ADL)**: it looks in the
namespaces of the argument types.

```cpp
std::vector<int> v{3, 1, 2};
sort(v.begin(), v.end());        // unqualified `sort` found via ADL (arg is in namespace std)

std::cout << x;                   // operator<< found via ADL in std — why `using std::operator<<` is unneeded
```

ADL is why `swap(a, b)` and range-based code can find the *right* overload without
qualification — and why you provide a free `swap`/`begin` in your type's namespace.

## Gotchas

- **Avoid `using namespace std;` (especially in headers).** It dumps every `std`
  name into scope, causing ambiguities and surprising overload picks. Prefer
  qualified names or targeted `using std::vector;`.
- **ADL is powerful and occasionally surprising.** An unqualified call can pull in
  an overload from an argument's namespace you didn't expect; qualify
  (`std::move`, `ns::f`) when you need a specific one. The "hidden friend" idiom
  (a `friend` function defined in-class) is found *only* by ADL — intentional.
- **`using namespace` in a header leaks into every includer** — never do it at
  namespace scope in headers.
- **Inline namespaces change mangled names** — adding/removing `inline` is an ABI
  break.

## See also

- [Namespaces — cppreference](https://en.cppreference.com/w/cpp/language/namespace)
- [Argument-dependent lookup (ADL) — cppreference](https://en.cppreference.com/w/cpp/language/adl)
- [Unqualified/qualified name lookup — cppreference](https://en.cppreference.com/w/cpp/language/lookup)
