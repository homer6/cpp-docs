// Checks claims made by docs/examples/diagnostics/exception-hierarchy.md.
#include <exception>
#include <functional>
#include <future>
#include <map>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <type_traits>
#include <typeinfo>
#include <variant>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "The tree": each class derives from the parent the diagram shows.
TEST(ExceptionHierarchy, TreeShape) {
    static_assert(std::is_base_of_v<std::exception, std::logic_error>);
    static_assert(std::is_base_of_v<std::logic_error, std::invalid_argument>);
    static_assert(std::is_base_of_v<std::logic_error, std::domain_error>);
    static_assert(std::is_base_of_v<std::logic_error, std::length_error>);
    static_assert(std::is_base_of_v<std::logic_error, std::out_of_range>);
    static_assert(std::is_base_of_v<std::exception, std::runtime_error>);
    static_assert(std::is_base_of_v<std::runtime_error, std::range_error>);
    static_assert(std::is_base_of_v<std::runtime_error, std::overflow_error>);
    static_assert(std::is_base_of_v<std::runtime_error, std::underflow_error>);
    static_assert(std::is_base_of_v<std::runtime_error, std::system_error>);
    static_assert(std::is_base_of_v<std::exception, std::bad_alloc>);
    static_assert(std::is_base_of_v<std::exception, std::bad_cast>);
    static_assert(std::is_base_of_v<std::exception, std::bad_optional_access>);
    static_assert(std::is_base_of_v<std::exception, std::bad_variant_access>);
    static_assert(std::is_base_of_v<std::exception, std::bad_function_call>);
}

// "The tree": the library functions named there throw the listed exceptions.
TEST(ExceptionHierarchy, LibraryThrowsListedTypes) {
    std::vector<int> v;
    std::map<int, int> m;
    std::optional<int> o;
    std::variant<int, double> var = 1;
    std::function<void()> f;
    EXPECT_THROW((void)v.at(0), std::out_of_range);
    EXPECT_THROW((void)m.at(0), std::out_of_range);
    EXPECT_THROW((void)o.value(), std::bad_optional_access);
    EXPECT_THROW((void)std::get<double>(var), std::bad_variant_access);
    EXPECT_THROW(f(), std::bad_function_call);
}

// "Throwing" and "Catch by const reference": most-derived handler first, base catches the rest.
TEST(ExceptionHierarchy, CatchOrderAndWhat) {
    auto check_index = [](std::size_t index, std::size_t size) {
        if (index >= size)
            throw std::out_of_range{"index " + std::to_string(index) + " out of range"};
    };
    std::string caught;
    try {
        check_index(5, 3);
    } catch (const std::out_of_range& e) {
        caught = std::string("out_of_range: ") + e.what();
    } catch (const std::exception&) {
        caught = "exception";
    }
    EXPECT_EQ(caught, "out_of_range: index 5 out of range");
}

// "Custom exceptions" and "Gotchas": a runtime_error subclass is caught by
// catch (const std::exception&); catching by value slices it.
class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& msg) : std::runtime_error{msg} {}
};

TEST(ExceptionHierarchy, CustomExceptionAndSlicing) {
    try {
        throw ConfigError{"bad key"};
    } catch (const std::exception& e) {
        EXPECT_STREQ(e.what(), "bad key");
        EXPECT_EQ(typeid(e), typeid(ConfigError));  // dynamic type preserved by reference
    }
    try {
        throw ConfigError{"bad key"};
    } catch (const std::runtime_error& e) {
        std::runtime_error by_value = e;  // what catch (std::runtime_error e) would copy
        EXPECT_EQ(typeid(by_value), typeid(std::runtime_error));  // sliced
    }
}

// "The tree": dynamic_cast to a reference throws bad_cast; a failed allocation bad_alloc.
TEST(ExceptionHierarchy, BadCastAndBadAlloc) {
    struct B { virtual ~B() = default; };
    struct D : B {};
    B b;
    B& rb = b;
    EXPECT_THROW((void)dynamic_cast<D&>(rb), std::bad_cast);
    std::vector<int> v;
    EXPECT_THROW(v.reserve(v.max_size()), std::bad_alloc);
    EXPECT_THROW(v.reserve(v.max_size() + 1), std::length_error);  // a logic_error
}

// "Gotchas": catch blocks are tried in order, so a base handler first shadows a derived one.
TEST(ExceptionHierarchy, BaseHandlerFirstShadows) {
    std::string caught;
    try {
        throw std::out_of_range{"x"};
    } catch (const std::exception&) {
        caught = "exception";
    }
    EXPECT_EQ(caught, "exception");
}

// "Capture and rethrow: std::exception_ptr": an exception caught in a worker
// thread is stored and rethrown in the joining thread with its type intact.
TEST(ExceptionHierarchy, ExceptionPtrMovesExceptionAcrossThreads) {
    EXPECT_EQ(std::current_exception(), nullptr);  // null outside a handler
    auto run_worker = [] {
        std::exception_ptr err;
        EXPECT_EQ(err, nullptr);  // null until something is stored
        std::thread worker{[&err] {
            try {
                throw std::runtime_error{"worker failed"};
            } catch (...) {
                err = std::current_exception();
            }
        }};
        worker.join();
        if (err) std::rethrow_exception(err);
    };
    std::string caught;
    try {
        run_worker();
    } catch (const std::runtime_error& e) {
        caught = e.what();
    }
    EXPECT_EQ(caught, "worker failed");
}

