# Object state and representation

> Initialize every object before anything reads it, send data across a trust boundary field by field rather than as raw object bytes, and confine casts that reinterpret memory or skip type checks.

## Mental model

Three ways an object's *representation* becomes an attack surface:

1. **Uninitialized state.** A variable or member that's read before it's set
   holds whatever was there before (a previous request's data, a pointer, a
   key), which can leak or steer a decision.
2. **Raw object bytes across a boundary.** A struct's bytes include padding
   and representation details that aren't part of its value. `memcpy`-ing it
   to a socket or file can leak memory, and reading it back trusts the peer to
   produce valid values.
3. **Casts that skip checks.** A `static_cast` to the wrong derived type or a
   `reinterpret_cast` over the wrong bytes makes the program treat one object
   as another; when the type is chosen by input, so is the confusion.

## Initialize before use

**MUST:** Initialize every object at its declaration, and make constructors
produce fully valid objects, so no code path can read an indeterminate value
or a half-built security state (**ES.20**, **Type.5**, **Type.6**, **C.41**;
CERT [EXP53-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/expressions-exp/exp53-cpp)).

```cpp bad
struct Session {
    bool authenticated;          // BAD: indeterminate until someone sets it
    int user_id;
};

bool may_read(const Session& s) { return s.authenticated; }

bool handle() {
    Session s;                   // members uninitialized
    return may_read(s);          // reads garbage: may be "true"
}
```

```cpp
struct Session {
    bool authenticated = false;  // deny until proven otherwise
    int user_id = -1;
};
```

**MUST:** Treat compiler-inserted initialization as defence in depth, not the
fix. `-ftrivial-auto-var-init=zero` (GCC 12 and later, Clang; both of ours
accept it) fills uninitialized automatic variables, and C++26 makes reading an
uninitialized automatic variable *erroneous behaviour* instead of undefined
(see [erroneous behaviour](../examples/language/basic-concepts/erroneous-behaviour.md)).
Neither makes `false` or `0` the *right* value for your state, and neither
covers heap objects or members of objects you `memcpy`.

## Serialize field by field across a trust boundary

**MUST:** Never send or store an object by copying its bytes when the data
crosses a trust boundary. Padding bytes aren't part of the value, so they
carry whatever was in memory (CERT
[DCL55-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/declarations-and-initialization-dcl/dcl55-cpp),
[DCL39-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/declarations-and-initialization-dcl/dcl39-c),
[EXP62-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/expressions-exp/exp62-cpp)):

```cpp bad
#include <cstdint>
#include <cstring>

struct Header {
    std::uint8_t kind;           // 1 byte, then (typically) 3 bytes of padding
    std::uint32_t length;
};

void send_header(unsigned char* out, const Header& h) {
    std::memcpy(out, &h, sizeof h);   // BAD: also sends the padding bytes
}
```

`std::has_unique_object_representations_v<T>` (C++17) is true only when every
bit of `T` is part of its value, which is a quick way to see that a type has
padding. Even when it's true, define the **wire format** yourself (sizes, byte
order, encodings, valid values, version) and write each field:

```cpp
#include <array>
#include <cstdint>
#include <type_traits>

struct Header {
    std::uint8_t kind;
    std::uint32_t length;
};
static_assert(!std::has_unique_object_representations_v<Header>,
              "Header has padding: never memcpy it to the wire");

// Wire format: kind (1 byte), length (4 bytes, little-endian).
std::array<std::uint8_t, 5> encode(const Header& h) {
    return {h.kind,
            static_cast<std::uint8_t>(h.length),
            static_cast<std::uint8_t>(h.length >> 8),
            static_cast<std::uint8_t>(h.length >> 16),
            static_cast<std::uint8_t>(h.length >> 24)};
}
```

**MUST:** Decode into validated values the same way, field by field, with
range checks ([parsing and validation](parsing-and-validation.md),
[integers](integers.md)). Copying received bytes over an object trusts the
peer to produce a valid `bool`, enum, pointer or invariant.

## Confine casts that skip type checks

**MUST:** Don't use `static_cast` to downcast unless the type is guaranteed
by construction; use `dynamic_cast` (or a `std::variant`, or a type tag you've
validated) when the dynamic type depends on input (**Type.2**, **C.146**,
**C.147**, **C.148**). A wrong `static_cast` makes the program use one
object's memory as another's:

```cpp bad run
#include <cstddef>
#include <string>

struct Message { virtual ~Message() = default; };
struct Login : Message { std::string user = "alice"; };
struct Ping  : Message {};       // smaller than Login: no string where Login has one

std::size_t user_length(Message& m) {
    return static_cast<Login&>(m).user.size();   // BAD: m may be a Ping
}

int main() {
    Ping p;                        // the type the peer chose
    return user_length(p) == 0;    // ASan: stack-buffer-overflow
}
```

```cpp
#include <cstddef>
#include <string>

struct Message { virtual ~Message() = default; };
struct Login : Message { std::string user = "alice"; };

std::size_t user_length(const Message& m) {
    if (auto* login = dynamic_cast<const Login*>(&m))   // checked
        return login->user.size();
    return 0;
}
```

**MUST:** Keep `reinterpret_cast`, type punning through pointers and unions,
`const_cast` and C-style casts out of ordinary code (**Type.1**, **Type.3**,
**Type.4**, **Type.7**, **ES.48**, **ES.49**; CERT
[EXP39-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/expressions-exp/exp39-c)).
To reinterpret a value's bits, use `std::bit_cast` (C++20), which copies and
checks sizes at compile time; for byte order, `std::endian` and
`std::byteswap` (C++23). See [bit manipulation](../examples/numerics/bit.md)
and the [object model](../examples/language/basic-concepts/object-model.md).

## See also

- Core Guidelines: [ES.20, ES.48, ES.49](../guidelines/core-guidelines/expr.md),
  [Type.1–Type.8](../guidelines/core-guidelines/profile.md),
  [C.41, C.146–C.148](../guidelines/core-guidelines/class.md)
- [Memory & safety: type safety](../guidelines/safety.md#1-type-safety-protype) ·
  [C++26 erroneous behaviour](../guidelines/safety.md#c26-erroneous-behaviour)
- cppreference: [`has_unique_object_representations`](https://en.cppreference.com/w/cpp/types/has_unique_object_representations),
  [`bit_cast`](https://en.cppreference.com/w/cpp/numeric/bit_cast)
- Tests: [`tests/security/object-representation_test.cpp`](../../tests/security/object-representation_test.cpp)
