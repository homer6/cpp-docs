// Checks claims made by docs/examples/text-processing/localization.md.
#include <clocale>
#include <cstring>
#include <format>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#ifdef __APPLE__
#include <cstdlib>
#include <sys/sysctl.h>
#endif

namespace {

#ifdef __APPLE__
// The running macOS's major version (kern.osproductversion, e.g. "15.5"), or 0.
int macos_major() {
    char buf[32] = {};
    std::size_t len = sizeof buf;
    if (sysctlbyname("kern.osproductversion", buf, &len, nullptr, 0) != 0) return 0;
    return std::atoi(buf);
}
#endif

// Returns the first of `names` this toolchain can construct, or nullopt.
std::optional<std::locale> try_locale(std::initializer_list<const char*> names) {
    for (const char* n : names) {
        try {
            return std::locale{n};
        } catch (const std::runtime_error&) {
        }
    }
    return std::nullopt;
}

std::string fixed2(const std::locale& loc, double v) {
    std::ostringstream os;
    os.imbue(loc);
    os << std::fixed << std::setprecision(2) << v;
    return os.str();
}

}  // namespace

// "The classic vs. the user's locale": the "C" locale prints 1234567.89 with no grouping.
TEST(Localization, ClassicHasNoGrouping) {
    EXPECT_EQ(fixed2(std::locale::classic(), 1234567.89), "1234567.89");
    EXPECT_EQ(std::locale::classic().name(), "C");
}

// "The classic vs. the user's locale": en_US groups as "1,234,567.89".
TEST(Localization, EnUsGrouping) {
    auto loc = try_locale({"en_US.UTF-8", "en_US.utf8", "en_US"});
    if (!loc) GTEST_SKIP() << "en_US locale not available to this C++ library";
    EXPECT_EQ(fixed2(*loc, 1234567.89), "1,234,567.89");
}

// "The classic vs. the user's locale": de_DE groups as "1.234.567,89" on glibc and on
// macOS 15 and later; macOS 14's de_DE has an empty numeric thousands separator
// (grouping CHAR_MAX), so it prints "1234567,89". The locale data comes from the
// running OS, so the expectation follows the OS version, not the SDK.
TEST(Localization, DeDeGrouping) {
    auto loc = try_locale({"de_DE.UTF-8", "de_DE.utf8", "de_DE"});
    if (!loc) GTEST_SKIP() << "de_DE locale not available to this C++ library";
#ifdef __APPLE__
    const int major = macos_major();
    ASSERT_GT(major, 0) << "couldn't read kern.osproductversion";
    EXPECT_EQ(fixed2(*loc, 1234567.89), major >= 15 ? "1.234.567,89" : "1234567,89") << "macOS " << major;
#else
    EXPECT_EQ(fixed2(*loc, 1234567.89), "1.234.567,89");
#endif
}

// "Locale-aware comparison": locale::operator() compares strings with the collate facet.
TEST(Localization, OperatorCallUsesCollate) {
    const std::locale& loc = std::locale::classic();
    std::string a = "apple", b = "banana";
    const auto& coll = std::use_facet<std::collate<char>>(loc);
    EXPECT_TRUE(loc(a, b));
    EXPECT_FALSE(loc(b, a));
    EXPECT_EQ(loc(a, b), coll.compare(a.data(), a.data() + a.size(), b.data(),
                                      b.data() + b.size()) < 0);
}

// "Querying a facet": numpunct gives thousands_sep and decimal_point.
TEST(Localization, QueryNumpunct) {
    const auto& np = std::use_facet<std::numpunct<char>>(std::locale::classic());
    EXPECT_EQ(np.decimal_point(), '.');
    EXPECT_EQ(np.thousands_sep(), ',');
    EXPECT_EQ(np.grouping(), "");  // why classic doesn't group
}

// "Gotchas": a named locale the OS doesn't have throws std::runtime_error.
TEST(Localization, UnknownNamedLocaleThrows) {
    EXPECT_THROW(std::locale{"xx_NOT_A_LOCALE.UTF-8"}, std::runtime_error);
}

// "Gotchas": libstdc++ on macOS supports only "C"/"POSIX", so std::locale{""} throws there.
TEST(Localization, EnvironmentLocale) {
    std::string saved = std::setlocale(LC_ALL, nullptr);
    const char* env = std::setlocale(LC_ALL, "");
    std::string env_name = env ? env : "";
    std::setlocale(LC_ALL, saved.c_str());
    if (env_name.empty() || env_name == "C" || env_name == "POSIX")
        GTEST_SKIP() << "environment locale is \"" << env_name
                     << "\", which every library supports";
#if defined(__GLIBCXX__) && (defined(__APPLE__) || defined(__MINGW32__))
    // libstdc++'s generic locale model (macOS, MinGW) supports only "C" and "POSIX".
    EXPECT_THROW(std::locale{""}, std::runtime_error);
#else
    EXPECT_NO_THROW(std::locale{""});
#endif
}

// "Gotchas": std::format is locale-independent by default, even with a grouping global locale.
TEST(Localization, FormatIgnoresGlobalLocale) {
    auto loc = try_locale({"de_DE.UTF-8", "de_DE.utf8", "en_US.UTF-8", "en_US.utf8"});
    if (!loc) GTEST_SKIP() << "no de_DE/en_US locale available to this C++ library";
    std::locale old = std::locale::global(*loc);
    std::string plain = std::format("{:.2f}", 1234567.89);
    std::string localized = std::format("{:.2Lf}", 1234567.89);
    std::locale::global(old);
    EXPECT_EQ(plain, "1234567.89");
    EXPECT_NE(localized, plain);
}

// "Gotchas": locales are immutable; build a modified one by combining facets.
TEST(Localization, CombineFacets) {
    struct Apostrophes : std::numpunct<char> {
        char do_thousands_sep() const override { return '\''; }
        std::string do_grouping() const override { return "\3"; }
    };
    std::locale base = std::locale::classic();
    std::locale custom{base, new Apostrophes};
    EXPECT_EQ(fixed2(custom, 1234567.89), "1'234'567.89");
    EXPECT_EQ(fixed2(base, 1234567.89), "1234567.89");  // the original is unchanged

    std::locale copy = custom;  // copies share the same facets
    EXPECT_TRUE(copy == custom);
    EXPECT_EQ(&std::use_facet<std::numpunct<char>>(copy),
              &std::use_facet<std::numpunct<char>>(custom));
}
