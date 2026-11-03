// Checks claims made by docs/security/threat-modeling.md.
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace {

class RecordId {
public:
    static std::optional<RecordId> parse(std::string_view text) {
        if (text.empty() || text.size() > 32) return std::nullopt;
        for (char c : text)
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return std::nullopt;
        return RecordId{std::string{text}};
    }
    std::string_view value() const { return value_; }

private:
    explicit RecordId(std::string v) : value_{std::move(v)} {}
    std::string value_;
};

struct User { std::string name; bool may_write = false; };
enum class Decision { allow, deny };

Decision authorize_write(const User& user, const RecordId&) {
    return user.may_write ? Decision::allow : Decision::deny;
}

struct Store {
    std::vector<std::string> written;
    bool store_record(const User& user, const RecordId& id, std::span<const std::byte>) {
        if (authorize_write(user, id) != Decision::allow) return false;
        written.emplace_back(id.value());
        return true;
    }
};

}  // namespace

// "Keep the trusted core small": a validated value is a type the parser alone can make,
// so raw input can't reach the privileged interface.
TEST(SecurityThreatModeling, ValidatedTypeCantBeMadeFromRawInput) {
    static_assert(!std::is_constructible_v<RecordId, std::string>);
    static_assert(!std::is_convertible_v<std::string, RecordId>);
    static_assert(!std::is_convertible_v<const char*, RecordId>);
    SUCCEED();
}

// "Keep the trusted core small": the parser accepts the expected shape and rejects the rest.
TEST(SecurityThreatModeling, ParserRejectsMalformedIds) {
    ASSERT_TRUE(RecordId::parse("abc123").has_value());
    EXPECT_EQ(RecordId::parse("abc123")->value(), "abc123");
    EXPECT_FALSE(RecordId::parse("").has_value());
    EXPECT_FALSE(RecordId::parse("../../etc/passwd").has_value());
    EXPECT_FALSE(RecordId::parse("ABC").has_value());
    EXPECT_FALSE(RecordId::parse(std::string(33, 'a')).has_value());
    EXPECT_FALSE(RecordId::parse(std::string_view("a\0b", 3)).has_value());
}

// "Authorize every request, separately from parsing": a well-formed request from a user
// without permission is denied, and nothing is written.
TEST(SecurityThreatModeling, ParsedIsNotAuthorized) {
    Store s;
    auto id = RecordId::parse("report7");
    ASSERT_TRUE(id);
    std::byte data[1]{};
    EXPECT_FALSE(s.store_record(User{"mallory", false}, *id, data));
    EXPECT_TRUE(s.written.empty());
    EXPECT_TRUE(s.store_record(User{"alice", true}, *id, data));
    EXPECT_EQ(s.written.size(), 1u);
}

// "Authorize every request": deny by default (a default-constructed user has no permission).
TEST(SecurityThreatModeling, DenyByDefault) {
    auto id = RecordId::parse("x");
    ASSERT_TRUE(id);
    EXPECT_EQ(authorize_write(User{}, *id), Decision::deny);
}
