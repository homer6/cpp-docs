// Included before every ```cpp block checked by tools/check_snippets.py.
// Snippets are illustrations, so they lean on a few placeholder names; this
// declares them (declarations only — snippets are never linked or run).
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <format>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <numbers>
#include <numeric>
#include <optional>
#include <ranges>
#include <set>
#include <shared_mutex>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#if __has_include(<print>)
#include <print>
#endif
#if __has_include(<generator>)
#include <generator>
#endif
// <meta> only when reflection is on (GCC 16 needs -freflection).
#if defined(__cpp_impl_reflection) && __has_include(<meta>)
#include <meta>
#endif

// Placeholder types and functions used across the docs.
struct Widget {
    Widget() = default;
    explicit Widget(int) {}
    void use() const {}
    void run() const {}
    void foo() const {}
};
template <class... Ts> void use(Ts&&...) {}
template <class... Ts> void process(Ts&&...) {}
template <class... Ts> void call(Ts&&...) {}
