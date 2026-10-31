// Checks claims made by docs/examples/language/classes/explicit-object-parameter.md.
#include <string>
#include <type_traits>
#include <utility>
#include <version>

#include <gtest/gtest.h>

#include "support/capture.hpp"
#include "support/feature.hpp"

#ifdef __cpp_explicit_this_parameter
namespace {

class Name {  // "One accessor for every value category"
    std::string s_;
public:
    explicit Name(std::string s) : s_{std::move(s)} {}
    template <class Self>
    auto&& str(this Self&& self) { return std::forward<Self>(self).s_; }
};

struct S {  // "Declaration"
    int last = 0;
    void g(this S& self, int x) { self.last = x; }
    template <class Self>
    Self&& who(this Self&& self) { return std::forward<Self>(self); }
};
struct DerivedS : S {};

struct Greeter {  // "Replacing CRTP"
    void greet(this const auto& self) { std::cout << "hello from " << self.name() << '\n'; }
};
struct Dog : Greeter { const char* name() const { return "Dog"; } };
struct Cat : Greeter { const char* name() const { return "Cat"; } };

struct Tagger {  // records Self, to show what it is through a base reference
    template <class Self>
    auto self_type(this const Self&) { return std::type_identity<Self>{}; }
};
struct Tagged : Tagger {};

struct Point {  // "By-value self"
    int x = 0, y = 0;
    Point moved_by(this Point self, int dx, int dy) {
        self.x += dx;
        self.y += dy;
        return self;
    }
};

struct Base {  // "Gotchas": this auto self copies the derived object
    auto copy_type(this auto self) { return self; }
};
struct Derived : Base { int extra = 7; };

}  // namespace
#endif

// "One accessor for every value category": the member keeps the object's category.
TEST(ExplicitObjectParameter, OneAccessorForEveryValueCategory) {
    CPPDOCS_REQUIRE_VERSION(__cpp_explicit_this_parameter, 202110L);
#ifdef __cpp_explicit_this_parameter
    Name n{"Ada"};
    const Name cn{"Grace"};
    static_assert(std::is_same_v<decltype(n.str()), std::string&>);
    static_assert(std::is_same_v<decltype(cn.str()), const std::string&>);
    static_assert(std::is_same_v<decltype(std::move(n).str()), std::string&&>);
    static_assert(std::is_same_v<decltype(std::move(cn).str()), const std::string&&>);
    std::string taken = std::move(n).str();
    EXPECT_EQ(taken, "Ada");
    EXPECT_EQ(cn.str(), "Grace");
#endif
}

// "Declaration": &S::g is a plain function pointer; Self deduces the derived type.
TEST(ExplicitObjectParameter, FunctionPointerAndDeducedSelf) {
    CPPDOCS_REQUIRE(__cpp_explicit_this_parameter);
#ifdef __cpp_explicit_this_parameter
    static_assert(std::is_same_v<decltype(&S::g), void (*)(S&, int)>);
    S s;
    auto pg = &S::g;
    pg(s, 1);
    EXPECT_EQ(s.last, 1);

    DerivedS d;
    static_assert(std::is_same_v<decltype(d.who()), DerivedS&>);
    static_assert(std::is_same_v<decltype(std::as_const(d).who()), const DerivedS&>);
#endif
}

// "Recursive lambdas": the explicit object parameter is the closure, so fib(10) is 55.
TEST(ExplicitObjectParameter, RecursiveLambda) {
    CPPDOCS_REQUIRE(__cpp_explicit_this_parameter);
#ifdef __cpp_explicit_this_parameter
    auto fib = [](this const auto& self, int n) -> int {
        return n < 2 ? n : self(n - 1) + self(n - 2);
    };
    EXPECT_EQ(fib(10), 55);
#endif
}

// "Replacing CRTP": Self is the derived type, but only statically.
TEST(ExplicitObjectParameter, ReplacingCrtp) {
    CPPDOCS_REQUIRE(__cpp_explicit_this_parameter);
#ifdef __cpp_explicit_this_parameter
    auto out = cppdocs::capture_stdout([] {
        Dog{}.greet();
        Cat{}.greet();
    });
    EXPECT_EQ(out, "hello from Dog\nhello from Cat\n");
    Tagged t;
    const Tagger& base = t;  // static dispatch: through a base reference, Self is the base
    static_assert(std::is_same_v<decltype(t.self_type())::type, Tagged>);
    static_assert(std::is_same_v<decltype(base.self_type())::type, Tagger>);
#endif
}

// "By-value self" and "Gotchas": self by value is a copy, and copies a derived object whole.
TEST(ExplicitObjectParameter, ByValueSelf) {
    CPPDOCS_REQUIRE(__cpp_explicit_this_parameter);
#ifdef __cpp_explicit_this_parameter
    auto out = cppdocs::capture_stdout([] {
        Point p{1, 2};
        Point q = p.moved_by(10, 0);
        std::cout << p.x << ' ' << q.x << '\n';
    });
    EXPECT_EQ(out, "1 11\n");

    Derived d;
    auto c = d.copy_type();
    static_assert(std::is_same_v<decltype(c), Derived>);
    EXPECT_EQ(c.extra, 7);
#endif
}
