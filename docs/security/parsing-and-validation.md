# Parsing and validation

> Parse untrusted input into constrained values with a defined grammar, reject anything ambiguous or partially consumed, bound the parser's work, and never let input become code: a format string, a query or a command.

## Mental model

The parser is the most exposed code you have: it runs on bytes the attacker
wrote, before anything has been checked. Keep its job narrow and its output
typed:

```
bytes ─▶ size limits ─▶ grammar ─▶ range & structure checks ─▶ typed values ─▶ authorize ─▶ act
        (transport)    (syntax)     (numbers, lengths,         (only these     (threat-modeling.md)
                                     relationships)              reach the core)
```

Each stage rejects; none "repairs". A request that only parses with a
guessed interpretation is an ambiguity an attacker can use against a
different component that guesses differently.

## Parse into constrained values

**MUST:** Check syntax, representability, meaning and authorization as
separate steps, and produce values whose types say they've been checked (see
[threat modeling](threat-modeling.md#keep-the-trusted-core-small)). Consider
ill-formed input on every read (**SL.io.2**).

**MUST:** When converting text to a number, check the error *and* that the
whole field was consumed (CERT
[ERR62-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/exceptions-and-error-handling-err/err62-cpp)).
`std::from_chars` (C++17) is the right tool: no locale, no exceptions, no
whitespace skipping, and it reports both:

```cpp
#include <charconv>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>

std::optional<std::uint16_t> parse_port(std::string_view text) {
    if (text.size() > 1 && text.front() == '0') return std::nullopt;   // "080" and "80" mustn't both mean 80
    unsigned value = 0;
    const char* const last = std::to_address(text.end());
    auto [end, ec] = std::from_chars(std::to_address(text.begin()), last, value);
    if (ec != std::errc{}) return std::nullopt;               // not a number, or too big for unsigned
    if (end != last) return std::nullopt;                     // "8080abc": trailing data
    if (value == 0 || value > 65535) return std::nullopt;      // the domain, not the type
    return static_cast<std::uint16_t>(value);
}
```

`from_chars` rejects a leading `+`, leading whitespace and (for unsigned
types) a `-`, so `" 80"`, `"+80"` and `"-1"` all fail; `std::stoi` and
`atoi` would accept or misreport some of them. It does accept leading zeros,
so the first line rejects them: otherwise `"080"` and `"80"` are two spellings
of one value, exactly the kind of ambiguity the next section warns about. This
repo's fuzz target for `parse_port` found that in its first 30 seconds (see
[verification](verification.md#fuzz-the-security-boundaries)). See
[charconv](../examples/text-processing/charconv.md).

## Define the ambiguous cases

**MUST:** Decide, document and test what the parser does with: trailing data,
duplicate fields, unknown fields and enum values, unsupported versions,
malformed or overlong encodings, embedded NUL characters, and length fields
that disagree with the data. Reject what two components could read
differently (one takes the first duplicate, another the last).

```cpp
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Headers as name/value pairs; a duplicate name is an error, not "last one wins".
std::optional<std::map<std::string, std::string>>
collect(const std::vector<std::pair<std::string_view, std::string_view>>& fields) {
    std::map<std::string, std::string> out;
    for (auto [name, value] : fields)
        if (!out.emplace(std::string{name}, std::string{value}).second) return std::nullopt;
    return out;
}
```

## Never let input become code

**MUST:** Never pass untrusted text as a format string, a query's structure,
a shell command or any other interpreted language; pass it as *data* to a
fixed template (CERT
[FIO30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/input-output-fio/fio30-c)).

`std::format` and `std::print` check the format string at compile time, so
the classic `printf(user_text)` mistake doesn't compile with them:

```cpp compile-fail="consteval|constant expression"
#include <format>
#include <string>

std::string greet(const std::string& name_from_request) {
    return std::format(name_from_request);   // error: the format string must be a constant
}
```

```cpp
#include <format>
#include <string>

std::string greet(const std::string& name_from_request) {
    return std::format("hello, {}", name_from_request);   // input is an argument, never the format
}
```

`std::runtime_format` (C++26) and `std::vformat` exist for format strings
that aren't constants; they throw `std::format_error` on a bad specification.
Use them only for format strings your program controls (its own
configuration or translations), never for request data. The same rule applies
to SQL (bound parameters, not concatenation) and to processes (an argument
vector, not a shell string; see
[filesystem and processes](filesystem-and-processes.md)).

## Bound the parser's work

**MUST:** Limit nesting depth, element counts, decoded and decompressed size,
and total allocation and work per input, not only the size of the raw bytes.
A recursive parser without a depth limit is a stack overflow an attacker can
trigger with a few kilobytes of `[[[[`:

```cpp
#include <cstddef>
#include <optional>
#include <string_view>

// Returns the maximum nesting depth of balanced parentheses, or nullopt if the
// input is malformed or nests deeper than max_depth.
std::optional<std::size_t> nesting(std::string_view s, std::size_t max_depth) {
    std::size_t depth = 0, deepest = 0;
    for (char c : s) {
        if (c == '(') {
            if (++depth > max_depth) return std::nullopt;   // bounded
            if (depth > deepest) deepest = depth;
        } else if (c == ')') {
            if (depth == 0) return std::nullopt;
            --depth;
        } else {
            return std::nullopt;
        }
    }
    if (depth != 0) return std::nullopt;
    return deepest;
}
```

**SHOULD:** Be careful with `std::regex` on untrusted input. Patterns with
nested quantifiers can take exponential time (ReDoS), and libstdc++ before GCC
16 matched recursively, so long input could overflow the stack (GCC bug
[86164](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86164); GCC 16 made the
matcher iterative). Bound the input length, prefer a hand-written scanner or
a linear-time engine for anything exposed, and fuzz it
([verification](verification.md)). See [regex](../examples/text-processing/regex.md).

**SHOULD:** Prefer mature, maintained parsers for complex formats (JSON, XML,
protocols), and track their versions and advisories
([supply chain](supply-chain.md)).

## See also

- Core Guidelines: [SL.io.2](../guidelines/core-guidelines/stdlib.md),
  [SL.str.1, SL.str.2](../guidelines/core-guidelines/stdlib.md),
  [I.5](../guidelines/core-guidelines/interfaces.md)
- [charconv](../examples/text-processing/charconv.md) ·
  [formatting](../examples/text-processing/formatting.md) ·
  [print](../examples/io/print.md) · [regex](../examples/text-processing/regex.md)
- [Integers](integers.md) (lengths and counts) ·
  [resource limits](resource-limits.md) · [verification](verification.md) (fuzzing parsers)
- Tests: [`tests/security/parsing-and-validation_test.cpp`](../../tests/security/parsing-and-validation_test.cpp)