// "Capture and rethrow": make_exception_ptr needs no throw, and a promise
// carries it to future::get, which rethrows it.
TEST(ExceptionHierarchy, MakeExceptionPtrThroughPromise) {
    auto failed_result = [] {
        std::promise<int> promise;
        promise.set_exception(std::make_exception_ptr(std::runtime_error{"no result"}));
        return promise.get_future();
    };
    std::future<int> result = failed_result();
    try {
        (void)result.get();
        FAIL() << "get() should have rethrown";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "no result");
    }
}

// "Is this scope failing? std::uncaught_exceptions": the count-comparing guard
// commits on normal exit, rolls back during unwinding, and commits when it is
// created and destroyed inside a destructor that runs during unwinding.
namespace {
struct CountingGuard {
    std::string* log;
    int exceptions_on_entry = std::uncaught_exceptions();
    ~CountingGuard() { *log += std::uncaught_exceptions() > exceptions_on_entry ? "rollback;" : "commit;"; }
};
struct RunsGuardInDestructor {
    std::string* log;
    ~RunsGuardInDestructor() { CountingGuard g{log}; }
};
}  // namespace

TEST(ExceptionHierarchy, UncaughtExceptionsScopeGuard) {
    static_assert(__cpp_lib_uncaught_exceptions >= 201411L);
    std::string log;
    { CountingGuard g{&log}; }
    try { CountingGuard g{&log}; throw 1; } catch (...) {}
    try { RunsGuardInDestructor r{&log}; throw 2; } catch (...) {}
    EXPECT_EQ(log, "commit;rollback;commit;");
    try {
        throw 3;
    } catch (...) {
        EXPECT_EQ(std::uncaught_exceptions(), 0);  // a caught exception no longer counts
    }
}

// "Is this scope failing?": the removed bool uncaught_exception() is true for a
// guard that lives entirely inside a destructor run by unwinding, so it can't
// tell that guard's own scope is ending normally. libc++ no longer declares it.
#if defined(__GLIBCXX__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
namespace {
struct OldApiProbe {
    bool* old_said_unwinding;
    ~OldApiProbe() {
        struct Guard {
            bool* out;
            ~Guard() { *out = std::uncaught_exception(); }
        } g{old_said_unwinding};
    }
};
}  // namespace
#pragma GCC diagnostic pop
#endif

TEST(ExceptionHierarchy, OldUncaughtExceptionCantTellNestedScopes) {
#if defined(__GLIBCXX__)
    bool old_said_unwinding = false;
    try { OldApiProbe p{&old_said_unwinding}; throw 1; } catch (...) {}
    EXPECT_TRUE(old_said_unwinding);  // although the guard's own scope ended normally
#else
    GTEST_SKIP() << "std::uncaught_exception was removed in C++20 and this library no longer declares it";
#endif
}

// "Inspect without rethrowing: std::exception_ptr_cast": a match through a base
// class, an empty result for another type or a null pointer, the deleted
// rvalue overload, and the return type for each feature-macro value.
#if defined(__cpp_lib_exception_ptr_cast)
template <class P>
concept CastAcceptsRvalue = requires(P&& p) { std::exception_ptr_cast<std::exception>(std::move(p)); };
#endif

TEST(ExceptionHierarchy, ExceptionPtrCast) {
    CPPDOCS_REQUIRE(__cpp_lib_exception_ptr_cast);
#if defined(__cpp_lib_exception_ptr_cast)
    auto err = std::make_exception_ptr(std::out_of_range{"cast me"});
    if (auto e = std::exception_ptr_cast<std::logic_error>(err))
        EXPECT_STREQ(e->what(), "cast me");
    else
        ADD_FAILURE() << "a catch (const std::logic_error&) would match out_of_range";
    EXPECT_FALSE(std::exception_ptr_cast<std::runtime_error>(err));
    std::exception_ptr none;
    EXPECT_FALSE(std::exception_ptr_cast<std::exception>(none));
    static_assert(!CastAcceptsRvalue<std::exception_ptr>);
    using R = decltype(std::exception_ptr_cast<std::logic_error>(err));
#if __cpp_lib_exception_ptr_cast >= 202603L
    static_assert(std::is_same_v<R, std::optional<const std::logic_error&>>);  // P3981R2
#else
    static_assert(std::is_same_v<R, const std::logic_error*>);  // P2927R3, as in GCC 16
#endif
#endif
}

// "Constexpr exceptions (C++26)": a vector::at out_of_range and a rethrown
// exception_ptr can both be caught inside a constant expression.
#if defined(__cpp_lib_constexpr_exceptions) && __cpp_lib_constexpr_exceptions >= 202502L
constexpr int safe_at(int i) {
    std::vector<int> v{1, 2, 3};
    try {
        return v.at(i);
    } catch (const std::out_of_range&) {
        return -1;
    }
}
constexpr bool rethrow_in_constexpr() {
    auto p = std::make_exception_ptr(std::runtime_error{"x"});
    try {
        std::rethrow_exception(p);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}
#endif

TEST(ExceptionHierarchy, ConstexprLibraryExceptions) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_constexpr_exceptions, 202502L);
#if defined(__cpp_lib_constexpr_exceptions) && __cpp_lib_constexpr_exceptions >= 202502L
    static_assert(safe_at(1) == 2);
    static_assert(safe_at(7) == -1);
    static_assert(rethrow_in_constexpr());
#endif
}
