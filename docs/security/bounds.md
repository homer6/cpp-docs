# Bounds and containers

> Keep the extent with the data, check every attacker-influenced index, offset and range before the access, and don't let a mutation invalidate what you're still holding.

## Mental model

An out-of-bounds read leaks memory (Heartbleed was one); an out-of-bounds
write corrupts it. In C++ they happen when an index, offset or length that
came from input reaches an operation whose precondition is "this is in range":
`operator[]`, pointer arithmetic, `memcpy`, an iterator pair, a `span` built
from a pointer and a count. The defence has three parts:

1. **Carry the extent** with the data (`span`, `string_view`, containers), so
   there's something to check against.
2. **Check before the access**, in code that runs in every build.
3. **Respect invalidation**: a reference, pointer or iterator into a container
   is only valid until the container reallocates or erases it.

[Memory & safety](../guidelines/safety.md#2-bounds-safety-probounds) has the
Bounds profile in general; this page is about input deciding the index.

## Carry the extent

**MUST:** Pass sequences as one object that knows its size (`std::span`,
`std::string_view`, a container), never as a pointer plus an unrelated count
(**I.13**, **F.24**, **Bounds.1**, **Bounds.3**, **SL.con.1**). Then the size
can't drift from the data.

**MUST:** Check a pointer and count that come from outside (a C API, a wire
header) *before* wrapping them. `std::span{p, n}` with the wrong `n` is still
wrong; the wrapper only makes later checks possible.

```cpp
#include <cstddef>
#include <optional>
#include <span>

// The payload length comes from the header; check it against what was received.
std::optional<std::span<const std::byte>>
payload(std::span<const std::byte> packet, std::size_t header_size, std::size_t claimed_len) {
    if (header_size > packet.size()) return std::nullopt;
    std::size_t available = packet.size() - header_size;   // can't wrap: checked above
    if (claimed_len > available) return std::nullopt;       // the header lied
    return packet.subspan(header_size, claimed_len);
}
```

## Check before the access

**MUST:** Validate every index, offset, length, dimension and iterator range
that an attacker can influence before it reaches an operation with a
precondition (**SL.con.3**, **ES.55**; CERT
[CTR50-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/containers-ctr/ctr50-cpp),
[CTR53-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/containers-ctr/ctr53-cpp),
[STR53-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/characters-and-strings-str/str53-cpp),
[ARR30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/arrays-arr/arr30-c)).
Check offsets with subtraction, as above, so the check itself can't overflow
([integers](integers.md)).

```cpp bad run
#include <cstddef>
#include <vector>

int lookup(const std::vector<int>& table, std::size_t index_from_message) {
    return table[index_from_message];          // BAD: no check
}

int main() {
    std::vector<int> table(4);
    return lookup(table, 4);                   // ASan: heap-buffer-overflow
}
```

**SHOULD:** Check explicitly at the boundary and handle the failure as an
ordinary outcome; then the unchecked `[]` is fine inside, where the index is
correct by construction. `at()` also checks, but turns an expected bad input
into an exception; prefer an explicit check unless throwing is the policy you
want (Abseil [#224](https://abseil.io/tips/224); see
[safety: hardening in practice](../guidelines/safety.md#hardening-in-practice)).

```cpp
#include <cstddef>
#include <optional>
#include <vector>

std::optional<int> lookup(const std::vector<int>& table, std::size_t index_from_message) {
    if (index_from_message >= table.size()) return std::nullopt;   // checked, every build
    return table[index_from_message];
}
```

**MUST:** Keep required checks in release builds. `assert` disappears under
`NDEBUG`, and a contract assertion can be evaluated with the *ignore*
semantic; neither is input validation
([language versions](language-versions.md#c26-contracts-and-library-hardening)).

**SHOULD:** Turn on standard-library hardening as the safety net for checks
you missed (libstdc++ `_GLIBCXX_ASSERTIONS`, libc++
`_LIBCPP_HARDENING_MODE`, MS STL `_MSVC_STL_HARDENING`). It checks library
preconditions such as `operator[]` on `vector`, `span`, `string` and
`array`; it doesn't check raw pointer arithmetic, `memcpy`, or a C array, so it
doesn't replace the check above ([hardening](hardening.md)).

**MUST:** Bound copies to the destination: when a library function writes
through an iterator or pointer (`std::copy`, `memcpy`, `std::ranges::copy` into
a raw buffer), the destination must have room (CERT
[CTR52-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/containers-ctr/ctr52-cpp));
iterator arithmetic past the end is undefined even before you dereference it
([CTR55-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/containers-ctr/ctr55-cpp)).

## Respect invalidation

**MUST:** Don't hold a pointer, reference, iterator or view into a container
across a mutation that can invalidate it, including a mutation made through
another alias or by a callback you call (CERT
[CTR51-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/containers-ctr/ctr51-cpp)).
`vector::push_back` past capacity reallocates and invalidates everything; each
container page has an "Iterator & reference invalidation" section with the
exact rules.

```cpp bad run
#include <functional>
#include <string>
#include <vector>

struct Registry {
    std::vector<std::string> names;
    std::function<void()> on_change;
};

int main() {
    Registry r;
    r.names.reserve(1);
    r.names.push_back("alice");
    r.on_change = [&r] { r.names.push_back("mallory"); };   // a callback that mutates
    const std::string& first = r.names[0];
    r.on_change();                    // reallocates: `first` now dangles
    return first.size() == 5 ? 0 : 1; // ASan: heap-use-after-free
}
```

Hold an index (re-checked after the call) or a copy instead, or don't call out
while holding the reference.

**MUST:** Distinguish capacity from size: `reserve` allocates memory but
doesn't create elements, so `v.reserve(n); v[0] = x;` is out of bounds.

## See also

- Core Guidelines: [Bounds.1–Bounds.4](../guidelines/core-guidelines/profile.md),
  [SL.con.1–SL.con.4](../guidelines/core-guidelines/stdlib.md),
  [I.13](../guidelines/core-guidelines/interfaces.md),
  [F.24](../guidelines/core-guidelines/functions.md),
  [ES.42, ES.55](../guidelines/core-guidelines/expr.md)
- [span](../examples/containers/views/span.md) ·
  [vector (invalidation)](../examples/containers/sequence/vector.md) ·
  [Memory & safety: iterator and reference invalidation](../guidelines/safety.md#iterator-and-reference-invalidation)
- [libc++ hardening modes](https://libcxx.llvm.org/Hardening.html)
- Tests: [`tests/security/bounds_test.cpp`](../../tests/security/bounds_test.cpp)
