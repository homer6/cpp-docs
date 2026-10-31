// Checks claims made by docs/examples/language/declarations-initialization/namespaces.md.
#include <filesystem>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace {
namespace app {
    int version() { return 3; }
    namespace net { int connect() { return 1; } }
}

namespace lib {
    inline namespace v2 { int api() { return 2; } }
    namespace v1 { int api() { return 1; } }
}

namespace geo {
    struct Pt {
        int x;
        friend int norm1(Pt p) { return p.x < 0 ? -p.x : p.x; }  // hidden friend: ADL only
    };
    int describe(Pt) { return 100; }
}

template <class T> concept NormFoundByAdl = requires(T t) { norm1(t); };

int helper() { return 42; }  // anonymous namespace → internal linkage
}  // namespace

// "Declaring and using": qualified names, nested namespaces, aliases, using-declarations.
TEST(Namespaces, DeclaringAndUsing) {
    EXPECT_EQ(app::version(), 3);
    EXPECT_EQ(app::net::connect(), 1);

    namespace fs = std::filesystem;
    static_assert(std::is_same_v<fs::path, std::filesystem::path>);

    using app::version;
    EXPECT_EQ(version(), 3);
    {
        using namespace app;
        EXPECT_EQ(net::connect(), 1);
    }
    EXPECT_EQ(helper(), 42);
}

// "inline namespaces": lib::api() resolves to lib::v2::api(); v1 stays reachable.
TEST(Namespaces, InlineNamespaces) {
    EXPECT_EQ(lib::api(), 2);
    EXPECT_EQ(lib::v2::api(), 2);
    EXPECT_EQ(lib::v1::api(), 1);
}

// "Name lookup & ADL": unqualified swap and operator<< are found in namespace std.
TEST(Namespaces, ArgumentDependentLookup) {
    std::vector<int> a{3, 1, 2}, b;
    swap(a, b);
    EXPECT_TRUE(a.empty());
    EXPECT_EQ(b, (std::vector<int>{3, 1, 2}));

    std::ostringstream os;
    std::string s = "hi";
    os << s;
    EXPECT_EQ(os.str(), "hi");

    geo::Pt p{-4};
    EXPECT_EQ(describe(p), 100);  // unqualified, found in geo via ADL
}

// Gotchas: a hidden friend is found only by ADL.
TEST(Namespaces, HiddenFriendOnlyViaAdl) {
    static_assert(NormFoundByAdl<geo::Pt>);
    static_assert(!NormFoundByAdl<int>);
    EXPECT_EQ(norm1(geo::Pt{-4}), 4);
}
