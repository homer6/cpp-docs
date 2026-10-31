# Testing — frameworks, properties, fuzzing, contracts, coverage

> How to test modern C++ beyond the basics: choosing a framework, testing
> algebraic laws with properties, fuzzing hostile inputs, checking contracts,
> reading coverage, and proving misuse doesn't compile. **Sources:** framework
> and tool docs (checked 2026-10-30) · pairs with
> [project conventions](project-conventions.md#tests)

The day-to-day rules — name tests after behaviour, arrange/act/assert, one
property per test, no helpers that hide logic — are in
[project conventions](project-conventions.md#tests). This page covers the
techniques that catch what example-based unit tests miss.

## Choosing a framework

| | GoogleTest | Catch2 v3 | doctest |
|---|---|---|---|
| Latest release (GitHub tags, checked 2026-10-31) | 1.18.0 (2026-08-10) | 3.16.1 (2026-10-08) | 2.5.3 (2026-07-06) |
| Minimum standard | C++17 (since 1.17.0) | C++14 | C++11 |
| Style | `TEST(Suite, name)` + `EXPECT_*`/`ASSERT_*` macros | `TEST_CASE` + `REQUIRE`/`CHECK`, nested `SECTION`s | Catch-like, `TEST_CASE` + `CHECK` |
| Parameterized / typed tests | `TEST_P`, `TYPED_TEST` | `GENERATE`, `TEMPLATE_TEST_CASE` | `TEST_CASE_TEMPLATE` |
| Death tests | `EXPECT_DEATH` | — | — |
| Mocking | gMock (bundled) | none (pair with a mocking library) | none |
| Best for | Large codebases, mocking, death tests | Expressive tests, BDD-style sections | Very fast compile times; tests next to code |

All three integrate with CTest (`gtest_discover_tests`, `catch_discover_tests`,
`doctest_discover_tests`). Pick one per project. GoogleTest is the safe
default when you need death tests (see [contracts](#testing-contracts)) or
mocks.

```cpp nocompile
#include <gtest/gtest.h>

TEST(ActorId, ordering_is_lexicographic_on_bytes) {
    const auto a = ActorId::from_hex("01");
    const auto b = ActorId::from_hex("02");
    EXPECT_LT(a, b);
}
```

## Property-based tests

An example-based test checks one input you thought of. A **property-based
test** states a rule that must hold for *every* input, generates hundreds of
random inputs, and — when one fails — **shrinks** it to a minimal failing case.

They're the natural way to [test the laws](design-principles.md#7-test-the-laws):
if `merge` is supposed to be commutative, say so and let the library hunt for
a counterexample.

```cpp nocompile
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

RC_GTEST_PROP(Document, merge_is_commutative, (const Document& a, const Document& b)) {
    RC_ASSERT(merge(a, b) == merge(b, a));
}

RC_GTEST_PROP(Document, merge_is_idempotent, (const Document& a)) {
    RC_ASSERT(merge(a, a) == a);
}
```

Custom types need a generator (an `rc::Arbitrary<Document>` specialization).
Good properties to look for:

- **Algebraic laws:** commutative, associative, idempotent, identity element.
- **Round trips:** `decode(encode(x)) == x`, `parse(print(x)) == x`.
- **Invariants:** a container stays sorted after every insert; sizes add up.
- **Oracles:** a fast implementation agrees with a slow, obviously-correct one.

> **Library status:** [RapidCheck](https://github.com/emil-e/rapidcheck) is
> still updated (last push Oct 2026) but publishes **no tagged releases** — pin
> a commit. Google's FuzzTest combines property tests with coverage-guided
> fuzzing; see the open questions in the research prompt for this page.

## Fuzzing

Fuzzing feeds a function millions of generated inputs, guided by code
coverage, looking for crashes, sanitizer reports, and hangs. Fuzz every
**hostile-input boundary**: parsers, decoders, deserializers, network message
handlers.

A libFuzzer harness is one function:

```cpp
#include <cstddef>
#include <cstdint>
#include <span>

struct Document {   // the type under test
    static std::optional<Document> load(std::span<const std::byte>);
    std::vector<std::byte> save() const;
    bool operator==(const Document&) const;
};

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto bytes = std::as_bytes(std::span{data, size});
    auto doc = Document::load(bytes);   // must never crash, leak, or hit UB
    if (doc) {
        auto again = Document::load(doc->save());
        if (!again || *again != *doc) __builtin_trap();   // round-trip property
    }
    return 0;
}
```

```bash
clang++ -std=c++23 -g -O1 -fsanitize=fuzzer,address,undefined harness.cpp lib.cpp -o fuzz_load
./fuzz_load corpus/          # runs until it finds a failure (or you stop it)
./fuzz_load crash-<hash>     # reproduce a single failing input
```

**Rules of thumb:**

- **Always fuzz with sanitizers** (ASan + UBSan at minimum). A fuzzer without
  ASan only finds crashes that happen to segfault.
- **Seed a corpus** with real inputs, and keep it in version control (or
  storage) so each run starts where the last one stopped.
- **Every crash becomes a regression test:** commit the input and a unit test
  that loads it.
- **Run continuously:** OSS-Fuzz for open source, ClusterFuzzLite or a nightly
  CI job otherwise.
- **The harness must be deterministic** and must not leak or keep global state
  between calls.

## Testing contracts

C++26 [contracts](../examples/language/basic-concepts/contracts.md) are part of a
function's interface, so test them:

- **Run the test suite with contracts `enforce`d.** A test that violates a
  precondition by accident then fails loudly instead of passing by luck.
- **Test that a precondition fires** with a death test: under `enforce`, a
  violation calls the handler and then terminates the program.

```cpp needs=contracts
int pop(std::vector<int>& v) pre(!v.empty());
```

```cpp nocompile
#include <gtest/gtest.h>   // GoogleTest: not compile-checked here

TEST(PopDeathTest, empty_vector_violates_precondition) {
    std::vector<int> v;
    EXPECT_DEATH(pop(v), "");
}
```

- **Don't assert on the handler's message text** — its format is
  implementation-defined.
- **Before contracts are available,** the same pattern works with `assert` in a
  debug build.

### A throwing handler (where you can replace it)

The handler is a global function you define yourself; the standard library
declares none, so you choose its exception specification (P2900R14
[basic.contract.handler]):

```cpp needs=contracts
#include <contracts>
#include <source_location>

struct contract_failed { std::source_location where; };   // your own type

void handle_contract_violation(const std::contracts::contract_violation& v) {
    throw contract_failed{v.location()};
}
```

```cpp nocompile
TEST(Pop, empty_vector_violates_precondition) {   // GoogleTest, as above
    std::vector<int> v;
    EXPECT_THROW(pop(v), contract_failed);   // no death test needed
}
```

What P2900R14 (2025-02-13) says about this:

- **It's allowed.** A handler may exit by throwing, and that skips the
  termination `enforce` would otherwise do (§3.6.6). It works under
  `observe` too.
- **For `pre`/`post`, the exception behaves as if the function body threw
  it.** If the function is `noexcept`, `std::terminate` is called, so this
  only tests functions that can throw. A function-try-block doesn't catch it.
- **For `contract_assert`, the search for a catch handler continues from
  that statement.**
- **Whether you can replace the handler at all is implementation-defined.**
  Where you can't, defining it is ill-formed, no diagnostic required. Check
  your toolchain before relying on this, and keep death tests as the
  portable option.
- **Label each contract test with the semantic it assumes** (`enforce`,
  `observe`): the same test passes or fails differently under each.

Don't use GCC 13's `-fcontracts` to try this out. Its manual describes it as
the contracts design briefly in the C++20 working paper (N4820) plus P1290,
P1332 and P1429, not the C++26 (P2900) design.

## Coverage

Coverage shows which code your tests *executed* — not whether they checked
the results. Use it to find untested code, not as a score.

```bash
# Clang source-based coverage (precise, handles templates and macros well)
clang++ -fprofile-instr-generate -fcoverage-mapping ... -o tests
LLVM_PROFILE_FILE=tests.profraw ./tests
llvm-profdata merge -sparse tests.profraw -o tests.profdata
llvm-cov report ./tests -instr-profile=tests.profdata
llvm-cov show   ./tests -instr-profile=tests.profdata -format=html -output-dir=cov/

# GCC
g++ --coverage ... -o tests && ./tests && gcovr --html-details cov.html
```

- **Look at uncovered branches, not the percentage.** An uncovered error path
  is a question: is it tested elsewhere, unreachable, or a gap?
- **100% line coverage proves little** — a test with no assertions gets it too.
- **Templates are covered per instantiation;** code that's never instantiated
  doesn't appear at all.
- **Merge coverage from all test binaries** (unit, integration, fuzz corpus
  replays) before judging gaps.

## Compile-time tests

The [design principles](design-principles.md) promise that misuse fails to
compile. Test that promise. The checks below are written against
automerge-cpp's types (`Document`, `OpId`, `ActorId`, …), so they aren't
compile-checked here.

**`static_assert` for compile-time facts:**

```cpp nocompile
static_assert(std::is_nothrow_move_constructible_v<Document>);
static_assert(std::regular<OpId>);
static_assert(sizeof(ActorId) == 16);
```

**Requires-expressions to check that misuse is rejected:**

```cpp nocompile
// ActorId and ObjId must not convert into each other
static_assert(!std::convertible_to<ActorId, ObjId>);

// splice() must not accept raw integers for its position argument
static_assert(!requires(Document d, ObjId o) { d.splice(o, 5, 0, "x"); });

// a const Document can't be mutated outside a transaction
static_assert(!requires(const Document& d) { d.put(root, "k", 1); });
```

**Pair every negative check with a positive one.** `!requires { ... }` is also
true when the expression is wrong for an unrelated reason — a typo, a renamed
function, a missing include — so the test can pass while checking nothing.

```cpp nocompile
static_assert( requires(Transaction& tx) { tx.put(root, "k", 1); });   // the right way compiles
static_assert(!requires(const Document& d) { d.put(root, "k", 1); });  // the wrong way doesn't
```

A requires-expression only detects errors in the *immediate context* (overload
resolution, constraints, deleted functions). Errors deep inside a function body
are hard errors, not `false`. For those, use a **negative compilation test**:
a small source file the build expects to fail (e.g. a CTest test that runs the
compiler with `WILL_FAIL`).

## Concurrency

- Run the test suite under **ThreadSanitizer** in CI (see
  [safety](safety.md#build-configurations)), as its own job: TSan can't be
  combined with ASan (GCC rejects the combination).
- **Repeat and shuffle** flaky-prone tests: `--gtest_repeat=1000
  --gtest_shuffle`.
- Prefer tests of **deterministic components** (a scheduler you control, an
  injected clock) over tests that sleep and hope.

## Checklist

- [ ] Algebraic laws and round trips have property tests
- [ ] Every parser/decoder of untrusted input has a fuzz harness, run with
      ASan + UBSan
- [ ] Every fuzzer or sanitizer finding has a regression test
- [ ] Tests run with contracts (or asserts) enforced; key preconditions have
      death tests
- [ ] Coverage reviewed for uncovered error paths, not chased as a number
- [ ] Type-level promises (no implicit conversions, `const` safety) have
      `static_assert`s

## See also

- [GoogleTest](https://google.github.io/googletest/) ·
  [Catch2](https://github.com/catchorg/Catch2) ·
  [doctest](https://github.com/doctest/doctest) ·
  [RapidCheck](https://github.com/emil-e/rapidcheck) ·
  [libFuzzer](https://llvm.org/docs/LibFuzzer.html) ·
  [llvm-cov](https://llvm.org/docs/CommandGuide/llvm-cov.html) ·
  [gcovr](https://gcovr.com/)
- [Project conventions — tests](project-conventions.md#tests) ·
  [Design principles — test the laws](design-principles.md#7-test-the-laws) ·
  [Safety — build configurations](safety.md#build-configurations)
