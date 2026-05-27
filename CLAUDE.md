# Working in cpp-docs

This repo is a learning reference for **all of modern C++** — the language and
the standard library — written as example-driven Markdown. There is **no build
system and no compiler step**; the deliverable is documentation, not a program.

We mirror how cppreference organizes the reference and fill it in topic by topic.
**Containers are the current starting point, not the whole scope.** Expect to
grow into strings, algorithms, ranges, utilities, numerics, concurrency, and the
core language over time.

## Source of truth

- The authoritative reference is **cppreference.com**. Follow its facts:
  signatures, complexity, since-version, and section organization.
- **Only cite cppreference.com** in "See also" sections for now. No blogs,
  Stack Overflow, isocpp, etc.
- cppreference blocks automated fetches (HTTP 403 from WebFetch). To verify
  facts, use the `perplexity` MCP tools with
  `search_domain_filter: ["cppreference.com"]`.

## Layout

Each cppreference area is a subfolder under `docs/examples/`:

```
docs/examples/
  containers/{sequence,associative,unordered,adaptors,views}/
  strings/  algorithms/  ranges/  utilities/  language/  ...   (added as we go)
  README.md   # master index + progress table
```

Keep `docs/examples/README.md` updated whenever a page is added or finished.

## House style for a page

Keep every page consistent. Skeleton:

```markdown
# std::NAME   (or: Topic name)

> One-sentence description. **Header:** `<header>` · **Since:** C++NN

## Mental model
A sentence or two + an ASCII sketch if it helps.

## Declaration
The signature with its parameters/options explained.

## Complexity        (for containers/algorithms)
A table of the operations that matter, Big-O.

## Iterator & reference invalidation     (for containers)
What invalidates iterators, pointers, references. The #1 footgun.

## When to use it
Fits and anti-fits; how it compares to its siblings.

## Examples
Short, focused snippets. Prefer modern C++ (CTAD, structured bindings, ranges,
`auto`). Note the C++ version when a feature isn't universal.

## Gotchas
The non-obvious traps.

## See also
Links to the relevant cppreference pages (cppreference.com only).
```

## Conventions

- Target the **latest C++ standard**; flag version requirements inline
  (e.g. "`contains()` — since C++20").
- Code blocks use ```cpp.
- Use `#include` + `std::`-qualified names; avoid `using namespace std`.
- One concept per snippet; keep examples minimal and illustrative.

## Git / commits

- **Do not add Claude as a commit author or co-author.** Commit messages must not
  include any `Co-Authored-By: Claude` (or similar) trailer. Commits are authored
  solely by the repository owner.
