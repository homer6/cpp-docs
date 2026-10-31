// Checks claims made by docs/examples/language/exceptions/throw-try-catch.md.
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <gtest/gtest.h>

namespace {
int parse(std::string_view s) {  // the page's example
    if (s.empty()) throw std::invalid_argument{"empty input"};
    return 0;
}

struct MyError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

std::string unwind_log;
struct Guard {
    char name;
    ~Guard() { unwind_log += name; }
};
void risky() { Guard g{'b'}; throw std::runtime_error{"boom"}; }
void outer() { Guard g{'a'}; risky(); }

std::mutex unwind_mutex;
void locked_risky() {
    std::lock_guard lk{unwind_mutex};
    risky();
}

struct ThrowingDtor {
    ~ThrowingDtor() noexcept(false) { throw std::runtime_error{"from destructor"}; }
};
void throw_during_unwinding() {
    ThrowingDtor d;
    throw std::runtime_error{"first"};  // d's destructor throws a second exception while unwinding
}

// Catching by value is the point here.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wcatch-value"
#endif
std::string catch_by_value() {
    try { throw MyError{"msg"}; }
    catch (std::runtime_error e) {  // by value: copies, slicing to the base
        try { throw e; } catch (const MyError&) { return "MyError"; } catch (...) { return "sliced"; }
    }
}
}  // namespace

// "The basics": parse("") throws std::invalid_argument; the most-derived handler wins.
TEST(ThrowTryCatch, HandlersMatchInOrder) {
    EXPECT_THROW(parse(""), std::invalid_argument);
    EXPECT_NO_THROW(parse("x"));
    std::string which;
    try {
        parse("");
    } catch (const std::invalid_argument& e) {
        which = std::string("invalid_argument: ") + e.what();
    } catch (const std::exception&) {
        which = "exception";
    }
    EXPECT_EQ(which, "invalid_argument: empty input");
}

// "Stack unwinding": destructors of locals run, innermost first, before the handler.
TEST(ThrowTryCatch, StackUnwindingRunsDestructors) {
    unwind_log.clear();
    try {
        outer();
    } catch (const std::runtime_error&) {
        unwind_log += '!';
    }
    EXPECT_EQ(unwind_log, "ba!");
}

// "Gotchas": bare throw; keeps the dynamic type; throw e; through a base reference slices.
TEST(ThrowTryCatch, BareThrowPreservesTypeThrowECopySlices) {
    auto rethrow_bare = [] {
        try { throw MyError{"msg"}; } catch (const std::runtime_error&) { throw; }
    };
    auto rethrow_copy = [] {
        try { throw MyError{"msg"}; } catch (const std::runtime_error& e) { throw e; }
    };
    EXPECT_THROW(rethrow_bare(), MyError);
    bool caught_as_my_error = false;
    try { rethrow_copy(); } catch (const MyError&) { caught_as_my_error = true; } catch (const std::runtime_error&) {}
    EXPECT_FALSE(caught_as_my_error);
}

// "Rethrow & nested exceptions": throw_with_nested keeps the cause, rethrow_if_nested gets it back.
TEST(ThrowTryCatch, NestedExceptionsPreserveCause) {
    try {
        throw 0;
    } catch (...) {
        std::nested_exception probe;  // must capture the exception being handled
        if (!probe.nested_ptr())
            GTEST_SKIP() << "std::nested_exception doesn't capture current_exception() on this "
                            "toolchain (seen with Homebrew GCC 15 on macOS); "
                            "std::rethrow_if_nested would terminate";
    }
    std::string outer_msg, inner_msg;
    try {
        try { throw std::invalid_argument{"low"}; }
        catch (...) { std::throw_with_nested(std::runtime_error{"high-level failed"}); }
    } catch (const std::runtime_error& e) {
        outer_msg = e.what();
        try { std::rethrow_if_nested(e); } catch (const std::invalid_argument& inner) { inner_msg = inner.what(); }
    }
    EXPECT_EQ(outer_msg, "high-level failed");
    EXPECT_EQ(inner_msg, "low");
}

// "Gotchas": destructors are noexcept by default.
TEST(ThrowTryCatch, DestructorsAreNoexceptByDefault) {
    static_assert(std::is_nothrow_destructible_v<Guard>);
    static_assert(noexcept(std::declval<Guard&>().~Guard()));
}

// "The basics": catch (...) catches anything, including non-std::exception types.
TEST(ThrowTryCatch, CatchAllCatchesAnything) {
    std::string which;
    try { throw 42; } catch (const std::exception&) { which = "exception"; } catch (...) { which = "..."; }
    EXPECT_EQ(which, "...");
}

// "Stack unwinding": the lock_guard's destructor releases the mutex when risky() throws.
TEST(ThrowTryCatch, UnwindingReleasesLock) {
    EXPECT_THROW(locked_risky(), std::runtime_error);
    EXPECT_TRUE(unwind_mutex.try_lock());
    unwind_mutex.unlock();
}

// "Gotchas": catching by value slices a derived exception to the base.
TEST(ThrowTryCatch, CatchByValueSlices) {
    EXPECT_EQ(catch_by_value(), "sliced");
}

// "Gotchas": a second exception escaping a destructor during unwinding calls std::terminate.
TEST(ThrowTryCatchDeathTest, ThrowDuringUnwindingTerminates) {
    EXPECT_DEATH(
        {
            try { throw_during_unwinding(); } catch (...) {}
        },
        "");
}
