# Deep research prompt template

Copy this file to `prompts/issue-<n>-<slug>.md` (or `general-<slug>.md`) and
fill it in. Everything above the `---` line is instructions for us; everything
below it is what the user pastes into Perplexity Deep Research.
`scripts/collect_prompts.py` replaces `{{DATE}}` with today's date and extracts
the part below the line.

Writing a good prompt:

- **"Already covered"** = what the pages *and your draft* already say, as
  specific claims (flags, versions, rule IDs). The report then spends its
  effort on corrections, gaps and disagreements instead of basics.
- **Questions** are numbered, concrete, and answerable from primary sources.
  Name the exact things to settle ("is P3552 in C++26?", "minimum CMake version
  for X").
- **"Still open"** (on reruns) lists what the last report couldn't verify.
- **Output headings match the target page's headings**, so the import is a
  straight mapping.
- Ask for primary sources, a confidence level and source date per fact, both
  sides of disagreements, and "say so" instead of guessing.

Page style the report should map onto (for reference when choosing headings):

```markdown
# Page title

> One-sentence summary. **Source:** <name, edition> · <targets/scope>

## Topic
**ID: Rule as a short imperative.** One-line reason.

// BAD — why … // GOOD — why …   (in a cpp block)

## See also
- <source link> · [matching reference page](../examples/...)
```

---

I maintain C++ coding guidelines for experienced engineers, targeting C++26
with C++20/23 as the practical baseline. I'm writing/updating a page on
**<TOPIC>** and need a deep, well-sourced research report that verifies and
extends my draft. Today's date is {{DATE}}.

## What I already cover (verify, correct, or add nuance — don't re-explain)

- <specific claim, with the flag/version/rule ID>
- <…>

## Still open from the last run (prioritize these)

- <only on reruns; otherwise delete this section>

## Questions to answer

1. **<Area>** — <precise question>. Include <versions / platforms / dates>.
2. **<Area>** — <…>
3. **Anything I'm missing** — important practices or recent changes not
   listed above.

## Output format

- Organize under these headings: **<Heading 1> · <Heading 2> · … · Sources**.
- Each recommendation: **short imperative rule** → one-line reason → exact
  flag/API/command → tool or standard version required → source(s).
- Mark corrections to my list explicitly ("**Correction:** …").
- Where sources disagree, show **both positions and their reasons**.
- Cite **primary sources** only: <the vendor docs, specs, papers, official
  repos for this topic>. No unsourced blog summaries or AI content farms.
- Give a **confidence level** and the **date of the source** for every version,
  default, or support claim. Flag conflicts.
- **Don't invent** flags, APIs, versions, paper numbers, or talk titles. If
  something can't be verified, say so.
- End with a **candidate sources table**: name, URL, best for, update
  frequency, licence.
