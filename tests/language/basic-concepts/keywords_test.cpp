// Checks claims made by docs/examples/language/basic-concepts/keywords.md.
#include <type_traits>
#include <version>

#include <gtest/gtest.h>

#define KEYWORDS_STR(x) #x

namespace {
struct Base { virtual ~Base() = default; virtual int f() { return 0; } };
struct Derived final : Base { int f() override { return 1; } };   // special meaning here

constexpr bool exactly_one(bool a, bool b) { return (a or b) and not (a and b); }

int digraph_array<:2:> = <%1, 2%>;    // int digraph_array[2] = {1, 2};

[[maybe_unused]] static int counter = 0;
}  // namespace

// "Contextual identifiers are still ordinary names": final/override/pre/post/module as variables.
TEST(Keywords, ContextualIdentifiersAreOrdinaryNames) {
    static_assert(std::is_final_v<Derived>);
    Derived d;
    Base& b = d;
    EXPECT_EQ(b.f(), 1);
    int final = 1, override = 2;          // ...and ordinary names here
    int pre = 3, post = 4, module = 5;    // compiles with GCC 15/16 and Clang 22
    EXPECT_EQ(final + override + pre + post + module, 15);
}

// "Alternative tokens and digraphs": the words are the operators; digraphs are punctuators.
TEST(Keywords, AlternativeTokensAndDigraphs) {
    static_assert(exactly_one(true, false) && !exactly_one(true, true) && !exactly_one(false, false));
    int x = 0b1100;
    x xor_eq 0b0101;
    EXPECT_EQ(x, 0b1001);
    EXPECT_EQ((x bitand 0b1000), 0b1000);
    EXPECT_EQ((x bitor 0b0110), 0b1111);
    EXPECT_EQ(compl 0, -1);
    EXPECT_TRUE(1 not_eq 2);
    EXPECT_EQ(digraph_array[0], 1);
    EXPECT_EQ(digraph_array<:1:>, 2);
}

// "Mental model": to the preprocessor (phase 4) keywords are still identifiers.
TEST(Keywords, KeywordsAreIdentifiersToThePreprocessor) {
    EXPECT_STREQ(KEYWORDS_STR(class), "class");
#if static   // a non-macro identifier in #if evaluates to 0
    FAIL() << "keyword treated as non-zero in #if";
#endif
}

// "Keywords can't be names": contract_assert is only reserved where contracts are implemented.
TEST(Keywords, ContractAssertReservedOnlyWithContracts) {
#ifdef __cpp_contracts
    GTEST_SKIP() << "contract_assert is a keyword here (the compile-fail fence covers GCC 16)";
#else
    int contract_assert = 0;   // GCC 15 and Clang 22 still accept it as a name
    EXPECT_EQ(contract_assert, 0);
#endif
}

#undef KEYWORDS_STR
