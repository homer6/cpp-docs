---
name: release-tweet
description: Compose the X (Twitter) post that announces a cpp-docs release, in the owner's shape (examples/v0.3.0.md) - a headline naming the version and what the project is, a dash list of what it offers and what's new, "PRs welcome!", the repo link and the hashtags - with every fact and number taken from the release notes and the platform support notes. Drafts only; the owner posts. Use when the owner says "tweet the release", "create a tweet for this release", "announce vX.Y.Z", "write the release post" or after a release is published.
---

# Compose the release post

The owner posts each release on X. This skill drafts that post (#395). It
**never posts**: the draft goes in chat, and the owner posts it.

## 1. Read the sources

- **The release:** `docs/releases/vX.Y.Z.md` at the tag (`git show
  vX.Y.Z:docs/releases/vX.Y.Z.md`). What's new, where it ran, the counts.
- **The platform support notes:** `docs/platform-support/notes.md` at the
  tag. The "In one paragraph" section has the headline findings (which
  compiler leads, what a toolchain lacks).
- **The previous posts:** every file in `examples/`, newest first. Match
  their shape and voice; the newest one wins where they differ.

## 2. The shape

Follow [`examples/v0.3.0.md`](examples/v0.3.0.md):

```text
cpp-docs vX.Y.Z is out: <what the project is, and this release's theme>.

- <the release's main addition>

- <the next one>

- <the standing pillars, as the release changed them: the reference, the skills, testing>

- <one platform finding from the notes>

PRs welcome!

https://github.com/homer6/cpp-docs

#cpp #cplusplus #cpp26 #moderncpp
```

- **Headline:** `cpp-docs vX.Y.Z is out:` then one plain line a C++
  developer would want, such as "learn modern C++ (C++11 to C++26) by
  example". A themed release adds its theme ("write secure C++, by
  example").
- **Bullets:** `- ` at the start, one blank line between them, 3 to 5 of
  them. Lead with what a reader gets (pages, skills, tools), not how we
  built it. One thought per bullet; no sub-bullets.
- **Numbers:** keep one or two that carry weight (toolchains tested, pages,
  0 failures). Every number comes from the release notes or the platform
  notes at the tag, never from memory or an earlier release.
- **Names:** the skills by name (`modern-cpp`, `cpp-guidelines`,
  `cpp-security`) and as "Claude skills", as the owner wrote. Platforms and
  compilers spelled as in the notes (GCC, Clang, Apple Clang, MSVC; Linux,
  macOS, Windows, FreeBSD).
- **Close:** `PRs welcome!`, the repo link (the repo, not the release page),
  then the hashtags. Keep the four above; add at most one for the theme
  (`#security` for a security release).
- **Plain text:** X shows no Markdown, so no `**bold**` or backticks.

## 3. The rules

- **True and measured.** Each claim is in the release or platform notes.
  A finding the results can't show (a CMake limit, not a compiler gap)
  isn't claimed for the compiler.
- **Positive security framing.** Secure C++ is C++ written to the Core
  Guidelines, with hardening, verification and a small trusted core. Never
  say or imply that C++ isn't memory-safe (see `CLAUDE.md`, "Security
  section").
- **No AI attribution** about how the post or the release was made. Naming
  the Claude skills as a feature is fine; "written by AI" is not.
- **Length:** `python3 .claude/skills/release-tweet/scripts/x_length.py
  post.txt` counts it as X does (a link is 23). The owner's posts run long
  (v0.3.0 counts 568), which needs a long post; offer a version of 280 or
  less as well, cut down to the headline, two bullets and the link.

## 4. Hand it over

Give the owner, in chat:

1. the post, in a `text` block, ready to paste;
2. its length from `x_length.py`, and the short version if it's over 280;
3. a short table of each claim and where it comes from (file and section).

Then stop: the owner posts it. When they share what they posted, save it
verbatim as `examples/vX.Y.Z.md` (with an issue and a PR, like any change)
so the next post follows it.
