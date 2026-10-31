// Checks claims made by docs/examples/diagnostics/exception-hierarchy.md.
#include <exception>
#include <functional>
#include <map>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <typeinfo>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

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
