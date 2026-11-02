// Checks claims made by docs/guidelines/core-guidelines/examples/errors.md.
#include <algorithm>
#include <charconv>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>
#include <version>
#if __has_include(<expected>)
#include <expected>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
// Counts destructor runs, to see what unwinding cleans up.
struct Tracker {
    int* destroyed;
    explicit Tracker(int* d) : destroyed{d} {}
    Tracker(const Tracker&) = delete;
    Tracker& operator=(const Tracker&) = delete;
    ~Tracker() { ++*destroyed; }
};

int port_from(const std::string& text) {
    if (text.empty()) throw std::invalid_argument{"port_from: empty text"};
    return std::stoi(text);
}

std::optional<std::size_t> index_of(const std::vector<int>& v, int x) {
    auto it = std::ranges::find(v, x);
    if (it == v.end()) return std::nullopt;
    return static_cast<std::size_t>(it - v.begin());
}

class Percent {
    int value_;
public:
    explicit Percent(int v) : value_{v} {
        if (v < 0 || v > 100) throw std::out_of_range{"Percent: not in 0..100"};
    }
    int value() const { return value_; }
};

void fail_after_acquiring(int* destroyed, bool fail) {
    Tracker owner{destroyed};
    if (fail) throw std::runtime_error{"failed"};
}

// Counts how a vector relocates its elements: by move or by copy.
template <bool NothrowMove>
struct Relocated {
    static inline int copies = 0;
    static inline int moves = 0;
    Relocated() = default;
    Relocated(const Relocated&) { ++copies; }
    Relocated(Relocated&&) noexcept(NothrowMove) { ++moves; }
    Relocated& operator=(const Relocated&) = default;
    Relocated& operator=(Relocated&&) = default;
    ~Relocated() = default;
};

struct RecordNotFound : std::runtime_error {
    int id;
    explicit RecordNotFound(int i)
        : std::runtime_error{"record not found: " + std::to_string(i)}, id{i} {}
};

void load(int id) {
    if (id <= 0) throw RecordNotFound{id};
}

struct Connection {
    bool ok = true;
    bool flush() { return ok; }
    ~Connection() { flush(); }
};

template <class F>
class final_action {
    F f_;
public:
    explicit final_action(F f) : f_{std::move(f)} {}
    final_action(const final_action&) = delete;
    final_action& operator=(const final_action&) = delete;
    ~final_action() { f_(); }
};

struct Handle {
    bool acquired;
    int* destroyed;
    Handle(bool ok, int* d) noexcept : acquired{ok}, destroyed{d} {}
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    ~Handle() { ++*destroyed; }
    [[nodiscard]] bool valid() const noexcept { return acquired; }
};

[[nodiscard]] int use_handle(bool ok, int* destroyed) noexcept {
    Handle h{ok, destroyed};
    if (!h.valid()) return -1;
    return 0;
}

#ifdef __cpp_lib_expected
std::expected<int, std::errc> parse_port(std::string_view s) {
    int v = 0;
    auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{}) return std::unexpected{ec};
    if (end != s.data() + s.size() || v < 1 || v > 65535) return std::unexpected{std::errc::invalid_argument};
    return v;
}
#endif

std::optional<long> to_long(std::string_view s) {
    long v = 0;
    auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || end != s.data() + s.size()) return std::nullopt;
    return v;
}

int twice(int v) noexcept { return 2 * v; }
int checked(int v) {
    if (v < 0) throw std::out_of_range{"negative"};
    return v;
}

std::string which_handler(int kind) {
    try {
        if (kind == 0) throw std::out_of_range{"range"};
        if (kind == 1) throw std::invalid_argument{"arg"};
        throw 42;
    } catch (const std::out_of_range&) {
        return "out_of_range";
    } catch (const std::exception&) {
        return "exception";
    } catch (...) {
        return "other";
    }
}
}  // namespace

// E.2: a function that can't do its job throws, and the caller can't miss it.
TEST(CoreGuidelinesE, E2_FailureIsThrown) {
    EXPECT_THROW(port_from(""), std::invalid_argument);
    EXPECT_EQ(port_from("8080"), 8080);
}

