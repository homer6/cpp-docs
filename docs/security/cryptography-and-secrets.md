# Cryptography and secrets

> Use a maintained cryptographic library and the operating system's random source, keep secrets out of code, logs and long-lived memory, compare them in constant time, and authenticate every TLS peer.

## Mental model

C++ gives you the building blocks, not the cryptography. The security comes
from four habits:

1. **Don't write the crypto.** Algorithms, protocols, password hashing and
   TLS come from a maintained library (libsodium, OpenSSL or BoringSSL,
   Botan, the platform's TLS stack), configured with safe defaults.
2. **Unpredictable means cryptographically random.** `<random>` engines are
   for simulations and tests, not for tokens, keys or nonces.
3. **Secrets have a lifetime.** Each copy in memory, a log, a crash dump or a
   repository is one more place to leak from.
4. **Encryption without authentication protects nothing.** A TLS connection
   to the wrong peer is an encrypted connection to the attacker.

## Use a maintained cryptographic library

**MUST:** Use a maintained library for encryption, signatures, MACs, key
exchange and password hashing; never a home-made algorithm or protocol.
Prefer high-level APIs with fewer choices to get wrong (libsodium's
`crypto_secretbox`, `crypto_pwhash`, `crypto_sign`), and document which
primitive, key handling and nonce rule each use relies on. Track the
library's version and advisories like any dependency
([supply chain](supply-chain.md)).

## Get randomness from the operating system

**MUST:** Generate tokens, keys, nonces, salts and session IDs from a
cryptographically secure source: the OS (`getrandom` on Linux,
`arc4random_buf` on macOS and the BSDs, `BCryptGenRandom` on Windows) or your
crypto library (`randombytes_buf` in libsodium). Never use `rand()` or a
`<random>` engine for them (CERT
[MSC50-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/miscellaneous-msc/msc50-cpp),
[MSC30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/miscellaneous-msc/msc30-c)):
an engine's whole future output follows from its seed and a few observed
outputs, and an engine seeded the same way repeats exactly:

```cpp run
#include <cassert>
#include <random>

int main() {
    std::mt19937 a{12345}, b{12345};
    for (int i = 0; i < 1000; ++i) assert(a() == b());   // same seed, same "random" tokens
}
```

**MUST NOT** treat `std::random_device` as a cryptographic source either.
The standard lets it fall back to a deterministic engine when no
non-deterministic source is available, and its `entropy()` isn't a reliable
indicator; older MinGW builds of libstdc++ returned the same sequence on every
run (GCC bug [85494](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85494), fixed
in GCC 9). It's fine for *seeding* a simulation engine (CERT
[MSC51-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/miscellaneous-msc/msc51-cpp));
for secrets, call the OS or the crypto library. See
[random](../examples/numerics/random.md).

## Keep secrets out of code, logs and long-lived memory

**MUST:** Never put credentials, keys or tokens in source code, build files,
committed configuration, logs or error messages (CERT
[MSC41-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/miscellaneous-msc/msc41-c)).
Load them at run time from a secret store or the environment the deployment
controls ([deployment](deployment.md)).

**SHOULD:** Keep secrets in few, short-lived buffers you own, and erase them
when done. An ordinary `memset` before the buffer dies is a dead store the
optimizer may remove, so use an erase the compiler must keep. C23 and C++26
add `memset_explicit`, but none of the libraries we test ships
`std::memset_explicit` yet (libstdc++ 15 and 16, libc++ 22), and glibc 2.41
declares its `memset_explicit` only for C, so use the platform's function
behind one wrapper:

```cpp
#include <cstddef>
#include <cstring>
#if defined(_WIN32)
#  include <windows.h>
#elif defined(__APPLE__)
// Apple's <string.h> declares memset_s (C11 Annex K) only if __STDC_WANT_LIB_EXT1__
// is 1 before its first inclusion; declaring it here avoids depending on include order.
extern "C" int memset_s(void* s, std::size_t smax, int c, std::size_t n);
#endif

// Erase a buffer in a way the optimizer can't remove.
void secure_zero(void* p, std::size_t n) noexcept {
#if defined(_WIN32)
    SecureZeroMemory(p, n);
#elif defined(__APPLE__)
    memset_s(p, n, 0, n);              // C11 Annex K, in Apple's libc
#elif defined(__GLIBC__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    explicit_bzero(p, n);
#else
    auto* v = static_cast<volatile unsigned char*>(p);   // fallback: volatile stores
    while (n--) *v++ = 0;
#endif
}
```

**MUST:** Don't claim that erasing one buffer removes the secret. Copies may
remain in `std::string` reallocations, temporary buffers, registers, the
kernel, swap or a core dump; libsodium's documentation says the same of its
own buffers.[^sodium] Reduce copies (pass `std::span<const std::byte>`, avoid
`std::string` for keys), lock pages with `mlock`/`VirtualLock` or libsodium's
`sodium_mlock` where the threat model needs it, and keep core dumps from
leaving the machine ([deployment](deployment.md)).

## Compare secrets in constant time

**SHOULD:** Compare MACs, tokens and password hashes with a constant-time
function from your crypto library (`sodium_memcmp`, OpenSSL's
`CRYPTO_memcmp`). `==` and `memcmp` stop at the first difference, so timing
reveals how much of a guess was right. A hand-written loop shows the idea,
but an optimizer is allowed to turn it back into an early exit, so prefer the
library's:

```cpp
#include <cstddef>
#include <span>

// Touches every byte whatever the data; prefer sodium_memcmp / CRYPTO_memcmp.
bool equal_secret(std::span<const unsigned char> a, std::span<const unsigned char> b) noexcept {
    if (a.size() != b.size()) return false;     // lengths are usually public
    unsigned char diff = 0;
    for (std::size_t i = 0; i < a.size(); ++i) diff |= a[i] ^ b[i];
    return diff == 0;
}
```

## Authenticate every TLS peer

**MUST:** Verify the peer's certificate chain and its name (or pin the key)
for every TLS connection, client and server side as your model requires.
Encryption to an unauthenticated peer is encryption to whoever answered.

**MUST:** Never ship "temporarily disable certificate verification". Test
against a test CA instead.

**SHOULD:** Default to TLS 1.3, allow TLS 1.2 only for compatibility, and
disable TLS 1.0, 1.1 and SSL entirely; OWASP's TLS guidance says applications
must default to TLS 1.3, may support 1.2, and must disable 1.0 and 1.1
(deprecated by RFC 8996).[^owasp-tls]

## See also

- [random](../examples/numerics/random.md) · [span](../examples/containers/views/span.md)
- [Object representation](object-representation.md) (secrets in padding) ·
  [deployment](deployment.md) (secret stores, logs, core dumps)
- [libsodium documentation](https://doc.libsodium.org/) ·
  cppreference [`std::random_device`](https://en.cppreference.com/w/cpp/numeric/random/random_device),
  [`memset_explicit`](https://en.cppreference.com/w/cpp/string/byte/memset)
- Tests: [`tests/security/cryptography-and-secrets_test.cpp`](../../tests/security/cryptography-and-secrets_test.cpp)

[^sodium]: libsodium, [Internals](https://doc.libsodium.org/internals) and [Secure memory](https://doc.libsodium.org/memory_management).
[^owasp-tls]: OWASP, [Transport Layer Security Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Transport_Layer_Security_Cheat_Sheet.html), "Only Support Strong Protocols".
