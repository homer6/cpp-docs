# Named requirements

> The standard's prose contracts on types — *CopyConstructible*, *Compare*,
> *Hash*, *Allocator*, *LegacyForwardIterator* … — that templates such as
> `std::sort` and `std::unordered_map` expect from their arguments.
> **Header:** none (specification only) · **Since:** C++98; partly re-expressed
> as [concepts](../concepts/README.md) in C++20

## Mental model

Long before concepts, the standard said things like "`T` shall meet the
*Cpp17CopyConstructible* requirements" and defined that name in a table:
which expressions must be valid, what type they return, and what they must
*mean* (a copy is equivalent to the original, `comp` is a strict weak ordering,
…). These are the **named requirements**. Nothing in the language checks them;
they are a contract between you and the library.

```
named requirement  =  syntax (expressions must compile)  +  semantics (what they must do)
                          │                                   │
C++20 concept        checks this at compile time       "modeled" — documented, not checked
```

Spelling: the standard prefixes them with `Cpp17` (*Cpp17CopyConstructible*,
*Cpp17ForwardIterator*). cppreference drops the prefix (*CopyConstructible*) and
writes iterator ones with `Legacy` (*LegacyForwardIterator*) to tell them apart
from the C++20 iterator concepts.

## The main groups

| Group | Named requirements (cppreference spelling) | Concept counterpart (C++20) |
|---|---|---|
| Basic | *DefaultConstructible*, *MoveConstructible*, *CopyConstructible*, *MoveAssignable*, *CopyAssignable*, *Destructible* | `default_initializable`, `move_constructible`, `copy_constructible`, `assignable_from`, `destructible` |
| Type properties | *TriviallyCopyable*, *TrivialType*, *StandardLayoutType*, *ImplicitLifetimeType*, *ScalarType* … | type traits: `is_trivially_copyable_v`, `is_standard_layout_v` … |
| Library-wide | *EqualityComparable*, *LessThanComparable*, *Swappable*, *ValueSwappable*, *NullablePointer*, *Hash*, *Allocator*, *FunctionObject*, *Callable*, *Predicate*, *BinaryPredicate*, *Compare* | `equality_comparable`, `totally_ordered`, `swappable`, `invocable`, `predicate`, `strict_weak_order` (no concept for *Hash* or *Allocator*) |
| Containers | *Container*, *ReversibleContainer*, *AllocatorAwareContainer*, *SequenceContainer*, *ContiguousContainer*, *AssociativeContainer*, *UnorderedAssociativeContainer* | ranges: `range`, `sized_range`, `contiguous_range` … (looser) |
| Iterators | *LegacyIterator*, *LegacyInputIterator*, *LegacyOutputIterator*, *LegacyForwardIterator*, *LegacyBidirectionalIterator*, *LegacyRandomAccessIterator*, *LegacyContiguousIterator* | `input_iterator`, `output_iterator`, `forward_iterator`, … (different rules, see below) |
| Stream I/O | *UnformattedInputFunction*, *FormattedInputFunction*, *UnformattedOutputFunction*, *FormattedOutputFunction* | — |
| Random numbers | *SeedSequence*, *UniformRandomBitGenerator*, *RandomNumberEngine*, *RandomNumberDistribution* | `uniform_random_bit_generator` |
| Concurrency | *BasicLockable*, *Lockable*, *TimedLockable*, *SharedLockable*, *SharedTimedLockable*, *Mutex*, *TimedMutex*, *SharedMutex* | — |
| Other | *Clock*, *TrivialClock*, *CharTraits*, *BitmaskType*, *LiteralType*, *Formatter*, *UnaryTypeTrait*, *BinaryTypeTrait* … | `chrono::is_clock_v` (a trait) |

## When it matters

- **Writing a type the library will use**: a hash functor, a custom allocator, a
  comparator, an iterator, a lock — check the requirement's table, not just
  "does it compile".
- **Reading library errors**: the pre-C++20 algorithms don't check
  requirements up front, so a violation surfaces as an error deep inside the
  implementation (or not at all).
- **Choosing between `std::` and `std::ranges::` algorithms**: the `ranges`
  versions are constrained by concepts and reject bad arguments at the call.

## Examples

### Satisfying *BasicLockable* — any `lock()`/`unlock()` pair works