// E.3: "not found" is an ordinary result, returned rather than thrown.
TEST(CoreGuidelinesE, E3_NotFoundIsAValueNotAnException) {
    const std::vector<int> v{4, 8, 15};
    EXPECT_EQ(index_of(v, 15), std::optional<std::size_t>{2});
    EXPECT_NO_THROW(EXPECT_EQ(index_of(v, 16), std::nullopt));
}

// E.5: a constructor that can't establish the invariant throws, so no invalid object exists.
TEST(CoreGuidelinesE, E5_ConstructorThrowsOnBadInput) {
    EXPECT_THROW(Percent{101}, std::out_of_range);
    EXPECT_THROW(Percent{-1}, std::out_of_range);
    EXPECT_EQ(Percent{42}.value(), 42);
}

// E.6: a destructor runs when the scope is left by an exception, so the resource is released.
TEST(CoreGuidelinesE, E6_RaiiCleansUpDuringUnwinding) {
    int destroyed = 0;
    EXPECT_THROW(fail_after_acquiring(&destroyed, true), std::runtime_error);
    EXPECT_EQ(destroyed, 1);
}

// E.12: vector moves elements on reallocation only when the move constructor is noexcept.
TEST(CoreGuidelinesE, E12_NoexceptMoveIsUsedByVector) {
    static_assert(std::is_nothrow_move_constructible_v<Relocated<true>>);
    static_assert(!std::is_nothrow_move_constructible_v<Relocated<false>>);

    std::vector<Relocated<true>> fast(1);
    std::vector<Relocated<false>> slow(1);
    Relocated<true>::copies = Relocated<true>::moves = 0;
    Relocated<false>::copies = Relocated<false>::moves = 0;
    fast.reserve(fast.capacity() * 2 + 1);   // reallocates the one element
    slow.reserve(slow.capacity() * 2 + 1);
    EXPECT_EQ(Relocated<true>::moves, 1);
    EXPECT_EQ(Relocated<true>::copies, 0);
    EXPECT_EQ(Relocated<false>::copies, 1);   // a throwing move is avoided: the element is copied
    EXPECT_EQ(Relocated<false>::moves, 0);
}

// E.13: an owning handle frees the object when a throw leaves the function.
TEST(CoreGuidelinesE, E13_OwnerReleasesOnThrow) {
    int destroyed = 0;
    auto build = [&](int n) {
        auto owner = std::make_unique<Tracker>(&destroyed);
        if (n < 0) throw std::invalid_argument{"negative"};
    };
    EXPECT_THROW(build(-1), std::invalid_argument);
    EXPECT_EQ(destroyed, 1);
}

// E.14: a purpose-made exception type carries its details and can be caught as std::exception too.
TEST(CoreGuidelinesE, E14_UserDefinedExceptionCarriesInformation) {
    try {
        load(-7);
    } catch (const RecordNotFound& e) {
        EXPECT_EQ(e.id, -7);
        EXPECT_STREQ(e.what(), "record not found: -7");
    }
    EXPECT_THROW(load(0), std::exception);
}

// E.15: catching by reference keeps the dynamic type, and `throw;` rethrows the original object.
TEST(CoreGuidelinesE, E15_CatchByReferenceKeepsDynamicType) {
    try {
        throw std::out_of_range{"index"};
    } catch (const std::logic_error& e) {
        EXPECT_EQ(typeid(e), typeid(std::out_of_range));
    }
    auto rethrow = [](int i) {
        try {
            if (i < 0) throw std::out_of_range{"index"};
        } catch (const std::logic_error&) {
            throw;
        }
    };
    EXPECT_THROW(rethrow(-1), std::out_of_range);
}

// E.16: destructors are noexcept by default, as are std::swap on such types and exception copies.
TEST(CoreGuidelinesE, E16_DestructorsAndSwapDoNotThrow) {
    static_assert(std::is_nothrow_destructible_v<Connection>);
    static_assert(std::is_nothrow_destructible_v<Tracker>);
    static_assert(std::is_nothrow_swappable_v<std::string>);
    static_assert(std::is_nothrow_copy_constructible_v<std::runtime_error>);
    Connection c;
    c.ok = false;
    EXPECT_FALSE(c.flush());
}

