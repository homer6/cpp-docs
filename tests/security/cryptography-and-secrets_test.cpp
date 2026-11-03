// Checks claims made by docs/security/cryptography-and-secrets.md.
#include <array>
#include <cstddef>
#include <cstring>
#include <random>
#include <span>
#include <string_view>
#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#elif defined(__APPLE__)
extern "C" int memset_s(void* s, std::size_t smax, int c, std::size_t n);
#endif

#include <gtest/gtest.h>

namespace {

void secure_zero(void* p, std::size_t n) noexcept {
#if defined(_WIN32)
    SecureZeroMemory(p, n);
#elif defined(__APPLE__)
    memset_s(p, n, 0, n);
#elif defined(__GLIBC__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    explicit_bzero(p, n);
#else
    auto* v = static_cast<volatile unsigned char*>(p);
    while (n--) *v++ = 0;
#endif
}

bool equal_secret(std::span<const unsigned char> a, std::span<const unsigned char> b) noexcept {
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (std::size_t i = 0; i < a.size(); ++i) diff |= a[i] ^ b[i];
    return diff == 0;
}

std::span<const unsigned char> bytes(std::string_view s) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): unsigned char may view any bytes
    return {reinterpret_cast<const unsigned char*>(s.data()), s.size()};
}

}  // namespace

// "Get randomness from the operating system": a <random> engine seeded the same way
// produces the same sequence, so its output is predictable.
// NOLINTBEGIN(bugprone-random-generator-seed,cert-msc32-c,cert-msc51-cpp): predictability is what's shown
TEST(SecurityCrypto, SeededEngineRepeats) {
    std::mt19937 a{12345}, b{12345};
    for (int i = 0; i < 1000; ++i) ASSERT_EQ(a(), b());
    std::mt19937_64 c{1}, d{1};
    EXPECT_EQ(c(), d());
}
// NOLINTEND(bugprone-random-generator-seed,cert-msc32-c,cert-msc51-cpp)

// "Keep secrets out of ... long-lived memory": the platform erase zeroes the buffer.
TEST(SecurityCrypto, SecureZeroErases) {
    std::array<unsigned char, 32> key{};
    key.fill(0xA5);
    secure_zero(key.data(), key.size());
    for (unsigned char c : key) ASSERT_EQ(c, 0);
}

// "Compare secrets in constant time": the comparison is correct for equal, different and
// different-length inputs (timing isn't asserted here).
TEST(SecurityCrypto, EqualSecretIsCorrect) {
    EXPECT_TRUE(equal_secret(bytes("token-123"), bytes("token-123")));
    EXPECT_FALSE(equal_secret(bytes("token-123"), bytes("token-124")));
    EXPECT_FALSE(equal_secret(bytes("token-123"), bytes("Token-123")));
    EXPECT_FALSE(equal_secret(bytes("token"), bytes("token-123")));
    EXPECT_TRUE(equal_secret(bytes(""), bytes("")));
}
