# Comments

> Text the compiler throws away: `// to end of line` and `/* block */`. They are
> removed in translation phase 3, before the preprocessor runs.
> **Since:** C++98 (both forms)

## Mental model

```
phase 1  decode the source file (UTF-8 is required to be supported)
phase 2  delete every backslash-newline  ->  physical lines spliced into logical lines
phase 3  split into preprocessing tokens; EACH COMMENT BECOMES ONE SPACE
phase 4  preprocessor: directives run, macros expand, #include / #embed
phase 5  adjacent string literals are concatenated
phase 6  preprocessing tokens become tokens (keywords are recognised here)
phase 7  parse, analyse, instantiate templates
phase 8  link
```

Two consequences fall out of that order:

- A comment is gone **before** any `#define` is processed, so a comment after a
  macro is never part of the macro.
- Line splicing (phase 2) happens **before** comments are recognised (phase 3),
  so a `//` comment that ends in a backslash swallows the next line.

## Declaration

| Form | Starts | Ends | Nests? |
|---|---|---|---|
| `// …` | `//` | just before the next new-line | — |
| `/* … */` | `/*` | the first `*/` | **no** |

Inside a `//` comment, `//`, `/*` and `*/` are ordinary characters. Inside a
`/* */` comment, `//` and `/*` are ordinary characters. Inside a string or
character literal, none of them start a comment.

## When to use it

- `//` for almost everything, including multi-line comments (one `//` per line).
  It can't be broken by a stray `*/`.
- `/* */` for comments inside a line, such as a parameter name you want to keep
  but not use: `void on_event(int /*code*/)`.
- To switch off a block of code, use `#if 0 … #endif`, not `/* */`: it nests,
  and the code inside it can contain comments.

## Examples

### Both forms, and what is *not* a comment

```cpp body
int width = 80;          // line comment: runs to the end of the line
int height = /* inline */ 24;
const char* s = "/* not a comment */ // nor this";   // inside a string literal
use(width, height, s);
```

### Comments in macros

The comment is removed in phase 3, so `SIZE` is just `4`:

```cpp
#define SIZE 4   // the comment is not part of the replacement list
int buffer[SIZE];
static_assert(sizeof buffer == 4 * sizeof(int));
```

Nor can a macro *produce* a comment: by phase 4 comments no longer exist, so
pasting `/` and `/` with `##` gives two `/` tokens (or an error), never a
comment.

### A comment is replaced by one space

```cpp body
int a = 10, b = 2, *p = &b;
int q = a/ *p;      // `/ *` with a space is division and dereference: 10 / 2
int r = a/**/-1;    // the comment becomes a space: `a - 1`
use(q, r);          // q == 5, r == 9
```

Because a comment is a space, it splits tokens: in `#define CAT(a, b) a/**/b`,
`CAT(x, y)` expands to the two tokens `x y`, not `xy` (some pre-standard
preprocessors pasted them; use `##` for that).

### Disabling code: `#if 0`, not `/* */`

```cpp
#if 0
int old_version();   /* this comment would end an enclosing block comment */
#endif
```

## Gotchas

- **Block comments don't nest.** The first `*/` ends the comment, and what
  follows is code:

  ```cpp compile-fail
  /* outer /* inner */ still in the comment? no */
  ```

- **A backslash at the end of a `//` comment continues it.** Line splicing runs
  first, so the next line becomes part of the comment and is silently not
  compiled. GCC and Clang warn (`-Wcomment`, in `-Wall`), and Clang 22 makes it
  an error under `-pedantic-errors`:

  ```cpp nocompile
  int r = 1;
  // C:\temp\
  r = 0;            // this line is part of the comment above
  use(r);           // r is still 1
  ```

- **`/*` right after `/` needs a space.** `x = a //* comment */ b;` is a `//`
  line comment in C++ (so the line is `x = a`), not `a / b`. C89 read it
  differently; C++ always has.
- **Unterminated `/*` is an error at end of file** ("a source file shall not
  end in a partial comment"), and the error points at the `/*`, not where you
  meant it to stop.
- **Comments are whitespace, not nothing.** That matters to the preprocessor
  (token splitting above) and to raw string literals: `R"(/* x */)"` contains
  the comment characters verbatim.

## See also

- [Comments — cppreference](https://en.cppreference.com/w/cpp/comment)
- [Phases of translation — cppreference](https://en.cppreference.com/w/cpp/language/translation_phases)
- [\[lex.comment\]](https://eel.is/c++draft/lex.comment) and
  [\[lex.phases\]](https://eel.is/c++draft/lex.phases) — C++ working draft
- [The preprocessor](../preprocessor/preprocessor.md)