```cpp
#include <atomic>
#include <mutex>

class SpinLock {                                   // BasicLockable: lock() + unlock()
    std::atomic_flag flag_;
public:
    void lock() noexcept   { while (flag_.test_and_set(std::memory_order_acquire)) {} }
    void unlock() noexcept { flag_.clear(std::memory_order_release); }
};

SpinLock g_lock;
int g_counter = 0;

void bump() {
    std::lock_guard guard{g_lock};                // lock_guard only needs BasicLockable
    ++g_counter;
}
```

### Satisfying *Hash* for an unordered container

```cpp
#include <cstddef>
#include <functional>
#include <print>
#include <unordered_set>

struct Point {
    int x, y;
    bool operator==(const Point&) const = default;   // keys need equality too
};

template<> struct std::hash<Point> {                  // Hash: callable, returns size_t,
    std::size_t operator()(const Point& p) const noexcept {   // equal keys -> equal hashes
        return std::hash<int>{}(p.x) * 31 + std::hash<int>{}(p.y);
    }
};

int main() {
    std::unordered_set<Point> s{{1, 2}, {1, 2}, {3, 4}};
    std::println("{}", s.size());                     // 2
}
```

### Same requirement, two eras of checking

```cpp compile-fail="no match for .operator-|invalid operands to binary expression"
#include <algorithm>
#include <list>

void f(std::list<int>& l) {
    std::sort(l.begin(), l.end());   // needs LegacyRandomAccessIterator: error deep in stl_algo.h
}
```

```cpp compile-fail="constraints not satisfied|random_access"
#include <algorithm>
#include <list>

void f(std::list<int>& l) {
    std::ranges::sort(l);            // concept-checked: "random_access_range ... not satisfied"
}
```

### Concepts and named requirements are not the same test

C++20 iterator concepts allow iterators whose `operator*` returns a value; the
*Legacy* requirements demand a real reference for forward iterators and up. So
`iota_view`'s iterator is a `std::random_access_iterator` but only a
*LegacyInputIterator*:

```cpp
#include <iterator>
#include <ranges>
#include <type_traits>

using It = std::ranges::iterator_t<std::ranges::iota_view<int, int>>;
static_assert(std::random_access_iterator<It>);                       // C++20 concept
static_assert(std::is_same_v<std::iterator_traits<It>::reference, int>);   // not int&
static_assert(std::is_same_v<std::iterator_traits<It>::iterator_category,
                             std::input_iterator_tag>);               // legacy category
```

Passing such an iterator to a pre-C++20 algorithm that dispatches on
`iterator_category` gets the slow input-iterator path.

## Gotchas

- **Semantic requirements are unchecked, and breaking them is undefined
  behaviour.** `std::sort(v.begin(), v.end(), [](int a, int b) { return a <= b; })`
  compiles cleanly and ran without complaint in a normal build; `<=` is not a
  strict weak ordering. With checking turned on, libstdc++ (`-D_GLIBCXX_DEBUG`)
  aborted with `comparison doesn't meet irreflexive requirements`, and libc++
  (`-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG`) aborted with
  `Comparator does not induce a strict weak ordering`.
- **Satisfying a concept is not modeling it.** `std::equality_comparable<T>` is
  true for any `T` with a compiling `==`, even one that isn't an equivalence
  relation. If a constraint's semantic requirements aren't *modeled* where a
  library declaration uses them, the program is ill-formed, no diagnostic
  required ([res.on.requirements]).
- **Requirements apply when the member is used.** `std::vector<T>` only needs
  *CopyInsertable* `T` if you call something that copies; `std::vector<std::unique_ptr<int>>`
  is fine until you copy the vector.
- **"Legacy" does not mean deprecated.** The *Legacy* iterator requirements still
  govern every non-ranges algorithm and `iterator_traits::iterator_category`.

## See also

- [Named requirements — cppreference](https://en.cppreference.com/w/cpp/named_req)
- [Concepts library — cppreference](https://en.cppreference.com/w/cpp/concepts)
- [Iterator library (legacy requirements vs concepts) — cppreference](https://en.cppreference.com/w/cpp/iterator)
- [`[utility.arg.requirements]` — standard draft](https://eel.is/c++draft/utility.arg.requirements)
- [libstdc++ debug mode](https://gcc.gnu.org/onlinedocs/libstdc++/manual/debug_mode.html) · [libc++ hardening modes](https://libcxx.llvm.org/Hardening.html)
- [Standard concepts](../concepts/standard-concepts.md) · [Iterators](../iterators/README.md)
