// Checks claims made by docs/examples/language/basic-concepts/comments.md.
#include <string_view>

#include <gtest/gtest.h>

#define SIZE 4   // the comment is not part of the replacement list
#define CAT(a, b) a/**/b
#define COMMENTS_STR(...) #__VA_ARGS__
#define COMMENTS_XSTR(...) COMMENTS_STR(__VA_ARGS__)

// "Both forms, and what is not a comment": comment markers inside a string literal are text.
TEST(Comments, BothFormsAndStringLiterals) {
    int width = 80;          // line comment: runs to the end of the line
    int height = /* inline */ 24;
    const char* s = "/* not a comment */ // nor this";   // inside a string literal
    EXPECT_EQ(width, 80);
    EXPECT_EQ(height, 24);
    EXPECT_EQ(std::string_view(s), "/* not a comment */ // nor this");
    // "Declaration": /* and */ are ordinary inside //, and // is ordinary inside /* */.
    int x = 1;  // /* not opened
    int y = /* // not a line comment */ 2;
    EXPECT_EQ(x + y, 3);
    // "Gotchas": a raw string literal contains the comment characters verbatim.
    EXPECT_EQ(std::string_view(R"(/* x */)"), "/* x */");
}

// "Comments in macros": SIZE is just 4; a comment in a replacement list splits tokens.
TEST(Comments, CommentsInMacros) {
    int buffer[SIZE];
    static_assert(sizeof buffer == 4 * sizeof(int));
    EXPECT_STREQ(COMMENTS_XSTR(SIZE), "4");
    EXPECT_STREQ(COMMENTS_XSTR(CAT(x, y)), "x y");  // two tokens, not xy
}

// "A comment is replaced by one space": q == 5, r == 9.
TEST(Comments, CommentBecomesOneSpace) {
    int a = 10, b = 2, *p = &b;
    int q = a/ *p;      // `/ *` with a space is division and dereference: 10 / 2
    int r = a/**/-1;    // the comment becomes a space: `a - 1`
    EXPECT_EQ(q, 5);
    EXPECT_EQ(r, 9);
    // "Gotchas": `a //* comment */ b` is a line comment, so the expression is just `a`.
    int z = a //* comment */ b
        ;
    EXPECT_EQ(z, 10);
}

// "Disabling code" / "Gotchas": #if 0 hides code containing */; a // comment ending in \ eats the next line.
TEST(Comments, IfZeroAndBackslashSplice) {
#if 0
    int old_version();   /* this comment would end an enclosing block comment */
    FAIL() << "inside #if 0";
#endif
    int r = 1;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomment"
    // C:\temp\
    r = 0;            // this line is part of the comment above
#pragma GCC diagnostic pop
    EXPECT_EQ(r, 1);  // r is still 1
}

#undef SIZE
#undef CAT