// E.17: an exception passes through functions with no try/catch, and RAII cleans up on the way.
TEST(CoreGuidelinesE, E17_ExceptionPropagatesWithoutLocalCatch) {
    int destroyed = 0;
    auto inner = [&](bool fail) { Tracker t{&destroyed}; if (fail) throw std::runtime_error{"deep"}; };
    auto middle = [&](bool fail) { Tracker t{&destroyed}; inner(fail); };
    EXPECT_THROW(middle(true), std::runtime_error);
    EXPECT_EQ(destroyed, 2);
}

// E.18: an owning object replaces a try/catch written only to free a resource.
TEST(CoreGuidelinesE, E18_OwnerReplacesTryCatch) {
    int destroyed = 0;
    auto run_job = [&](bool fail) {
        auto w = std::make_unique<Tracker>(&destroyed);
        if (fail) throw 1;   // not a std::exception: still cleaned up
    };
    EXPECT_ANY_THROW(run_job(true));
    EXPECT_EQ(destroyed, 1);
}

// E.19: a final_action runs its cleanup on a normal return and on a throw.
TEST(CoreGuidelinesE, E19_FinalActionRunsOnEveryPath) {
    bool busy = false;
    auto work = [&](bool fail) {
        busy = true;
        final_action reset{[&] { busy = false; }};
        if (fail) throw std::runtime_error{"fail"};
    };
    work(false);
    EXPECT_FALSE(busy);
    EXPECT_THROW(work(true), std::runtime_error);
    EXPECT_FALSE(busy);
}

// E.25: without exceptions, the valid() check reports the failure and the destructor still runs.
TEST(CoreGuidelinesE, E25_SimulatedRaiiStillReleases) {
    int destroyed = 0;
    EXPECT_EQ(use_handle(false, &destroyed), -1);
    EXPECT_EQ(use_handle(true, &destroyed), 0);
    EXPECT_EQ(destroyed, 2);
}

// E.27: std::expected returns the value or the error in one object.
TEST(CoreGuidelinesE, E27_ExpectedCarriesValueOrError) {
    CPPDOCS_REQUIRE(__cpp_lib_expected);
#ifdef __cpp_lib_expected
    auto ok = parse_port("8080");
    ASSERT_TRUE(ok.has_value());
    EXPECT_EQ(*ok, 8080);
    auto bad = parse_port("http");
    ASSERT_FALSE(bad.has_value());
    EXPECT_EQ(bad.error(), std::errc::invalid_argument);
    EXPECT_EQ(parse_port("70000").error(), std::errc::invalid_argument);
#endif
}

// E.28: std::from_chars returns its error instead of setting a global like errno.
TEST(CoreGuidelinesE, E28_ErrorIsReturnedNotGlobal) {
    EXPECT_EQ(to_long("1234"), std::optional<long>{1234});
    EXPECT_EQ(to_long("12ab"), std::nullopt);
    long v = 0;
    const std::string_view huge = "99999999999999999999999";
    auto r = std::from_chars(huge.data(), huge.data() + huge.size(), v);
    EXPECT_EQ(r.ec, std::errc::result_out_of_range);
}

// E.30: noexcept is the specification that remains; a function without it may throw.
TEST(CoreGuidelinesE, E30_NoexceptIsTheOnlySpecification) {
    static_assert(noexcept(twice(1)));
    static_assert(!noexcept(checked(1)));
    EXPECT_EQ(twice(21), 42);
    EXPECT_THROW(checked(-1), std::out_of_range);
}

// E.31: handlers are tried in order: the most derived first, catch (...) last.
TEST(CoreGuidelinesE, E31_FirstMatchingHandlerWins) {
    EXPECT_EQ(which_handler(0), "out_of_range");
    EXPECT_EQ(which_handler(1), "exception");
    EXPECT_EQ(which_handler(2), "other");
}
