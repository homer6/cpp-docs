# Design principles — composable, hard-to-misuse C++

> A design style built on algebraic types, algorithms instead of loops, and
> APIs the compiler can check. **Source:** Ben Deane's talks — *Easy to Use,
> Hard to Misuse*, *Using Types Effectively*, and *Composable C++* — as applied
> in the automerge-cpp project

The guiding question is always: **can the compiler reject the mistake?** If a
caller can misuse an API in a way that compiles, redesign the API.

## 1. Types model the domain

### Make illegal states unrepresentable

If fields are mutually exclusive, they belong in a `std::variant`, not in a
struct with a tag and fields that are "only valid when…".

```cpp
// BAD — nothing stops { .state = disconnected, .connection_id = 7 }
struct Connection {
    enum State { disconnected, connected, error } state;
    int connection_id;       // only valid when connected
    std::string error_msg;   // only valid when error
};

// GOOD — each state carries exactly its own data
struct Disconnected {};
struct Connected       { int connection_id; };
struct ConnectionError { std::string message; };

using ConnectionState = std::variant<Disconnected, Connected, ConnectionError>;
```

This is the **sum type** (variant: *one of*) and **product type** (struct:
*all of*) vocabulary. Most domain models are a few of each, nested.

### Strong types for identifiers

Never pass a raw `int`, `uint64_t`, or `std::string` where the value has a
domain meaning. Wrap it so swapping two arguments is a compile error.

```cpp
// BAD — which is which?
void apply(std::uint64_t actor, std::uint64_t counter, std::uint64_t op);

// GOOD
struct ActorId { std::array<std::byte, 16> bytes; };
struct OpId    { std::uint64_t counter; ActorId actor; };
```

A strong type usually wants:

- defaulted `operator<=>` (and so `==`)
- a `std::hash` specialization if it goes in unordered containers
- a `std::formatter` specialization (or `operator<<`) for logging

### Closed sets use `std::variant`

When the set of alternatives is fixed and known, use `std::variant` — not an
inheritance hierarchy, `void*`, or `std::any`.

```cpp
using ScalarValue = std::variant<
    std::monostate, bool, std::int64_t, std::uint64_t, double, std::string
>;
```

Use virtual functions (or concepts) only when the set is genuinely *open* —
when users must be able to add new types.

### Vocabulary types say what you mean

| Situation | Use | Not |
|---|---|---|
| Value might be absent | `std::optional<T>` | `T*`, `-1`, `""` |
| Operation might fail | `std::expected<T, Error>` | error codes, out-params |
| One of N alternatives | `std::variant<A, B, C>` | `union`, enum + `void*` |
| View of contiguous data | `std::span<T>` | `T*` + `size_t` |
| View of a string | `std::string_view` | `const char*` |

## 2. No raw loops

Express traversals with named algorithms, ranges pipelines, and folds. The name
states the intent, off-by-one errors disappear, and the structure of the
computation becomes visible (which is what makes it parallelizable).

### Prefer named algorithms

```cpp body
struct Item { int id; };
std::vector<Item> items;
int target = 42;

// BAD — you have to read the body to learn it's a search
bool found = false;
for (const auto& item : items) {
    if (item.id == target) { found = true; break; }
}

// GOOD
bool found1 = std::ranges::any_of(items, [&](const auto& i) { return i.id == target; });
// or, with a projection:
bool found2 = std::ranges::contains(items, target, &Item::id);   // C++23
```

### Pipelines for composition

```cpp
struct Entry { bool is_active() const; std::string name() const; };
std::string to_upper(std::string_view);
std::vector<Entry> entries;

void bad() {
    std::vector<std::string> result;
    for (const auto& e : entries)
        if (e.is_active()) result.push_back(to_upper(e.name()));
}

// GOOD
auto result = entries
    | std::views::filter(&Entry::is_active)
    | std::views::transform([](const auto& e) { return to_upper(e.name()); })
    | std::ranges::to<std::vector>();
```

### Folds, and monoids

Recognize when an operation is a **fold**. If the binary operation is
**associative** and has an **identity** element, it's a **monoid** — and a monoid
can be split into chunks, folded in parallel, and the partial results combined.

```cpp needs=pstl
#include <execution>
std::vector<int> values{1, 2, 3};

// a fold, in order
auto total = std::ranges::fold_left(values, 0, std::plus{});

// a parallel reduction
auto total2 = std::reduce(std::execution::par, values.begin(), values.end(), 0);
```

