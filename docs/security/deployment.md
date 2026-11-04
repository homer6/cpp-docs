# Deployment and incident readiness

> Run with the least privilege the job needs, put the code that parses untrusted input in a process that can do little else, keep secrets out of logs and crash dumps, and have a way to receive and ship fixes.

## Mental model

The other pages reduce the chance of a bug; this one limits what a bug
is worth. If the parser is compromised, what can the attacker do with that
process's privileges, files, network and memory?

```
            ┌───────────── broker (privileged, small) ─────────────┐
 network ─▶ │ accept ─▶ hand bytes to worker ─▶ act on typed result │ ─▶ files, keys, DB
            └───────────────────────┬──────────────────────────────┘
                                    ▼ IPC: bytes in, typed result out
            ┌──────── worker (unprivileged, sandboxed) ────────┐
            │ parse, decode, decompress: no files, no network  │
            └──────────────────────────────────────────────────┘
```

This is the [threat model](threat-modeling.md)'s "small trusted core",
applied at the process level: the code that handles attacker bytes runs
where a compromise reaches nothing valuable.

## Run with least privilege

**MUST:** Start with only the privileges the program needs, and drop the rest
as soon as the work that needed them is done: bind the low port, open the
key file, then drop. Drop in the right order, supplementary groups, then the
group, then the user, because once the user ID is gone the process can no
longer change its groups. Then check that the drop can't be undone (CERT
[POS36-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/posix-pos/pos36-c),
[POS37-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/posix-pos/pos37-c)):

```cpp
#include <grp.h>
#include <stdexcept>
#include <sys/types.h>
#include <unistd.h>

// Permanently become uid:gid. Throws instead of carrying on with privileges.
void drop_privileges(uid_t uid, gid_t gid) {
    if (setgroups(0, nullptr) != 0) throw std::runtime_error("setgroups failed");
    if (setgid(gid) != 0) throw std::runtime_error("setgid failed");
    if (setuid(uid) != 0) throw std::runtime_error("setuid failed");
    if (uid != 0 && setuid(0) == 0) throw std::runtime_error("root could be regained");
}
```

Every call's result is checked. A failed `setuid` that's ignored leaves the
program running as root, believing it isn't.

**SHOULD:** Prefer not having the privilege at all: let the service manager
bind the socket and pass it in, use Linux capabilities (`CAP_NET_BIND_SERVICE`)
instead of root, run in a container as a non-root user with a read-only
filesystem. On Windows, run as a dedicated service account with a restricted
token.

## Isolate the code that parses untrusted input

**SHOULD:** Run high-risk parsing (file formats, media, compression,
protocol decoding) in a separate process that has no access to files,
credentials or the network, and give it bytes and take back a validated,
typed result. A memory-safety bug in the worker then can't reach the
broker's privileges. Browsers and mail clients are built this way.

Sandboxing is per operating system:

| Platform | Mechanism |
|---|---|
| Linux | `prctl(PR_SET_NO_NEW_PRIVS, 1)` then a seccomp filter on system calls; Landlock (5.13+) to restrict file access; namespaces |
| FreeBSD | Capsicum: `cap_enter()`, after which only descriptors already held (with their rights) work |
| OpenBSD | `pledge()` (allowed system call groups) and `unveil()` (visible paths) |
| macOS | App Sandbox entitlements |
| Windows | AppContainer or a restricted token, job objects |

On Linux, an unprivileged process can install a seccomp filter only after
setting `no_new_privs`, which also makes set-user-ID programs it runs gain
nothing. The strictest built-in mode, `SECCOMP_MODE_STRICT`, allows only
`read`, `write`, `_exit` and `sigreturn`. That's enough for a worker that
reads a request from one pipe and writes its answer to another.[^seccomp]

```cpp
#if defined(__linux__)
#include <sys/prctl.h>

// First step of any Linux sandbox: nothing this process runs can gain privileges.
bool forbid_new_privileges() {
    return prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0;
}
#endif
```

**MUST:** Treat the worker's output as untrusted too: validate the result in
the broker ([parsing and validation](parsing-and-validation.md)), and bound
the worker's memory, CPU time and output size
([resource limits](resource-limits.md)).

## Stop safely, restart cleanly

