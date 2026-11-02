# Core Guidelines examples

Our own examples for the rules of the
[C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines),
one page per official section, named like the section's page in the
[index of every rule](../README.md) (`class.md` for C, `expr.md` for ES, …).
The index links each rule's example and test, and counts what's still to do
(#235). Examples for any rule are welcome.

## How a page is written

- **Title:** `# <section title> — examples`, then a one-line `>` summary that
  links the section's index page.
- **One `## <ID>: <official title>` heading per rule**, in the official order,
  with the title copied exactly from the index (the generator reads the ID).
- **Under it:** a sentence or two in our own words on what the rule asks and
  why, then **our own** code. Never copy the Guidelines' text or examples: their
  licence permits copies for personal or internal business use only.
- **BAD, then GOOD**, each a ```` ```cpp ```` block that the snippet checker
  compiles (`tools/check_snippets.py`, the fence tags in CLAUDE.md):
  - the BAD block is `compile-fail="regex"` when the compiler rejects the
    mistake (the repo's question: can the compiler reject it?), with a regex
    that matches both GCC's and Clang's diagnostic;
  - otherwise `nocompile`, with a comment saying why it's wrong (UB, a leak,
    a race), since it compiles;
  - the GOOD block compiles (`cpp`, or `cpp body` for statements), with
    `needs=<feature>` for what not every compiler has.
- **A rule that isn't about code** (philosophy, naming advice, FAQ entries)
  gets one line instead of code: `*No example: <reason>.*`. The index shows
  the reason.
- **Tested:** when the rule's claim can be checked at run time or with a
  `static_assert`, a test in `tests/guidelines/<page>_test.cpp` checks it, and
  the rule's section ends with a "Tested by" line linking that file.
- Rules whose ID or title changes upstream are renamed here when the
  submodule moves; `gen_core_guidelines.py --check` fails on an ID the
  Guidelines don't have.

## Template

````markdown
## Con.4: Use `const` to define objects with values that do not change after construction

One or two sentences, in our words.

```cpp compile-fail="assignment of read-only|cannot assign"
// BAD: ...
```

```cpp
// GOOD: ...
```
````

## Pages

- [Con: Constants and immutability](const.md)