> **Gotcha:** `std::reduce` may regroup *and reorder* elements, so it needs an
> operation that is associative **and commutative**. Floating-point `+` is
> neither exactly, so parallel sums of `double` can differ in the last bits.
> `fold_left` keeps the order.

Spotting monoids is a design tool, not just an optimization. In automerge-cpp,
merging two CRDT documents is associative, commutative, and idempotent, with the
empty document as identity — so merging many replicas is a reduction, and
"merge is a monoid" becomes a set of laws to test:

```cpp
struct Document { void merge(const Document&); };
std::vector<Document> docs;

auto merged = std::reduce(docs.begin(), docs.end(), Document{},
    [](Document a, const Document& b) { a.merge(b); return a; });
```

### When a loop is fine

- Performance-critical code with no standard algorithm, where a lambda would
  hide the intent more than a loop does.
- Tests, where plain clarity wins.

Even then, extract the loop body into a named function.

### Algorithms first; pipelines are a separate choice

"Use a named algorithm" and "use lazy range pipelines" are different
decisions. Chromium allows algorithms but bans most range adaptors and the `|`
operator over build time, run-time cost, and design concerns (see
[industry practice](industry-practice.md)). If you use pipelines, watch for:

- **Dangling:** a view holds references into its source — don't return a view
  over a local, and don't store one past its source's lifetime.
- **Repeated work:** some views re-evaluate their function each time they're
  iterated (e.g. `filter` + `transform` iterated twice runs the lambda twice).
  Materialize with `std::ranges::to` when you'll reuse the result.
- **Hidden cost:** check that a pipeline in a hot loop compiles to what you
  expect.

## 3. Errors in the type system

| Kind of failure | Use |
|---|---|
| Expected, caller should handle (I/O, parsing, validation, external data) | return `std::expected<T, Error>` |
| Not an error, just absent (lookup miss) | return `std::optional<T>` |
| Programmer error (violated precondition, broken invariant) | contract / assertion |

```cpp
struct Document {};
struct Error { std::string message; };
auto load(std::span<const std::byte> data) -> std::expected<Document, Error>;

void open(std::span<const std::byte> bytes) {
    auto doc = load(bytes);
    if (!doc) {
        std::println(stderr, "load failed: {}", doc.error().message);
        return;
    }
    use(*doc);
}
```

```cpp needs=contracts
struct ObjId {}; struct ScalarValue {}; std::size_t length(ObjId);

void set(ObjId obj, std::size_t index, ScalarValue v)
    pre(index < length(obj));          // C++26; use assert() before that
```

A library may go further and **not use exceptions at all** for expected
failures, leaving only `std::bad_alloc` from the standard library. That's a
per-project choice — the Core Guidelines (E.2) favour exceptions for "can't do
its job" failures, while Google, Chromium, and LLVM don't use exceptions at all
(see [industry practice](industry-practice.md)). What matters is picking one
strategy (E.1) and making the failure visible in the signature.

Be explicit about *which kind* of failure each path is: a violated
precondition, a cancelled operation, an environmental error (file missing), and
a domain error (invalid input) deserve different handling, even if several can
be carried in the same error type.

## 4. Value semantics and immutability

### Prefer regular types

Types should be copyable, movable, and comparable — they behave like `int`.
If copying is expensive, make the type cheap to move (`noexcept` move
operations) and document it.

```cpp
using ActorId = std::array<std::uint8_t, 16>;

struct OpId {
    std::uint64_t counter;
    ActorId actor;
    auto operator<=>(const OpId&) const = default;
};
```

### `const` by default

```cpp
struct Config { static Config defaults(); static Config from_file(std::string_view); };
int compute();

void init(bool use_defaults, std::string_view path) {
    const auto result = compute();          // can't be accidentally reassigned

    const auto config = [&] {               // complex init, still const
        if (use_defaults) return Config::defaults();
        return Config::from_file(path);
    }();
}
```

