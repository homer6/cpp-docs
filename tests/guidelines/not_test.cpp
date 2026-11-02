// Checks claims made by docs/guidelines/core-guidelines/examples/not.md.
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace {
int classify(int x) {
    if (x < 0) return -1;
    if (x == 0) return 0;
    return 1;
}

int parse_port(const std::string& s) {
    if (s.empty()) throw std::invalid_argument("empty port");
    return std::stoi(s);
}

class Connection {
public:
    explicit Connection(std::string host) : host_{std::move(host)} {
        if (host_.empty()) throw std::invalid_argument("no host");
    }
    const std::string& host() const { return host_; }
private:
    std::string host_;
};

int released = 0;
struct Resource { ~Resource() { ++released; } };

int early_exit(bool first_fails, bool second_fails) {
    auto a = std::make_unique<Resource>();
    if (first_fails) return -1;
    auto b = std::make_unique<Resource>();
    if (second_fails) return -2;
    return 0;
}

class Account {
public:
    long balance() const { return balance_; }
protected:
    void deposit(long amount) { if (amount > 0) balance_ += amount; }
private:
    long balance_ = 0;
};
class Promo : public Account {
public:
    void bonus() { deposit(100); }
    void bad_bonus() { deposit(-1000); }
};

template <class T>
concept balance_visible = requires(T& t) { t.balance_; };
}  // namespace

// NR.2: early returns give the same answers as a single-exit version, with less code.
TEST(CoreGuidelinesNR, NR2_EarlyReturns) {
    EXPECT_EQ(classify(-5), -1);
    EXPECT_EQ(classify(0), 0);
    EXPECT_EQ(classify(9), 1);
}

// NR.3: an exception reports the error and can't be silently ignored like a return code.
TEST(CoreGuidelinesNR, NR3_ExceptionsCannotBeIgnored) {
    EXPECT_THROW(static_cast<void>(parse_port("")), std::invalid_argument);
    EXPECT_EQ(parse_port("8080"), 8080);
}

// NR.5: a constructor that establishes the invariant leaves no half-built state; there's no default constructor.
TEST(CoreGuidelinesNR, NR5_OnePhaseInitialization) {
    static_assert(!std::is_default_constructible_v<Connection>);
    EXPECT_THROW(Connection{""}, std::invalid_argument);
    EXPECT_EQ(Connection{"example.org"}.host(), "example.org");
}

// NR.6: destructors clean up on every exit path, with no goto.
TEST(CoreGuidelinesNR, NR6_RaiiInsteadOfGotoExit) {
    released = 0;
    EXPECT_EQ(early_exit(true, false), -1);
    EXPECT_EQ(released, 1);
    EXPECT_EQ(early_exit(false, true), -2);
    EXPECT_EQ(released, 3);
    EXPECT_EQ(early_exit(false, false), 0);
    EXPECT_EQ(released, 5);
}

// NR.7: with private data, a derived class goes through functions that keep the invariant.
TEST(CoreGuidelinesNR, NR7_PrivateDataKeepsTheInvariant) {
    static_assert(!balance_visible<Promo>);
    Promo p;
    p.bonus();
    p.bad_bonus();
    EXPECT_EQ(p.balance(), 100);
}
