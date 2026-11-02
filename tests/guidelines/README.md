# Core Guidelines tests

Unit tests for the examples in
[`docs/guidelines/core-guidelines/examples/`](../../docs/guidelines/core-guidelines/examples/README.md)
(#235): one file per examples page, `<page>_test.cpp`, built as the
`guidelines` area (`examples_guidelines_tests`, `ctest -L '^guidelines$'`).

- The file's first line is
  `// Checks claims made by docs/guidelines/core-guidelines/examples/<page>.md.`
- Above each `TEST`, one comment line starting with the rule ID:
  `// Con.4: a const object can't be assigned; its value is fixed at construction.`
  `tools/gen_core_guidelines.py` reads it to link the test from the index.
- Suite names: `CoreGuidelines<Section>` (e.g. `CoreGuidelinesCon`), test
  names after the rule (`Con4_ConstObjectsAreFixed`), unique within the area.
- The suite's rules apply (CLAUDE.md, "Writing tests"): it builds on MSVC,
  clang-cl and MinGW too, and missing features skip with a reason
  (`CPPDOCS_REQUIRE`), never vanish.
- Test what the rule claims, not just that code runs: `static_assert` for
  type properties (`std::is_const_v`, `std::is_invocable_v` on a `const`
  object), `EXPECT_*` on values and on side effects (destructor counts,
  call counts).
