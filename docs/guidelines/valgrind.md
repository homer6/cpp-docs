# Valgrind — Memcheck, Helgrind and DRD

> Finding memory and threading bugs in a binary you didn't rebuild.
> **Sources:** Valgrind manual (Memcheck, Helgrind, DRD, core and advanced
> topics), Valgrind supported-platforms page — checked 2026-10-31 · pairs with
> [tooling](tooling.md)

Valgrind runs your program on a synthetic CPU and checks every memory access.
It needs **no recompile or relink**, so it works on third-party libraries you
can't instrument. The cost: Memcheck runs programs about **20–30× slower**
(Valgrind quick-start guide), so it's for targeted runs, not the whole suite.
Everything below was run with Valgrind 3.24 on Linux AArch64 (the `gcc:16`
Docker image, GCC 16); output is trimmed.

**Platforms:** Linux (x86-64 is the primary target; ARM64, x86, PowerPC,
s390x, MIPS and RISC-V are supported), FreeBSD, Solaris/illumos and Android.
macOS support stops at macOS 11 on Intel — there is no Apple Silicon port —
and Windows isn't supported
([platforms](https://valgrind.org/info/platforms.html)). On a Mac, run it in a
Linux container.

## Valgrind or ASan?

| Prefer Valgrind when… | Prefer ASan when… |
|---|---|
| You can't rebuild the binary or some of its libraries | You can rebuild everything — it's ~2× slower instead of 20–30× |
| You want uninitialized-value reports without MSan's "instrument everything" rule | You need stack and global buffer overflows (Memcheck missed both here; it checks heap blocks) |
| You want a second opinion on a sanitizer-clean run | You're on macOS or Windows, or want it on every test run |

Don't combine them: a sanitized binary under Valgrind stops at startup
(`ASan runtime does not come first in initial library list`). For data races,
[TSan](tooling.md#threadsanitizer-tsan) is faster and more precise than
Helgrind or DRD when you can rebuild.

## Memcheck

```console
$ g++ -g -O0 app.cpp -o app
$ valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./app
```

- Build with `-g` (and `-O0` or `-O1`) for accurate file and line numbers.
- `--leak-check=full` lists each leak with its allocation stack (the default,
  `summary`, only counts them).
- `--track-origins=yes` says where an uninitialized value came from (a stack
  variable or a heap block). Slower, but without it the report only says *that*
  a value was uninitialized.
- `--error-exitcode=1` makes the run fail in CI. Definite and possible leaks
  count as errors (`--errors-for-leak-kinds`, default `definite,possible`).

### Reading the report

```text
==744== Conditional jump or move depends on uninitialised value(s)
==744==    at 0x400810: main (mem.cpp:7)                   ← where it was used
==744==  Uninitialised value was created by a stack allocation
==744==    at 0x4007DC: main (mem.cpp:3)                   ← --track-origins
==744==
==744== Invalid read of size 4
==744==    at 0x400864: main (mem.cpp:10)                  ← use after free
==744==  Address 0x4e110d0 is 0 bytes inside a block of size 4 free'd
==744==    at 0x4889484: operator delete(void*, unsigned long)
==744==    by 0x40085F: main (mem.cpp:9)                   ← where it was freed
==744==  Block was alloc'd at
==744==    at 0x4885C0C: operator new(unsigned long)
==744==    by 0x400827: main (mem.cpp:8)                   ← where it was allocated
==744==
==744== 16 bytes in 1 blocks are definitely lost in loss record 1 of 1
==744==    at 0x488722C: operator new[](unsigned long)
==744==    by 0x4007D3: make() (mem.cpp:2)
==744==    by 0x4007EF: main (mem.cpp:4)
```

Each error is a stack read top-down, like an ASan report: the access, then
where the block was freed and allocated. An uninitialized value is reported
where it *changes behaviour* (a branch, a system call), not where it's copied.

Leak kinds: **definitely lost** — no pointer to the block remains; fix these.
**Indirectly lost** — reachable only through a definitely lost block; fixing
the parent fixes them. **Possibly lost** — only an interior pointer remains.
**Still reachable** — a pointer exists at exit, usually a deliberate global
(libstdc++ leaves one 72 KiB block); shown only with `--show-leak-kinds=all`.

## Helgrind and DRD — threading errors

```console
$ valgrind --tool=helgrind ./app
$ valgrind --tool=drd ./app
```

Both report data races and misuse of the pthreads API (unlocking a mutex
another thread holds, destroying a locked mutex). Helgrind also checks lock
order: it reports a *potential* deadlock when two locks are taken in
inconsistent orders, even if the run didn't deadlock. A race report names both
accesses:

```text
==770== Possible data race during read of size 4 at 0x42007C by thread #3
==770== Locks held: none
==770==    at 0x400B18: work() (race.cpp:4)
...
==770== This conflicts with a previous write of size 4 by thread #2
```

They understand pthreads primitives (which `std::thread` and `std::mutex` use
on Linux) but **not synchronization built from atomics**. An
`std::atomic<bool>` release/acquire handoff — correct code — gave two false
reports under each tool here, while TSan was silent. Code that synchronizes
through atomics (lock-free queues, reference counts) needs
`ANNOTATE_HAPPENS_BEFORE` / `ANNOTATE_HAPPENS_AFTER` from `<valgrind/helgrind.h>`,
or use TSan instead.

## Suppressions

Errors in code you can't fix (a system library, a deliberate leak) go in a
suppressions file. Let Valgrind write the entries:

```console
$ valgrind --leak-check=full --gen-suppressions=all ./app
$ valgrind --leak-check=full --suppressions=app.supp -s ./app
```

```text
{
   make-leak
   Memcheck:Leak
   match-leak-kinds: definite
   ...
   fun:_Z4makev
}
```

An entry is a name you choose, the tool and error kind (`Memcheck:Leak`,
`Memcheck:Addr4`, `Memcheck:Cond`, `Helgrind:Race`, …), then the stack,
innermost frame first, using **mangled** names as `--gen-suppressions` prints
them; `...` matches any number of frames and `*` any characters
(`fun:*make*`). Trim generated entries to the frames that identify the problem
so they survive inlining changes. Comments go on their own lines, starting
with `#`. `-s` lists which suppressions were used.

## Debugging under Valgrind (vgdb)

Valgrind contains a gdb server, so you can stop at the first error with a
debugger attached:

```console
$ valgrind --vgdb-error=0 ./app          # waits for gdb before starting
$ gdb ./app                              # in a second shell
(gdb) target remote | vgdb
(gdb) continue                           # stops at each Valgrind error
(gdb) monitor leak_check full reachable any
```

`--vgdb-error=N` waits for gdb after N errors (`0` = before the program runs,
so you can set breakpoints); `monitor` sends tool commands such as an
on-demand leak check. With several Valgrind processes running, add `--pid=PID`
to `vgdb`.

## Heap profiling

Valgrind's heap profilers, **Massif** (heap size over time) and **DHAT** (how
allocations are used), are covered in
[performance tooling](performance-tooling.md#memory-and-allocation).

## Gotchas

- **Newer CPU instructions:** Valgrind's x86-64 support goes up to AVX2
  ([platforms](https://valgrind.org/info/platforms.html)). A binary built with
  `-march=native` on a machine with AVX-512 can stop with an unhandled
  instruction; build the Valgrind target for a baseline architecture.
- **Custom allocators and memory pools** hide errors from Memcheck — it only
  sees `malloc`/`new`. Run with the system allocator, or annotate the pool
  with the client requests in `<valgrind/valgrind.h>` (`VALGRIND_MALLOCLIKE_BLOCK`,
  `VALGRIND_CREATE_MEMPOOL`).
- **Threads run one at a time** — Valgrind serialises them (core manual) — so
  timing-dependent bugs may not show up; a clean run isn't proof.

## See also

- Valgrind manual: [Memcheck](https://valgrind.org/docs/manual/mc-manual.html) · [Helgrind](https://valgrind.org/docs/manual/hg-manual.html) · [DRD](https://valgrind.org/docs/manual/drd-manual.html) ·
  [suppressions](https://valgrind.org/docs/manual/manual-core.html#manual-core.suppress) ·
  [vgdb](https://valgrind.org/docs/manual/manual-core-adv.html) · [quick start](https://valgrind.org/docs/manual/quick-start.html) ·
  [supported platforms](https://valgrind.org/info/platforms.html)
- [Tooling](tooling.md) · [Debugging](debugging.md) · [Performance tooling](performance-tooling.md)
