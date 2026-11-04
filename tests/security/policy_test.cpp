// Checks claims made by docs/security/policy.md: the clang-tidy checks its rule
// table names are enabled, and findings are errors, in this repo's profile.
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <set>
#include <string>

#include <gtest/gtest.h>

namespace {

std::filesystem::path repo() { return std::filesystem::path{CPPDOCS_FUZZ_DIR}.parent_path(); }

std::string read(const std::filesystem::path& p) {
    std::ifstream in{p, std::ios::binary};
    EXPECT_TRUE(in) << "can't read " << p;
    return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

std::set<std::string> matches(const std::string& text, const std::regex& re) {
    std::set<std::string> out;
    for (std::sregex_iterator it{text.begin(), text.end(), re}, end; it != end; ++it) out.insert((*it)[1].str());
    return out;
}

}  // namespace

// "Rules and the checks that enforce them": the profile enables the three
// families, turns every finding into an error, and disables only the two
// checks it documents.
TEST(SecurityPolicy, ProfileEnablesFamiliesAsErrors) {
    const std::string cfg = read(repo() / ".clang-tidy");
    for (const char* family : {"cppcoreguidelines-*", "cert-*", "bugprone-*"}) {
        EXPECT_NE(cfg.find(std::string{"\n  "} + family), std::string::npos) << family;
    }
    EXPECT_NE(cfg.find("WarningsAsErrors: '*'"), std::string::npos);
    const auto disabled = matches(cfg, std::regex{R"(\n\s+-([a-z][a-z0-9-]+),?)"});
    const std::set<std::string> documented{"cppcoreguidelines-pro-bounds-avoid-unchecked-container-access",
                                           "cppcoreguidelines-avoid-magic-numbers"};
    EXPECT_EQ(disabled, documented);
}

// "Rules and the checks that enforce them": no check the page relies on is
// disabled in the root profile.
TEST(SecurityPolicy, TableChecksAreEnabled) {
    const std::string page = read(repo() / "docs" / "security" / "policy.md");
    const auto named = matches(page, std::regex{R"(`((?:cppcoreguidelines|cert|bugprone)-[a-z0-9-]+)`)"});
    EXPECT_GE(named.size(), 20u);
    const std::string cfg = read(repo() / ".clang-tidy");
    for (const auto& check : named) {
        EXPECT_EQ(cfg.find("-" + check + "\n"), std::string::npos) << check << " is disabled";
        EXPECT_EQ(cfg.find("-" + check + ","), std::string::npos) << check << " is disabled";
    }
}