**But not for data members of value types.** A `const` or reference data
member deletes the type's assignment operators, so it can't be reassigned,
sorted in a vector, or moved-from efficiently. Make *locals* and *member
functions* `const`; keep data members non-`const` and protect invariants with
`private` (Abseil [#177](https://abseil.io/tips/177)).

```cpp
struct BadPoint { const int x, y; };   // BAD: BadPoint can't be assigned
struct Point { int x, y; };            // GOOD: a regular value type
```

### Explicit mutation boundaries

Make mutation a visible, scoped event. One pattern: an object exposes only
read operations, and all writes go through a function that hands a mutable
handle to a callable.

```cpp nocompile
// automerge-cpp's API (third-party), shown for the pattern
auto doc = Document{};
doc.transact([](Transaction& tx) {
    tx.put(root, "name", "Alice");
    tx.insert(list, 0, 42);
});   // the changes commit as one unit here
```

Everything that changes state is inside the lambda, and the transaction is all
or nothing.

## 5. Composition over inheritance

- **No deep hierarchies for data.** A struct *contains* parts; a variant *is
  one of* alternatives. Neither needs virtual dispatch.

  ```cpp
  // BAD — polymorphism used to model data
  class ValueBase { public: virtual ~ValueBase() = default; };
  class IntValue : public ValueBase { int v; };
  class StrValue : public ValueBase { std::string v; };

  // GOOD — closed set, no heap, visitable
  using Value = std::variant<int, std::string>;
  ```

- **Concepts for generic interfaces.** When several types share an interface,
  a concept gives compile-time checking with no virtual call.

  ```cpp
  struct ObjId {}; using Prop = std::string; using Value = std::variant<int, std::string>;

  template <typename T>
  concept Readable = requires(const T& doc, ObjId obj, Prop key) {
      { doc.get(obj, key) } -> std::same_as<std::optional<Value>>;
      { doc.length(obj) }   -> std::same_as<std::size_t>;
  };
  ```

- **Small, focused components.** Each piece does one thing and composes with
  the others (storage, sync, and the public API as separate parts). No god
  classes.

## 6. Easy to use, hard to misuse

- **Encode semantics in the return type.**

  | Semantics | Return |
  |---|---|
  | Always succeeds | `T` |
  | Might not exist | `std::optional<T>` |
  | Might fail, with a reason | `std::expected<T, Error>` |
  | Mutates in place | `void` |
  | Lazy sequence | a range / `std::generator<T>` |

- **Avoid adjacent parameters of the same type.** `splice(obj, 5, 0, "hi")` —
  is 5 the position or the count? Use strong types, a parameter struct with
  designated initializers, or split the function.

  ```cpp
  struct Splice { std::size_t pos; std::size_t del = 0; std::string_view text; };
  struct Document { void splice(int obj, Splice s); };

  void f(Document& doc, int text_obj) {
      doc.splice(text_obj, {.pos = 5, .text = "hello"});
  }
  ```

- **Don't take `const std::optional<T>&` parameters.** It forces callers with
  a plain `T` to build an `optional` copy. Take small optionals by value; for
  large objects take a nullable `const T*` (or `std::optional<const T&>` in
  C++26). Use `optional` for things you own — members and return values
  (Abseil [#163](https://abseil.io/tips/163)).

- **Prefer non-member, non-friend functions** when an operation doesn't need
  private state (`to_json(doc)` rather than `doc.to_json()`). The class
  interface stays minimal.

- **Make the common case short.** Initializer lists, typed accessors
  (`get<std::int64_t>(key)`), and overloads for scalar types (in the style of
  nlohmann/json) remove boilerplate without giving up type checking.

## 7. Test the laws

When a type has algebraic properties, test them directly — ideally as
property-based tests over random inputs:

```cpp
// merge(a, b) == merge(b, a)                     commutative
// merge(merge(a, b), c) == merge(a, merge(b, c)) associative
// merge(a, a) == a                               idempotent
// merge(a, Document{}) == a                      identity
```

## Checklist

- [ ] No raw loops in library code (algorithms, ranges, or folds)
- [ ] Mutually exclusive state lives in a `std::variant`
- [ ] No implicit conversions between domain types
- [ ] No `void*`, `std::any`, or owning raw pointers
- [ ] Every variant alternative handled (no unchecked `std::get`)
- [ ] Failure visible in the signature (`expected` / `optional`)
- [ ] Nothing mutable that could be `const`
- [ ] Public types are regular (copy/move/compare)
- [ ] Algebraic laws have tests

## See also

- Ben Deane, *Using Types Effectively* (CppCon 2016)
- Ben Deane, *Easy to Use, Hard to Misuse: Declarative Style in C++* (C++Now 2018)
- Ben Deane, *Composable C++: Principles and Patterns* (CppCon 2021)
- [variant](../examples/utilities/variant.md) ·
  [expected](../examples/utilities/expected.md) ·
  [views](../examples/ranges/views.md) ·
  [execution & ranges algorithms](../examples/algorithms/execution-and-ranges.md)
