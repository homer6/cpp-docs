// Checks claims made by docs/examples/execution/senders-receivers.md.
//
// No local toolchain defines __cpp_lib_senders yet (GCC 15.2, GCC 16.2,
// Clang 22 + libc++), so these tests skip with that reason until one does.
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <version>

#ifdef __cpp_lib_senders
#include <execution>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Sender adaptors" + "Running it": just(21) | then(x*2+1) yields 43 via sync_wait.
TEST(SendersReceivers, ThenChainSyncWait) {
    CPPDOCS_REQUIRE(__cpp_lib_senders);
#ifdef __cpp_lib_senders
    namespace ex = std::execution;
    auto work = ex::just(21) | ex::then([](int x) { return x * 2; }) |
                ex::then([](int x) { return x + 1; });
    auto [result] = std::this_thread::sync_wait(std::move(work)).value();
    EXPECT_EQ(result, 43);
#endif
}

// "Gotchas": senders are lazy; nothing runs until the work is started.
TEST(SendersReceivers, SendersAreLazy) {
    CPPDOCS_REQUIRE(__cpp_lib_senders);
#ifdef __cpp_lib_senders
    namespace ex = std::execution;
    bool ran = false;
    auto work = ex::just() | ex::then([&ran] { ran = true; });
    EXPECT_FALSE(ran);
    std::this_thread::sync_wait(std::move(work));
    EXPECT_TRUE(ran);
#endif
}

// "Running it": sync_wait rethrows an error sent on the error channel.
TEST(SendersReceivers, SyncWaitRethrowsError) {
    CPPDOCS_REQUIRE(__cpp_lib_senders);
#ifdef __cpp_lib_senders
    namespace ex = std::execution;
    auto err = std::make_exception_ptr(std::runtime_error{"boom"});
    EXPECT_THROW(std::this_thread::sync_wait(ex::just_error(err)), std::runtime_error);
#endif
}
