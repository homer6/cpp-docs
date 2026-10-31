// Checks claims made by docs/examples/text-processing/regex.md.
#include <regex>
#include <string>
#include <vector>

#include <gtest/gtest.h>

// "Match vs. search": regex_match needs the whole string, regex_search finds it within.
TEST(Regex, MatchVsSearch) {
    std::regex date{R"(\d{4}-\d{2}-\d{2})"};
    EXPECT_TRUE(std::regex_match("2026-05-26", date));
    EXPECT_FALSE(std::regex_match("on 2026-05-26", date));
    EXPECT_TRUE(std::regex_search("on 2026-05-26", date));
}

// "Capture groups": m[0] is the whole match, m[1]/m[2] the groups, with position/length.
TEST(Regex, CaptureGroups) {
    std::string s = "key=value";
    std::smatch m;
    std::regex kv{R"((\w+)=(\w+))"};
    ASSERT_TRUE(std::regex_search(s, m, kv));
    EXPECT_EQ(m[0], "key=value");
    EXPECT_EQ(m[1], "key");
    EXPECT_EQ(m[2], "value");
    EXPECT_EQ(m.position(1), 0);
    EXPECT_EQ(m.length(1), 3);
    EXPECT_EQ(m.position(2), 4);
    EXPECT_EQ(m.length(2), 5);
}

// "Mental model": cmatch is the match_results type for const char*.
TEST(Regex, Cmatch) {
    std::cmatch m;
    ASSERT_TRUE(std::regex_search("x=1", m, std::regex{R"((\w)=(\d))"}));
    EXPECT_EQ(m[1], "x");
    EXPECT_EQ(m[2], "1");
}

// "Iterate every match": sregex_iterator yields a1, b2, c3.
TEST(Regex, IterateEveryMatch) {
    std::string text = "a1 b2 c3";
    std::regex token{R"([a-z]\d)"};
    std::vector<std::string> found;
    for (std::sregex_iterator it{text.begin(), text.end(), token}, end; it != end; ++it)
        found.push_back((*it)[0]);
    EXPECT_EQ(found, (std::vector<std::string>{"a1", "b2", "c3"}));
}

// "Replace (with backreferences)": "$2, $1" turns "John Smith" into "Smith, John".
TEST(Regex, ReplaceWithBackreferences) {
    std::string out = std::regex_replace("John Smith", std::regex{R"((\w+)\s+(\w+))"}, "$2, $1");
    EXPECT_EQ(out, "Smith, John");
}

// "Grammar & flags": icase matches regardless of case; ECMAScript is the default grammar.
TEST(Regex, GrammarAndFlags) {
    std::regex ci{"hello", std::regex::icase};
    EXPECT_TRUE(std::regex_match("HeLLo", ci));
    EXPECT_FALSE(std::regex_match("HeLLo", std::regex{"hello"}));

    std::regex dflt{R"(\d+)"};
    EXPECT_EQ(dflt.flags() & std::regex::ECMAScript, std::regex::ECMAScript);
    std::regex ecma{R"(\d+)", std::regex::ECMAScript};
    EXPECT_TRUE(std::regex_match("123", ecma));

    // The other grammars exist as syntax_option_type values.
    for (auto g : {std::regex::basic, std::regex::extended, std::regex::awk, std::regex::grep,
                   std::regex::egrep})
        EXPECT_NO_THROW(std::regex("a", g));
}

// "Gotchas": a raw string and a doubled-backslash literal spell the same pattern.
TEST(Regex, RawStringEqualsEscaped) {
    EXPECT_EQ(std::string{R"(\d)"}, std::string{"\\d"});
    EXPECT_TRUE(std::regex_match("7", std::regex{"\\d"}));
}
