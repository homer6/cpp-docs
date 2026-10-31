// Checks claims made by docs/examples/execution/senders-receivers.md.
//
// No local toolchain defines __cpp_lib_senders yet (GCC 15.2, GCC 16.2,
// Clang 22 + libc++), so these tests skip with that reason until one does.
#include <exception>
#include <stdexcept>
#include <optional>
#include <string>
#include <thread>
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

// "Sender factories": just_stopped completes as cancelled, so sync_wait returns
// an empty optional; upon_error turns the error channel back into a value.
TEST(SendersReceivers, StoppedAndUponError) {
    CPPDOCS_REQUIRE(__cpp_lib_senders);
#ifdef __cpp_lib_senders
    namespace ex = std::execution;
    EXPECT_FALSE(std::this_thread::sync_wait(ex::just_stopped()).has_value());
    auto err = std::make_exception_ptr(std::runtime_error{"boom"});
    auto handled = ex::just_error(err) | ex::upon_error([](std::exception_ptr) { return 7; });
    auto [v] = std::this_thread::sync_wait(std::move(handled)).value();
    EXPECT_EQ(v, 7);
#endif
}

// "Worked example": on(run_loop scheduler, ...) runs the work on the loop's thread.
TEST(SendersReceivers, OnRunLoopRunsOnWorker) {
    CPPDOCS_REQUIRE(__cpp_lib_senders);
#ifdef __cpp_lib_senders
    namespace ex = std::execution;
    ex::run_loop loop;
    std::jthread worker([&](std::stop_token st) {
        std::stop_callback cb{st, [&] { loop.finish(); }};
        loop.run();
    });
    auto work = ex::on(loop.get_scheduler(),
                       ex::just(std::string{"hi"}) | ex::then([](std::string m) {
                           return std::pair{m.size(), std::this_thread::get_id()};
                       }));
    auto [r] = std::this_thread::sync_wait(std::move(work)).value();
    EXPECT_EQ(r.first, 2u);
    EXPECT_EQ(r.second, worker.get_id());
#endif
}
