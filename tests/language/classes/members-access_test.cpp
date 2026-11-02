// Checks claims made by docs/examples/language/classes/members-access.md.
#include <sstream>
#include <type_traits>
#include <utility>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {

class Account {  // "Members"
public:
    explicit Account(double balance) : balance_{balance} { ++count_; }
    double balance() const { return balance_; }
    void deposit(double amount) { balance_ += amount; }
    static int count() { return count_; }
private:
    double balance_ = 0.0;
    static inline int count_ = 0;
};

template <class T>
concept CanDeposit = requires(T& a) { a.deposit(1.0); };
template <class T>
concept CanReadBalance = requires(T& a) { a.balance(); };

class C { int x = 0; };
struct S { int x = 0; };
template <class T>
concept PublicX = requires(T t) { t.x; };

class Vault {  // "friend — selective access"
    int secret_ = 42;
    friend class Auditor;
    friend int inspect(const Vault& v) { return v.secret_; }
    friend std::ostream& operator<<(std::ostream& os, const Vault& v) { return os << v.secret_; }
};
class Auditor {
public:
    static int read(const Vault& v) { return v.secret_; }
};
template <class T>
concept PublicSecret = requires(T t) { t.secret_; };

struct Builder {  // "this and ref-qualified members"
    int n = 0;
    Builder& set(int v) & { n = v; return *this; }
    Builder build() && { return std::move(*this); }
};
template <class T>
concept CanSet = requires(T&& b) { std::forward<T>(b).set(1); };
template <class T>
concept CanBuild = requires(T&& b) { std::forward<T>(b).build(); };

}  // namespace

// "Members": const members work on const objects; static members are shared by the class.
TEST(MembersAccess, ConstAndStaticMembers) {
    const int before = Account::count();
    Account a{10.0};
    a.deposit(5.0);
    EXPECT_DOUBLE_EQ(a.balance(), 15.0);
    const Account b{1.0};
    EXPECT_EQ(Account::count(), before + 2);
    static_assert(CanReadBalance<const Account>);
    static_assert(!CanDeposit<const Account>);
    static_assert(CanDeposit<Account>);
    (void)b;
}

// "Access specifiers": class defaults to private, struct to public.
TEST(MembersAccess, DefaultAccess) {
    static_assert(!PublicX<C>);
    static_assert(PublicX<S>);
}

// "friend — selective access": friends read privates without making them public.
TEST(MembersAccess, FriendGrantsAccess) {
    static_assert(!PublicSecret<Vault>);
    Vault v;
    EXPECT_EQ(Auditor::read(v), 42);
    EXPECT_EQ(inspect(v), 42);
    std::ostringstream os;
    os << v;
    EXPECT_EQ(os.str(), "42");
}

// "this and ref-qualified members": set() needs an lvalue, build() an rvalue.
TEST(MembersAccess, RefQualifiedMembers) {
    static_assert(CanSet<Builder&>);
    static_assert(!CanSet<Builder>);
    static_assert(CanBuild<Builder>);
    static_assert(!CanBuild<Builder&>);
    Builder b;
    b.set(3);
    Builder out = std::move(b).build();
    EXPECT_EQ(out.n, 3);
}

#if __cpp_variadic_friend >= 202403L  // MSVC has no C++26 core features yet
namespace passkey {
template <class... Ts>
class Passkey {
    friend Ts...;
    Passkey() = default;
};
class Registry {
public:
    int added = 0;
    void add(Passkey<class Loader, class Admin>) { ++added; }
};
class Loader { public: void run(Registry& r) { r.add({}); } };
class Admin { public: void run(Registry& r) { r.add({}); } };
class Stranger {};
}  // namespace passkey
#endif

// "friend — selective access": variadic friends (C++26) and the passkey idiom.
TEST(MembersAccess, VariadicFriends) {
    CPPDOCS_REQUIRE_VERSION(__cpp_variadic_friend, 202403L);
#if __cpp_variadic_friend >= 202403L
    passkey::Registry r;
    passkey::Loader{}.run(r);
    passkey::Admin{}.run(r);
    EXPECT_EQ(r.added, 2);
    static_assert(!std::is_default_constructible_v<passkey::Passkey<passkey::Loader>>);  // private to outsiders
#endif
}