The pages tell code to stop on a broken invariant rather than continue
([error handling](error-handling.md#stop-on-broken-invariants)), and
hardening and contract checks stop the process the same way. That is only
safe if something restarts it and nothing is left half-done.

**SHOULD:** Run the program under a supervisor that restarts it: a service
manager (systemd `Restart=on-failure`, launchd, a Windows service's recovery
actions) or an orchestrator. Limit the restart rate, so input that crashes the
worker every time doesn't keep the machine busy restarting it.

**MUST:** Write state so a crash at any moment leaves either the old version
or the new one. Write to a temporary file, then rename it over the target:
on POSIX file systems `rename` replaces the target atomically. For
durability across a power loss, also `fsync` the file and its directory
(POSIX only; the standard library has no portable call).

```cpp
#include <filesystem>
#include <fstream>
#include <string_view>
#include <system_error>

// A crash leaves the old file or the new one, never half of each.
bool write_atomically(const std::filesystem::path& target, std::string_view data) {
    std::filesystem::path tmp = target;
    tmp += ".tmp";
    {
        std::ofstream out{tmp, std::ios::binary | std::ios::trunc};
        if (!out.write(data.data(), static_cast<std::streamsize>(data.size()))) return false;
        out.flush();
        if (!out) return false;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, target, ec);   // replaces target
    return !ec;
}
```

## Get secrets at run time

**MUST:** Keep secrets out of the binary, the repository and the command line,
and load them when the program starts. Use a secret store (a cloud secret
manager or KMS, HashiCorp Vault, systemd's `LoadCredential=`, the OS keychain
or DPAPI on Windows), or a file only the service account can read. This shapes
the C++:
- **Arguments:** never pass a secret in `argv`. On Linux any local user can
  read a process's arguments (`ps`, `/proc/<pid>/cmdline`).
- **Environment variables:** prefer files or a credentials API. Every child
  process inherits the environment, and the same user and root can read it
  through `/proc/<pid>/environ`.
- **Storage:** read each secret once, into the one type that holds it
  ([cryptography and secrets](cryptography-and-secrets.md#keep-secrets-out-of-code-logs-and-long-lived-memory)),
  and erase buffers you no longer need.
- **Rotation:** plan for it. The program reloads a rotated secret without a
  rebuild, which means no secret is a compile-time constant.

## Keep secrets out of crash dumps

A core dump or minidump holds the process's memory, so it holds every key,
password and token the process had. If it's written to disk, collected by a
crash reporter or attached to a bug report, the secrets go with it.

**MUST:** Decide where crash data goes and who can read it, and keep dumps
of processes that hold secrets from leaving the machine unreviewed. On
Linux:

- `prctl(PR_SET_DUMPABLE, 0)` stops core dumps of the process, and also stops
  other processes of the same user from attaching a debugger to it. One
  caveat: with `fs.suid_dumpable=2`, the kernel still dumps such a process,
  readable by root only.
- `madvise(addr, len, MADV_DONTDUMP)` (Linux 3.4+) leaves the pages holding
  secrets out of any dump.
- `setrlimit(RLIMIT_CORE, {0, 0})` stops core *files*. The kernel doesn't
  enforce it when `core_pattern` pipes dumps to a handler such as
  systemd-coredump, so don't rely on it alone.[^core]

```cpp
#if defined(__linux__)
#include <cstddef>
#include <sys/mman.h>
#include <sys/prctl.h>

bool keep_out_of_dumps(void* page_aligned, std::size_t len) {
    return prctl(PR_SET_DUMPABLE, 0, 0, 0, 0) == 0
        && madvise(page_aligned, len, MADV_DONTDUMP) == 0;
}
#endif
```

**SHOULD:** If you ship a crash reporter (Crashpad, Breakpad, Windows Error
Reporting), know what it collects: minidumps include thread stacks and can
include heap. Strip or exclude secret-holding memory, and treat uploaded
dumps as sensitive data with access control and retention. Keep secrets in
few places to begin with ([cryptography and secrets](cryptography-and-secrets.md#keep-secrets-out-of-code-logs-and-long-lived-memory)).

## Keep logs safe to keep and safe to read

**MUST:** Never log credentials, keys, session tokens or whole requests that
may carry them. Make the safe thing the default with a type that can't
print its contents:

```cpp
#include <format>
#include <ostream>
#include <string>
#include <utility>

class Secret {
public:
    explicit Secret(std::string value) : value_{std::move(value)} {}
    const std::string& reveal() const { return value_; }   // grep-able at every use
private:
    std::string value_;
};

template <>
struct std::formatter<Secret> : std::formatter<std::string_view> {
    template <class FormatContext>   // not just std::format_context: libc++ 18 rejects that
    auto format(const Secret&, FormatContext& ctx) const {
        return std::formatter<std::string_view>::format("[redacted]", ctx);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Secret&) { return os << "[redacted]"; }
```

The formatter's `format` is a template on the context. libc++ 18 checks
whether a type is formattable with its own internal context type, so a
`format` that takes only `std::format_context&` makes `std::format` reject
`Secret` at compile time ("the supplied type is not formattable"). libc++ 19
and later, libstdc++ and the MS STL accept either form (Clang 18–23 with
libc++, GCC 15 and MSVC in CI, [#384](https://github.com/homer6/cpp-docs/issues/384)).

**MUST:** Neutralize untrusted text before it goes into a log line. A user
name containing a newline can forge a whole log entry, and terminal escape
sequences can rewrite what an operator sees
([CWE-117](https://cwe.mitre.org/data/definitions/117.html)). Escape control
characters, or log structured fields (JSON) through an encoder that does:

```cpp
#include <format>
#include <string>
#include <string_view>

// Control characters become visible escapes, so one input is one log line.
std::string log_field(std::string_view untrusted) {
    std::string out;
    for (unsigned char c : untrusted) {
        if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c < 0x20 || c == 0x7f) out += std::format("\\x{:02x}", c);
        else out += static_cast<char>(c);
    }
    return out;
}
```

**SHOULD:** Protect the logs themselves: who can read them, how long they're
kept, and that the program can append but not rewrite them.

## Be ready to receive and ship fixes

**MUST:** Have a way for people to report vulnerabilities privately (a
`SECURITY.md` naming the contact; GitHub's private vulnerability reporting),
and a process behind it: acknowledge, reproduce, fix, ship and disclose. For
C++ code, "ship" means a new binary: know which versions are supported, how
users get a patched build, and how long that takes
([supply chain](supply-chain.md) keeps what you need to rebuild exactly).

**MUST:** Turn each reported vulnerability into a regression test and look
for the same pattern elsewhere ([verification](verification.md#test-the-intended-failure-behaviour)).

## See also

- [Threat modeling](threat-modeling.md) (the small trusted core) ·
  [filesystem and processes](filesystem-and-processes.md) (launching a child safely) ·
  [cryptography and secrets](cryptography-and-secrets.md) · [hardening](hardening.md)
- Core Guidelines: [P.8](../guidelines/core-guidelines/philosophy.md) (don't leak resources),
  [I.4](../guidelines/core-guidelines/interfaces.md) (precisely typed interfaces)
- Man pages: [seccomp(2)](https://man7.org/linux/man-pages/man2/seccomp.2.html) ·
  [landlock(7)](https://man7.org/linux/man-pages/man7/landlock.7.html) ·
  [core(5)](https://man7.org/linux/man-pages/man5/core.5.html) ·
  [PR_SET_DUMPABLE](https://man7.org/linux/man-pages/man2/PR_SET_DUMPABLE.2const.html) ·
  [madvise(2)](https://man7.org/linux/man-pages/man2/madvise.2.html) ·
  [capsicum(4)](https://man.freebsd.org/cgi/man.cgi?capsicum(4)) ·
  [pledge(2)](https://man.openbsd.org/pledge.2)
- [GitHub: privately reporting a security vulnerability](https://docs.github.com/en/code-security/security-advisories/guidance-on-reporting-and-writing-information-about-vulnerabilities/privately-reporting-a-security-vulnerability)
- Tests: [`tests/security/deployment_test.cpp`](../../tests/security/deployment_test.cpp)

[^seccomp]: [seccomp(2)](https://man7.org/linux/man-pages/man2/seccomp.2.html): the system calls `SECCOMP_MODE_STRICT` permits, and the `no_new_privs` or `CAP_SYS_ADMIN` requirement for `SECCOMP_SET_MODE_FILTER`; the kernel's [no_new_privs](https://docs.kernel.org/userspace-api/no_new_privs.html) documentation.
[^core]: [core(5)](https://man7.org/linux/man-pages/man5/core.5.html): "The RLIMIT_CORE limit is not enforced for core dumps that are piped to a program"; `suid_dumpable` modes; [PR_SET_DUMPABLE](https://man7.org/linux/man-pages/man2/PR_SET_DUMPABLE.2const.html); [madvise(2)](https://man7.org/linux/man-pages/man2/madvise.2.html) `MADV_DONTDUMP` (since Linux 3.4).
