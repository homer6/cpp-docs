// Checks claims made by docs/security/supply-chain.md, about this repo's own
// CI inputs: each action and image is pinned to an immutable identifier.
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <string>

#include <gtest/gtest.h>

namespace {

std::filesystem::path repo() { return std::filesystem::path{CPPDOCS_FUZZ_DIR}.parent_path(); }

std::string read(const std::filesystem::path& p) {
    std::ifstream in{p, std::ios::binary};
    EXPECT_TRUE(in) << "can't read " << p;
    return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

}  // namespace

// "This repo's practice": every action a workflow uses is pinned to a full commit
// SHA, and every container image to a digest.
TEST(SecuritySupplyChain, WorkflowInputsPinned) {
    const auto dir = repo() / ".github" / "workflows";
    if (!std::filesystem::exists(dir)) GTEST_SKIP() << "no workflows in this checkout";
    const std::regex uses{R"(uses:\s*([^\s#]+))"};
    const std::regex image{R"(image:\s*"?([a-z0-9./_-]+:[^\s"@]+)(@sha256:[0-9a-f]{64})?)"};
    int checked = 0;
    for (const auto& entry : std::filesystem::directory_iterator{dir}) {
        if (entry.path().extension() != ".yml") continue;
        const std::string text = read(entry.path());
        for (std::sregex_iterator it{text.begin(), text.end(), uses}, end; it != end; ++it) {
            const std::string ref = (*it)[1].str();
            if (ref.starts_with("./")) continue;   // a local action or workflow
            EXPECT_TRUE(std::regex_search(ref, std::regex{"@[0-9a-f]{40}$"})) << entry.path().filename() << ": " << ref;
            ++checked;
        }
        for (std::sregex_iterator it{text.begin(), text.end(), image}, end; it != end; ++it) {
            EXPECT_TRUE((*it)[2].matched) << entry.path().filename() << ": image " << (*it)[1].str();
            ++checked;
        }
    }
    EXPECT_GT(checked, 0);
}
