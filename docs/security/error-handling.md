# Error handling and fail-closed behaviour

> When a security decision can't be made (an error, a timeout, an exception, a missing rule), the answer is no; every failure leaves a defined state; and a broken internal invariant stops the component rather than carrying on.

## Mental model

Attackers aim at the error paths, because they're tested least. Three
failures matter most:

- **Fail open:** an exception, timeout or missing configuration turns into
  "allowed", because the code defaulted to success or caught and carried on.
- **Partial updates:** an operation fails halfway, leaving a credential half
  changed or a permission granted without its audit record.
- **Continuing after corruption:** a check that means "memory or an invariant
  is already broken" is handled like an ordinary bad request, and the process
  keeps running in a state nobody reasoned about.

The Core Guidelines' error-handling rules (**E.1**–**E.31**) give the
mechanics; this page is about which way a failure must fall.

## Deny when the decision can't be made

**MUST:** Make every security decision fail closed: if authentication,
authorization or a required piece of security configuration can't be
established (an error, a timeout, an exception, an unknown result), deny.
OWASP's authorization guidance says the same: deny by default, and exit
safely when an authorization check fails.[^owasp]

```cpp bad
#include <string>

bool policy_allows(const std::string& user, const std::string& action);   // may throw

bool authorize(const std::string& user, const std::string& action) {
    try {
        return policy_allows(user, action);
    } catch (...) {
        return true;              // BAD: an outage of the policy service grants access
    }
}
```

```cpp
#include <string>

bool policy_allows(const std::string& user, const std::string& action);   // may throw

bool authorize(const std::string& user, const std::string& action) noexcept {
    try {
        return policy_allows(user, action);
    } catch (...) {
        return false;             // can't decide: deny (and log it)
    }
}
```

**MUST:** Make the result hard to ignore. Mark functions that return a
security decision or an error `[[nodiscard]]`, so dropping the result is a
warning, and with `std::expected` (C++23) or `std::optional` the caller has
to look before using the value (see [expected](../examples/utilities/expected.md)):

```cpp bad warns="ignoring return value"
[[nodiscard]] bool verify_signature(const char* message, const char* signature);

void accept(const char* message, const char* signature) {
    verify_signature(message, signature);   // BAD: the result is thrown away
    // ... process message as if verified ...
}
```

Detect and handle the errors that standard and system functions report;
don't assume success (CERT
[ERR33-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/error-handling-err/err33-c)).

## Leave a defined state

**MUST:** Give operations that change security state the **strong exception
guarantee**: they either complete or leave everything as it was (CERT
[ERR56-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/exceptions-and-error-handling-err/err56-cpp)).
Build the new state on the side, then commit it with an operation that can't
fail:

```cpp
#include <string>
#include <utility>
#include <vector>

struct Credentials {
    std::string password_hash;
    std::vector<std::string> recovery_codes;
};

// Either both fields change or neither does.
void rotate(Credentials& current, std::string new_hash, std::vector<std::string> new_codes) {
    Credentials next{std::move(new_hash), std::move(new_codes)};   // may throw: nothing changed yet
    std::swap(current, next);                                      // can't throw: commit
}
```

**MUST:** Release resources on every path, exceptional ones included, with
RAII (**E.6**, **E.13**; CERT
[ERR57-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/exceptions-and-error-handling-err/err57-cpp)),
and never let a destructor, `swap` or deallocation fail (**E.16**). Catch by
reference, most derived first (**E.15**, **E.31**; CERT
[ERR61-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/exceptions-and-error-handling-err/err61-cpp)).

## Put exception boundaries where control leaves your code

**MUST:** Catch exceptions at every boundary they mustn't cross: a thread's
entry function, a callback registered with a C library or an OS, a plugin
interface, a `noexcept` function. An exception escaping any of these calls
`std::terminate` (CERT
[ERR51-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/exceptions-and-error-handling-err/err51-cpp),
[ERR55-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/exceptions-and-error-handling-err/err55-cpp),
[ERR58-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/exceptions-and-error-handling-err/err58-cpp);
**F.6**, **E.12**). That's a crash an attacker can trigger, so decide at each
boundary: translate to an error code, or deny and log.

**SHOULD:** Don't disable exceptions as a security measure. With
`-fno-exceptions`, a failed allocation or a library precondition terminates
the program instead of reporting an error; if you build that way, know and
test what each failure does (**E.25**–**E.27**).

## Stop on broken invariants

**MUST:** Separate *expected* rejection (a malformed request, a failed login)
from evidence of an internal fault (a hardening check firing, a corrupted
data structure, an invariant that "can't" fail). Reject the first and carry
on; for the second, stop: terminate the request's worker and let a supervisor
replace it, rather than catching and continuing in a state nobody reasoned
about. Library hardening and contract violations are built this way: they
terminate by default (see [hardening](hardening.md)). Ending the program on
purpose is different from an abrupt, unplanned exit (CERT
[ERR50-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/exceptions-and-error-handling-err/err50-cpp)):
log what you need first, and isolate the work so one worker's exit isn't the
whole service's ([deployment](deployment.md)).

## See also

- Core Guidelines: [E.1–E.31](../guidelines/core-guidelines/errors.md),
  [I.10](../guidelines/core-guidelines/interfaces.md),
  [F.6](../guidelines/core-guidelines/functions.md)
- [throw, try, catch](../examples/language/exceptions/throw-try-catch.md) ·
  [noexcept and exception safety](../examples/language/exceptions/noexcept-and-safety.md) ·
  [expected](../examples/utilities/expected.md)
- [Threat models](threat-modeling.md#authorize-every-request-separately-from-parsing) (deny by default)
- Tests: [`tests/security/error-handling_test.cpp`](../../tests/security/error-handling_test.cpp)

[^owasp]: OWASP, [Authorization Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Authorization_Cheat_Sheet.html): "Deny by Default", "Exit Safely when Authorization Checks Fail".
