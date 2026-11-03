# Filesystem, process and FFI boundaries

> Operate on what you opened rather than on a path you checked earlier, keep user-named files inside the directory they belong to, launch programs with an argument vector instead of a shell, and give every C interface an explicit contract for lengths, ownership and lifetimes.

## Mental model

A path is a name, looked up again every time it's used, and anyone who can
write to a directory on that path can change what the name means between your
check and your use. A shell command line is a small program, and anyone who
controls part of it controls the program. A C interface has no types for
ownership or length, so the contract lives only in documentation. All three
are boundaries where the operating system or another language takes over from
C++'s type system.

## Don't check a path, then use it

**MUST:** Don't check a file by name and then act on the same name ("exists?
then open", "is a regular file? then write", "not a symlink? then follow").
Between the two calls the name can be swapped for a symlink to somewhere else
(CERT
[FIO45-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/input-output-fio/fio45-c),
[POS35-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/posix-pos/pos35-c)).
Open first, then check the opened handle (`fstat`), and let the open itself
enforce what you need: `O_CREAT | O_EXCL` to create a new file and fail if one
exists, `O_NOFOLLOW` to refuse a symlink.

`std::filesystem` doesn't change this. It works on paths, and the standard
says that if another thread, process or computer changes the file system
while its functions run (a *file system race*), the behaviour is undefined.
Use it for paths you control, not as a security check on paths an attacker
can influence. See [filesystem](../examples/io/filesystem.md).

## Keep user-named files inside their directory

**MUST:** Treat a file name from input as untrusted. Reject absolute paths,
root names (`C:`) and `..` components before joining it to your directory;
string-prefix checks and "canonicalize, then compare" aren't enough on their
own, because a symlink inside the tree can still point out of it.

```cpp
#include <filesystem>

// Lexical check: a relative path with no "..", no root name and no root directory.
bool stays_inside(const std::filesystem::path& untrusted) {
    if (untrusted.empty() || untrusted.is_absolute() ||
        untrusted.has_root_name() || untrusted.has_root_directory())
        return false;
    for (const auto& part : untrusted.lexically_normal())
        if (part == "..") return false;
    return true;
}
```

**SHOULD:** Then open the file relative to a directory handle you already
hold, with the OS refusing to leave it. On Linux, `openat2` with
`RESOLVE_BENEATH` (and `RESOLVE_NO_SYMLINKS` if you don't want symlinks at
all) makes the kernel reject any resolution that escapes the directory; on
other systems, `openat` with `O_NOFOLLOW` on each component is the portable
approximation.[^openat2] The lexical check above rejects the obvious cases
early; the handle-relative open is what holds against a hostile file system.

## Launch programs with an argument vector, not a shell

**MUST:** Never build a shell command from input. `system()` and `popen()`
hand the string to `/bin/sh` (or `cmd.exe`), so `;`, `|`, `$(...)` and quotes
in the input become commands (CERT
[ENV33-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/environment-env/env33-c)):

```cpp bad
#include <cstdlib>
#include <string>

void make_thumbnail(const std::string& file_from_upload) {
    std::string cmd = "convert " + file_from_upload + " thumb.png";
    std::system(cmd.c_str());     // BAD: "x.png; rm -rf ~" runs two commands
}
```

Pass the program and each argument separately, so no shell parses them:

```cpp
#include <string>
#if defined(__unix__) || defined(__APPLE__)
#  include <spawn.h>
#  include <sys/wait.h>
extern char** environ;

int make_thumbnail(const std::string& file_from_upload) {
    // "--" ends option parsing, so a file named "-rf" isn't read as a flag.
    char* const argv[] = {const_cast<char*>("convert"), const_cast<char*>("--"),
                          const_cast<char*>(file_from_upload.c_str()),
                          const_cast<char*>("thumb.png"), nullptr};
    pid_t pid;
    if (posix_spawnp(&pid, "convert", nullptr, nullptr, argv, environ) != 0) return -1;
    int status = 0;
    waitpid(pid, &status, 0);
    return status;
}
#endif
```

**MUST:** Control everything else the child inherits: choose the executable
by absolute path or a fixed search path, pass a minimal environment rather
than the parent's, close or mark close-on-exec every descriptor it shouldn't
have, set its working directory, and drop privileges in the right order
(CERT
[POS36-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/posix-pos/pos36-c),
[POS37-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/posix-pos/pos37-c)).
On Windows the equivalent is `CreateProcessW` with an explicit application
name and a quoted command line.

**SHOULD:** Load plugins and shared libraries only from directories the
attacker can't write, by full path, and never from the current directory.

## Treat every C interface as a trust boundary

**MUST:** For each function that crosses into C (or another language), write
down and enforce: who owns each pointer and how it's released (the matching
deallocator), how long the callee may keep it, the length of every buffer
(never infer it from a terminator you didn't check), whether null is allowed,
how errors are reported, and how long a registered callback and its user-data
pointer must stay valid. Wrap the C API in a small C++ layer that takes
`std::span`/`std::string_view`, returns RAII owners, and checks the C side's
error codes (see [ownership](ownership-and-lifetime.md#make-ownership-explicit)
for a `unique_ptr` with a deleter, and
[error handling](error-handling.md#put-exception-boundaries-where-control-leaves-your-code)
for exceptions at callbacks). Close what you open (CERT
[FIO51-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/input-output-fio/fio51-cpp)).

The other side doesn't enforce C++'s rules for you: a C library that keeps a
pointer you passed still needs that object alive, whatever your RAII thinks.

## See also

- [filesystem](../examples/io/filesystem.md) · [file streams](../examples/io/files.md) ·
  [signals](../examples/language-support/signals.md)
- [Threat models](threat-modeling.md) · [deployment](deployment.md) (least privilege, sandboxes)
- man pages: [openat2(2)](https://man7.org/linux/man-pages/man2/openat2.2.html),
  [posix_spawn(3)](https://man7.org/linux/man-pages/man3/posix_spawn.3.html),
  [path_resolution(7)](https://man7.org/linux/man-pages/man7/path_resolution.7.html)
- Tests: [`tests/security/filesystem-and-processes_test.cpp`](../../tests/security/filesystem-and-processes_test.cpp)

[^openat2]: [openat2(2)](https://man7.org/linux/man-pages/man2/openat2.2.html): `RESOLVE_BENEATH` "do not permit the path resolution to succeed if any component of the resolution is not a descendant of the directory indicated by `dirfd`"; Linux 5.6 and later.
