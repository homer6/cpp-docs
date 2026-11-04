// Checks claims made by docs/security/error-handling.md.
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {

using Policy = std::function<bool(const std::string&, const std::string&)>;

bool authorize(const Policy& policy, const std::string& user, const std::string& action) noexcept {
    try {
        return policy(user, action);
    } catch (...) {
        return false;
    }
}

struct Credentials {
    std::string password_hash;
    std::vector<std::string> recovery_codes;
};

void rotate(Credentials& current, std::string new_hash, std::vector<std::string> new_codes) {
    Credentials next{std::move(new_hash), std::move(new_codes)};
    std::swap(current, next);
}

// A value whose copy throws on demand, to fail an update halfway.
struct Flaky {
    static inline bool fail = false;
    std::string v;
    explicit Flaky(std::string s) : v(std::move(s)) {}
    Flaky(const Flaky& o) : v(o.v) { if (fail) throw std::runtime_error("copy failed"); }
    Flaky& operator=(const Flaky&) = default;
    Flaky(Flaky&&) = delete;               // copies only, so every element goes through the throwing copy
    Flaky& operator=(Flaky&&) = delete;
    ~Flaky() = default;
};

}  // namespace

// "Deny when the decision can't be made": a throwing policy (an outage) denies.
TEST(SecurityErrorHandling, ThrowingPolicyDenies) {
    Policy allow_all = [](const std::string&, const std::string&) { return true; };
    Policy outage = [](const std::string&, const std::string&) -> bool { throw std::runtime_error("timeout"); };
    EXPECT_TRUE(authorize(allow_all, "alice", "read"));
    EXPECT_FALSE(authorize(outage, "alice", "read"));
    EXPECT_FALSE(authorize(Policy{}, "alice", "read"));   // no policy at all: bad_function_call, denied
}

// "Leave a defined state": rotate() changes both fields or neither.
TEST(SecurityErrorHandling, RotateCommitsBothFields) {
    Credentials c{"old", {"a", "b"}};
    rotate(c, "new", {"x"});
    EXPECT_EQ(c.password_hash, "new");
    EXPECT_EQ(c.recovery_codes, std::vector<std::string>{"x"});
}

// "Leave a defined state": when building the new state throws, the old state is untouched.
TEST(SecurityErrorHandling, FailureBeforeCommitLeavesOldState) {
    std::vector<Flaky> current{Flaky{"one"}, Flaky{"two"}};
    std::vector<Flaky> incoming{Flaky{"three"}, Flaky{"four"}};
    Flaky::fail = true;
    EXPECT_THROW({
        std::vector<Flaky> next = incoming;   // the copy throws before the commit
        std::swap(current, next);
    }, std::runtime_error);
    Flaky::fail = false;
    ASSERT_EQ(current.size(), 2u);
    EXPECT_EQ(current[0].v, "one");
    EXPECT_EQ(current[1].v, "two");
}

// "Put exception boundaries where control leaves your code": swap is noexcept for
// standard types, which is what makes it safe as the commit step.
TEST(SecurityErrorHandling, CommitStepCantThrow) {
    static_assert(noexcept(std::swap(std::declval<Credentials&>(), std::declval<Credentials&>())));
    SUCCEED();
}
