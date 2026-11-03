// Checks claims made by docs/security/object-representation.md.
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

namespace {

struct Session {
    bool authenticated = false;
    int user_id = -1;
};

struct Header {
    std::uint8_t kind;
    std::uint32_t length;
};

struct NoPadding {
    std::uint32_t a;
    std::uint32_t b;
};

std::array<std::uint8_t, 5> encode(const Header& h) {
    return {h.kind,
            static_cast<std::uint8_t>(h.length),
            static_cast<std::uint8_t>(h.length >> 8),
            static_cast<std::uint8_t>(h.length >> 16),
            static_cast<std::uint8_t>(h.length >> 24)};
}

struct Message {                       // C.67: a polymorphic base suppresses copying
    Message() = default;
    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;
    Message(Message&&) = delete;
    Message& operator=(Message&&) = delete;
    virtual ~Message() = default;
};
struct Login : Message { std::string user = "alice"; };
struct Ping : Message { int seq = 0; };

std::size_t user_length(const Message& m) {
    if (auto* login = dynamic_cast<const Login*>(&m)) return login->user.size();
    return 0;
}

}  // namespace

// "Initialize before use": default member initializers make a default-constructed
// session deny access.
TEST(SecurityObjectRepresentation, DefaultMemberInitializersDeny) {
    Session s;
    EXPECT_FALSE(s.authenticated);
    EXPECT_EQ(s.user_id, -1);
}

// "Serialize field by field": a struct with a uint8_t then a uint32_t has padding, and
// has_unique_object_representations says so; one without padding is unique.
TEST(SecurityObjectRepresentation, PaddingDetectedByTrait) {
    EXPECT_GT(sizeof(Header), sizeof(std::uint8_t) + sizeof(std::uint32_t));
    static_assert(!std::has_unique_object_representations_v<Header>);
    static_assert(std::has_unique_object_representations_v<NoPadding>);
    SUCCEED();
}

// "Serialize field by field": the wire format is exactly the fields, little-endian,
// regardless of the host's layout or byte order.
TEST(SecurityObjectRepresentation, FieldByFieldEncodingIsExact) {
    auto bytes = encode(Header{7, 0x01020304u});
    std::array<std::uint8_t, 5> expected{7, 0x04, 0x03, 0x02, 0x01};
    EXPECT_EQ(bytes, expected);
    EXPECT_EQ(bytes.size(), 5u);   // no padding bytes on the wire
}

// "Confine casts that skip type checks": dynamic_cast checks the dynamic type, so a
// message of the wrong type is rejected instead of misused.
TEST(SecurityObjectRepresentation, DynamicCastRejectsWrongType) {
    Login login;
    Ping ping;
    EXPECT_EQ(user_length(login), 5u);
    EXPECT_EQ(user_length(ping), 0u);
}

// "Confine casts": bit_cast reinterprets a value's bits through a copy, with matching sizes.
TEST(SecurityObjectRepresentation, BitCastCopiesBits) {
    static_assert(std::bit_cast<std::uint32_t>(1.0f) == 0x3f800000u);
    EXPECT_EQ(std::bit_cast<float>(0x3f800000u), 1.0f);
}
