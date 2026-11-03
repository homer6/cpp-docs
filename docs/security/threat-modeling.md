# Threat models and trust boundaries

> Decide what an attacker controls and what must stay protected, then keep the code that handles untrusted input small, typed and separate from the code that acts.

## Mental model

A **trust boundary** is where data or control crosses from something you
don't control to something you do: a socket, a file, an environment variable,
a plugin. Everything on the far side is attacker-controlled until your code
has checked it. The security of a component comes down to three questions:

```
  attacker-controlled            your process
  ───────────────────┐   ┌─────────────────────────────────────────────┐
  bytes, files, env, │──▶│ parse ──▶ validated values ──▶ authorize ──▶ act │
  args, IPC, plugins │   │ (untrusted)   (typed)        (per request)  (privileged) │
  ───────────────────┘   └─────────────────────────────────────────────┘
```

1. **What can an attacker control?** The entry points.
2. **What must stay protected?** The assets.
3. **What stands between them?** Parsing, validation, authorization and
   resource limits, each a separate step with its own failure policy.

## Write a short threat model per component

**SHOULD:** For each component that handles untrusted input, write down
(a page is enough, kept with the code):

- **Assets:** secrets and credentials, privileged operations, other users' or
  tenants' data, the integrity of what you store, availability.
- **Entry points:** network messages, files, IPC, command-line arguments,
  environment variables, configuration, plugins, callbacks, and anything a
  library hands you from those.
- **Trust transitions:** remote to local, tenant to tenant, unprivileged to
  privileged, parser to the code that acts on its result.
- **What an attacker can do:** send malformed or oversized input, replay it,
  race concurrent requests, exhaust resources, tamper with files in
  directories they can write, compromise a dependency.
- **Failure policy:** which errors reject one request, which close a
  connection, which restart an isolated worker.

Microsoft's threat-modelling practice has the same shape: define security
requirements, diagram the application, identify threats (its STRIDE
categories: spoofing, tampering, repudiation, information disclosure, denial
of service, elevation of privilege), mitigate them, and validate that the
mitigations work.[^ms-tm] CISA's *Secure by Design* principles treat this as
part of designing the product, not a review at the end.[^cisa]

## Keep the trusted core small

**SHOULD:** Separate the code that parses untrusted input from the code that
performs privileged actions, and give the privileged side a narrow interface:
operations on validated values, never raw input, paths or handles.

**MUST:** Make the validated form a **type**. A parser that returns a
`RecordId` (or rejects the input) lets the compiler check that nothing
unvalidated reaches the privileged code: an interface that takes a
`RecordId` can't be called with a string. This is the Core Guidelines'
**I.4** (precisely and strongly typed interfaces) and **P.5** (prefer
compile-time checking) used as a security boundary; **C.41** keeps the type
from existing in an unchecked state.

```cpp
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

// The only way to get a RecordId is through parse(), which checks it.
class RecordId {
public:
    static std::optional<RecordId> parse(std::string_view text) {
        if (text.empty() || text.size() > 32) return std::nullopt;
        for (char c : text)
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return std::nullopt;
        return RecordId{std::string{text}};
    }
    std::string_view value() const { return value_; }

private:
    explicit RecordId(std::string v) : value_{std::move(v)} {}
    std::string value_;
};

struct User { std::string name; bool may_write = false; };

// The privileged side: a narrow operation on validated, authorized values.
class RecordStore {
public:
    bool store(const User& user, const RecordId& id, std::span<const std::byte> data);
};
```

Passing the attacker's text straight to the privileged side doesn't compile:

```cpp compile-fail="invalid initialization of reference|cannot convert|no viable conversion|no matching"
#include <string>

class RecordId {
public:
    static RecordId parse(const std::string&);   // the only way in
private:
    explicit RecordId(std::string);
};
void store(const RecordId& id);

void handle(const std::string& from_network) {
    store(from_network);          // error: a string isn't a RecordId
}
```

A type can only say *what shape* a value has. It can't say whether this user
may use it, so authorization is a separate step.

## Authorize every request, separately from parsing

**MUST:** Check authorization on every request, against the specific object,
after parsing and independently of it. A request that parses is well formed,
not permitted.

**MUST:** Deny by default: if a permission can't be established (unknown user,
missing rule, an error or timeout in the check), the answer is no.

**SHOULD:** Give each component the least privilege it needs, and put the
authorization check where the action happens, not only at the edge.

These come from the OWASP *Authorization Cheat Sheet* (Enforce Least
Privileges, Deny by Default, Validate the Permissions on Every Request,
Verify that Authorization Checks are Performed in the Right Location, Exit
Safely when Authorization Checks Fail).[^owasp] Failing closed is covered in
[error handling](error-handling.md).

```cpp
#include <cstddef>
#include <span>
#include <string>

struct User { std::string name; bool may_write = false; };
class RecordId;   // as above: only parse() makes one

enum class Decision { allow, deny };

// Deny unless a rule allows it.
Decision authorize_write(const User& user, const RecordId&) {
    return user.may_write ? Decision::allow : Decision::deny;
}

bool store_record(const User& user, const RecordId& id, std::span<const std::byte> data) {
    if (authorize_write(user, id) != Decision::allow) return false;   // checked here, every time
    // ... write data under id ...
    (void)data;
    return true;
}
```

## Gotchas

- **A library's convenience isn't your policy.** A framework that lets any
  authenticated caller reach an object hasn't decided that they should;
  review the authorization logic of the tools you use (OWASP).[^owasp]
- **Identifiers are input too.** A record ID in a request is
  attacker-chosen; check that *this* user may use *this* ID, not only that it
  exists.
- **The parser is the most exposed code.** Keep it free of privileges, bound
  its work ([resource limits](resource-limits.md)), and fuzz it
  ([verification](verification.md)).

## See also

- Core Guidelines: [I.4](../guidelines/core-guidelines/interfaces.md) (strongly typed interfaces),
  [P.5, P.7](../guidelines/core-guidelines/philosophy.md) (compile-time checking, catch errors early),
  [C.41](../guidelines/core-guidelines/class.md) (fully initialized objects)
- [Parsing and validation](parsing-and-validation.md) ·
  [error handling](error-handling.md) · [deployment](deployment.md) (privilege separation)
- Tests: [`tests/security/threat-modeling_test.cpp`](../../tests/security/threat-modeling_test.cpp)

[^ms-tm]: Microsoft, [Threat modeling in the SDL](https://www.microsoft.com/en-us/securityengineering/sdl/threatmodeling) and [Getting started with the Threat Modeling Tool](https://learn.microsoft.com/en-us/azure/security/develop/threat-modeling-tool-getting-started) (STRIDE).
[^cisa]: CISA and partners, [*Secure by Design: Shifting the Balance of Cybersecurity Risk*](https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf) (2023).
[^owasp]: OWASP, [Authorization Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Authorization_Cheat_Sheet.html), "Recommendations".
