// Checks claims made by docs/examples/language/basic-concepts/contracts.md.
#include <cstdio>
#include <stdexcept>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

#ifdef __cpp_contracts
#include <contracts>

// The page: "whether you can replace it is implementation-defined". GCC 16 lets
// the program define it; this one reports what it received and returns, so the
// enforce semantic then terminates.
void handle_contract_violation(const std::contracts::contract_violation& v) {
    using std::contracts::assertion_kind;
    const char* kind = v.kind() == assertion_kind::pre    ? "pre"
                       : v.kind() == assertion_kind::post ? "post"
                                                          : "assert";
    std::fprintf(stderr, "violation %s [%s] enforce=%d\n", kind, v.comment(),
                 v.semantic() == std::contracts::evaluation_semantic::enforce);
}

namespace {
int factorial(int n)
    pre (n >= 0)
    post(r: r >= 1)
{
    contract_assert(n < 13);
    return n <= 1 ? 1 : n * factorial(n - 1);
}

bool throws_instead(int) { throw std::runtime_error("predicate threw"); }
void guarded(int x) pre (throws_instead(x)) {}

int broken_result(int n) post(r: r >= 1) { return n; }
}  // namespace
#endif

// "Mental model": pre/post/contract_assert in one function; all hold for valid input.
TEST(Contracts, AssertionsHoldForValidInput) {
    CPPDOCS_REQUIRE(__cpp_contracts);
#ifdef __cpp_contracts
    static_assert(__cpp_contracts >= 202502L);  // "Gotchas": feature-test value
    EXPECT_EQ(factorial(0), 1);
    EXPECT_EQ(factorial(5), 120);
    EXPECT_EQ(factorial(12), 479001600);
#endif
}

// "Contract violations": the handler gets the kind and predicate text; enforce (GCC 16's
// default under plain -std=c++26) then terminates.
TEST(Contracts, ViolationCallsHandlerThenTerminates) {
    CPPDOCS_REQUIRE(__cpp_contracts);
#ifdef __cpp_contracts
    EXPECT_DEATH(factorial(-1), R"(violation pre \[n >= 0\] enforce=1)");
    EXPECT_DEATH(factorial(13), R"(violation assert \[n < 13\] enforce=1)");
    // "post(r: ...) binds the return value": r is what the function returned.
    EXPECT_DEATH(broken_result(0), R"(violation post \[r >= 1\] enforce=1)");
    // "...finds the predicate false (or it throws)": a throwing predicate is a violation too.
    EXPECT_DEATH(guarded(1), R"(violation pre \[throws_instead\(x\)\] enforce=1)");
#endif
}
