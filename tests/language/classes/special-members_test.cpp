// Checks claims made by docs/examples/language/classes/special-members.md.
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {

struct Widget {};
struct Gadget {  // "Rule of Zero" example from the page
    std::string name;
    std::vector<int> data;
    std::unique_ptr<Widget> part;
};

struct Move {  // "default and delete" example from the page
    Move() = default;
    Move(const Move&) = delete;
    Move(Move&&) = default;
    Move& operator=(Move&&) = default;
};

// Records which constructor ran.
struct Tracker {
    static inline std::string log;
    Tracker() = default;
    Tracker(const Tracker&) { log += "copy "; }
    Tracker(Tracker&&) noexcept { log += "move "; }
    Tracker& operator=(const Tracker&) = default;
    Tracker& operator=(Tracker&&) = default;
};

struct WithDtor {  // user-declared destructor, no move members
    Tracker t;
    ~WithDtor() {}
};

struct MayThrowMove {
    static inline int copies = 0;
    MayThrowMove() = default;
    MayThrowMove(const MayThrowMove&) { ++copies; }
    MayThrowMove(MayThrowMove&&) {}  // not noexcept
};

// RAII: acquires in the constructor, releases in the destructor.
struct Resource {
    static inline int open = 0;
    Resource() { ++open; }
    ~Resource() { --open; }
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;
};

struct Point {  // "Construction details" example from the page
    int x, y;
    Point(int x, int y) : x{x}, y{y} {}
    explicit Point(int v) : Point{v, v} {}
};

struct Implicit {};  // declares nothing: the compiler supplies all six

}  // namespace

// "The six special members": a class that declares none gets all six, trivially.
TEST(SpecialMembers, CompilerSynthesizesAllSix) {
    static_assert(std::is_trivially_default_constructible_v<Implicit>);
    static_assert(std::is_trivially_destructible_v<Implicit>);
    static_assert(std::is_trivially_copy_constructible_v<Implicit>);
    static_assert(std::is_trivially_copy_assignable_v<Implicit>);
    static_assert(std::is_trivially_move_constructible_v<Implicit>);
    static_assert(std::is_trivially_move_assignable_v<Implicit>);
}

// "RAII": the destructor runs at scope exit, including when an exception propagates.
TEST(SpecialMembers, RaiiReleasesOnScopeExitAndThrow) {
    Resource::open = 0;
    {
        Resource r;
        EXPECT_EQ(Resource::open, 1);
    }
    EXPECT_EQ(Resource::open, 0);
    try {
        Resource r;
        throw std::runtime_error("boom");
    } catch (const std::runtime_error&) {
        EXPECT_EQ(Resource::open, 0);
    }
}

// "Construction details": member initializer list, delegating and explicit constructors.
TEST(SpecialMembers, ConstructionDetails) {
    Point p{1, 2};
    Point q{7};
    EXPECT_EQ(p.x, 1);
    EXPECT_EQ(p.y, 2);
    EXPECT_EQ(q.x, 7);
    EXPECT_EQ(q.y, 7);
    static_assert(!std::is_convertible_v<int, Point>);
    static_assert(std::is_constructible_v<Point, int>);
}

// "Rule of Zero": a member that is move-only makes the aggregate move-only, automatically.
TEST(SpecialMembers, RuleOfZeroComposes) {
    static_assert(!std::is_copy_constructible_v<Gadget>);
    static_assert(std::is_nothrow_move_constructible_v<Gadget>);
    static_assert(!std::is_copy_constructible_v<Move>);
    static_assert(std::is_move_constructible_v<Move> && std::is_move_assignable_v<Move>);
}

// "Gotchas": declaring a destructor suppresses implicit moves, so "moving" copies.
TEST(SpecialMembers, UserDestructorSuppressesMove) {
    Tracker::log.clear();
    WithDtor a;
    WithDtor b = std::move(a);
    (void)b;
    EXPECT_EQ(Tracker::log, "copy ");
}

// "Gotchas": vector reallocation copies elements whose move constructor isn't noexcept.
TEST(SpecialMembers, VectorCopiesWhenMoveIsNotNoexcept) {
    MayThrowMove::copies = 0;
    std::vector<MayThrowMove> v(1);
    v.reserve(v.capacity() + 1);  // force a reallocation
    EXPECT_EQ(MayThrowMove::copies, 1);

    Tracker::log.clear();
    std::vector<Tracker> w(1);
    w.reserve(w.capacity() + 1);
    EXPECT_EQ(Tracker::log, "move ");
}

// The out-of-order initializer list below is the point of the test.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
// "Gotchas": members are initialized in declaration order, not initializer-list order.
TEST(SpecialMembers, MemberInitOrderIsDeclarationOrder) {
    static std::string order;
    struct Rec { Rec(char c) { order += c; } };
    struct S {
        Rec a, b;
        S() : b{'b'}, a{'a'} {}  // deliberately out of order
    };
    order.clear();
    S s;
    (void)s;
    EXPECT_EQ(order, "ab");
}
#pragma GCC diagnostic pop
