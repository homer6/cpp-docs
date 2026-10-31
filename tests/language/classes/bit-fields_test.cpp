// Checks claims made by docs/examples/language/classes/bit-fields.md.
#include <type_traits>

#include <gtest/gtest.h>

#include "support/capture.hpp"

namespace {

struct Flags {  // "Flags with default member initializers (C++20)"
    unsigned ready   : 1 = 0;
    unsigned dirty   : 1 = 1;
    unsigned mode    : 3 {5};
    unsigned         : 0;
    unsigned counter : 4 = 0;
};

struct Packed {  // Flags without the ":0"
    unsigned ready : 1, dirty : 1, mode : 3, counter : 4;
};

struct Small {  // "Wraparound on overflow"
    unsigned u : 4;
    int      s : 2;
};

enum class Color : unsigned { red, green, blue };
struct Pixel {  // "Enumerations in bit-fields"
    Color    c     : 2;
    unsigned alpha : 6;
};

enum class SColor : int { red, green, blue };
struct SPixel { SColor c : 2; };

struct Empty {};
struct WithEmpty { int i; Empty e; };
struct WithNUA   { int i; [[no_unique_address]] Empty e; };

struct Gap {  // the unnamed field isn't a member, so aggregate init skips it
    unsigned a : 4;
    unsigned   : 4;
    unsigned b : 4;
};

struct B { unsigned x : 4; };

template <class T>
concept AddressableA = requires(T& t) { &t.a; };
struct F { unsigned a : 1; };

}  // namespace

// "Flags with default member initializers (C++20)": the defaults print 0150.
TEST(BitFields, DefaultMemberInitializers) {
    auto out = cppdocs::capture_stdout([] {
        Flags f;
        std::cout << f.ready << f.dirty << f.mode << f.counter << '\n';
    });
    EXPECT_EQ(out, "0150\n");
}

// "Mental model": ":0" starts a new allocation unit, so sizeof is 8 (Itanium ABI only).
TEST(BitFields, ZeroWidthStartsNewUnit) {
#if defined(__GNUC__) && !defined(_MSC_VER)
    EXPECT_EQ(sizeof(Flags), 8u);
    EXPECT_EQ(sizeof(Packed), 4u);
#else
    GTEST_SKIP() << "bit-field layout is implementation-defined; page states Itanium-ABI sizes";
#endif
}

// "Declaration": a field keeps the low width bits of an out-of-range value (C++20).
TEST(BitFields, WraparoundOnOverflow) {
    volatile int seventeen = 17, three = 3;  // run-time values: truncated silently
    Small x{};
    x.u = static_cast<unsigned>(seventeen);
    x.s = three;
    EXPECT_EQ(x.u, 1u);
    EXPECT_EQ(x.s, -1);
    auto out = cppdocs::capture_stdout([&] { std::cout << x.u << ' ' << x.s << '\n'; });
    EXPECT_EQ(out, "1 -1\n");
}

// "Enumerations in bit-fields": unsigned underlying type round-trips; a signed one doesn't.
TEST(BitFields, EnumerationFields) {
    static_assert(Pixel{Color::blue, 63}.c == Color::blue);
    SPixel p{};
    volatile int two = 2;
    p.c = static_cast<SColor>(two);
    EXPECT_EQ(static_cast<int>(p.c), -2);
    EXPECT_FALSE(p.c == SColor::blue);
}

// "[[no_unique_address]] is a different tool": the empty member takes no space (Itanium ABI).
TEST(BitFields, NoUniqueAddress) {
#if defined(__GNUC__) && !defined(_MSC_VER)
    EXPECT_EQ(sizeof(WithEmpty), 2 * sizeof(int));
    EXPECT_EQ(sizeof(WithNUA), sizeof(int));
#else
    GTEST_SKIP() << "[[no_unique_address]] may be ignored; page states Itanium-ABI sizes";
#endif
}

// "Declaration": unnamed bit-fields are skipped by aggregate initialization.
TEST(BitFields, UnnamedFieldsAreSkipped) {
    Gap g{1, 2};
    EXPECT_EQ(g.a, 1u);
    EXPECT_EQ(g.b, 2u);
}

// "Gotchas": no address; const& binds a copy; structured bindings name the field itself.
TEST(BitFields, NoAddressReferencesAndBindings) {
    static_assert(!AddressableA<F>);

    Flags f;
    const unsigned& r = f.mode;  // bound to a temporary copy
    f.mode = 2;
    EXPECT_EQ(r, 5u);

    F one{0};
    auto& [a] = one;
    a = 1;
    EXPECT_EQ(one.a, 1u);
}

// "Gotchas": a narrow unsigned bit-field promotes to int; auto deduces the declared type.
TEST(BitFields, PromotionSurprises) {
    B b{3};
    static_assert(std::is_same_v<decltype(+b.x), int>);
    auto y = b.x;
    static_assert(std::is_same_v<decltype(y), unsigned>);
    EXPECT_EQ(y, 3u);
}
