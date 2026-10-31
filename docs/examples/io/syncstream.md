# std::osyncstream

> A stream wrapper that collects output privately and writes it to the target in
> one piece, so lines from different threads don't interleave.
> **Header:** `<syncstream>` · **Since:** C++20

## Mental model

Writing to `std::cout` from several threads is not a data race, but the
characters of separate `<<` chains can interleave. An `osyncstream` gives each
writer its **own buffer** (a `std::basic_syncbuf`) and transfers it to the
wrapped stream buffer **atomically** when it is destroyed (or `emit()` is
called):

```
thread A:  osyncstream(cout) << "A1 " << "A2\n";  ──buffer──┐  emit (atomic)
thread B:  osyncstream(cout) << "B1 " << "B2\n";  ──buffer──┤──────────────> cout
                                                            │  "A1 A2\nB1 B2\n"
```

All `osyncstream`s that wrap the **same** stream buffer are serialised against
each other. Writes to `std::cout` that don't go through an `osyncstream` are not.

## Declaration

```cpp nocompile
namespace std {
  template <class CharT, class Traits = char_traits<CharT>,
            class Allocator = allocator<CharT>>
  class basic_syncbuf;                       // the buffering streambuf

  template <class CharT, class Traits = char_traits<CharT>,
            class Allocator = allocator<CharT>>
  class basic_osyncstream : public basic_ostream<CharT, Traits> {
  public:
    explicit basic_osyncstream(basic_streambuf<CharT, Traits>* wrapped);
    explicit basic_osyncstream(basic_ostream<CharT, Traits>& os);  // wraps os.rdbuf()
    ~basic_osyncstream();                    // calls emit()

    void emit();                             // transfer the buffer now
    streambuf_type* get_wrapped() const noexcept;
    syncbuf_type* rdbuf() const noexcept;
  };

  using osyncstream  = basic_osyncstream<char>;
  using wosyncstream = basic_osyncstream<wchar_t>;
  using syncbuf      = basic_syncbuf<char>;
  using wsyncbuf     = basic_syncbuf<wchar_t>;
}

// in <ostream>:
template <class CharT, class Traits>
basic_ostream<CharT, Traits>& emit_on_flush(basic_ostream<CharT, Traits>&);
template <class CharT, class Traits>
basic_ostream<CharT, Traits>& noemit_on_flush(basic_ostream<CharT, Traits>&);
template <class CharT, class Traits>
basic_ostream<CharT, Traits>& flush_emit(basic_ostream<CharT, Traits>&);
```

| Member / manipulator | Effect |
|---|---|
| destructor | `emit()`, then destroy |
| `emit()` | Atomically write everything buffered so far to the wrapped buffer; if a flush was requested since the last emit, flush the wrapped buffer too |
| `get_wrapped()` | The `streambuf*` being written to |
| `std::emit_on_flush` | From now on, `std::flush` / `std::endl` on this stream emit immediately |
| `std::noemit_on_flush` | Back to the default: a flush is only *remembered* and done at the next emit |
| `std::flush_emit` | Flush, then emit, right now |

## Compiler support

| Toolchain | `__cpp_lib_syncbuf` | Status |
|---|---|---|
| GCC 15.2 (libstdc++) | `201803L` | full |
| GCC 16.2 (libstdc++) | `201803L` | full |
| Clang 22 + libc++ | only with `-fexperimental-library` (`201803L`) | `osyncstream` works with the flag; `emit_on_flush`, `noemit_on_flush` and `flush_emit` are missing |

Without `-fexperimental-library`, libc++ 22 has the `<syncstream>` header but
`std::osyncstream` isn't declared.

## When to use it

- **Fits:** log or progress lines from worker threads to `std::cout` /
  `std::cerr` / a shared `std::ofstream`, when each line must stay whole.
- **Anti-fit:** high-volume logging. Every emit takes a lock shared by all
  syncstreams on that buffer; a dedicated logging thread or library scales better.
- **Alternative:** `std::print` / `std::println` (C++23) write one formatted
  call as a unit to a `FILE*`, which is often all you need — see
  [print functions](print.md).

## Examples

### Whole lines from many threads

```cpp
#include <iostream>
#include <syncstream>
#include <thread>
#include <vector>

void run_workers() {
    std::vector<std::jthread> workers;
    for (int i = 0; i < 4; ++i)
        workers.emplace_back([i] {
            std::osyncstream(std::cout) << "thread " << i << " says hi\n";
        });     // the temporary is destroyed at ';' -> one atomic emit
}
```

Run with GCC 15.2, every line comes out whole (order varies between runs).

### When does the text arrive?

```cpp
#include <cassert>
#include <ostream>
#include <sstream>
#include <syncstream>

void when_does_it_arrive() {
    std::ostringstream sink;
    {
        std::osyncstream out(sink);
        out << "first ";
        assert(sink.str() == "");                  // nothing transferred yet
        out.emit();
        assert(sink.str() == "first ");
        out << std::emit_on_flush << "second" << std::flush;
        assert(sink.str() == "first second");      // flush now emits
        out << std::noemit_on_flush << " third" << std::flush;
        assert(sink.str() == "first second");      // flush only remembered
    }
    assert(sink.str() == "first second third");    // destructor emitted the rest
}
```

Every `assert` holds when run with GCC 15.2 and GCC 16.2.

### Keep one syncstream for a multi-statement message

```cpp
#include <iostream>
#include <syncstream>
#include <vector>

void report(const std::vector<int>& values) {
    std::osyncstream out(std::cerr);
    out << "values:";
    for (int v : values) out << ' ' << v;
    out << '\n';
}   // emitted here, as one block
```

## Gotchas

- **`std::endl` doesn't emit by default.** It requests a flush, which happens
  at the next `emit()` (usually the destructor). Use `std::emit_on_flush` or
  `std::flush_emit` if you need the text out now.
- **Named syncstreams hold text until they die.** A long-lived `osyncstream`
  can sit on its output for the whole scope — and lose it if the process
  crashes. Prefer the temporary `std::osyncstream(std::cout) << …;` form.
- **Only syncstreams are synchronised.** A plain `std::cout << …` from another
  thread isn't serialised against them and can still interleave with their
  output.
- **Wrapping the same stream through different buffers defeats it.** The lock
  is keyed by the wrapped `streambuf*`; `std::cout` and a `std::ofstream` on
  `/dev/stdout` are different buffers.
- **libc++ needs `-fexperimental-library`** and still lacks the three
  manipulators (Clang 22).

## See also

- [`std::basic_osyncstream` — cppreference](https://en.cppreference.com/w/cpp/io/basic_osyncstream)
- [`std::basic_syncbuf` — cppreference](https://en.cppreference.com/w/cpp/io/basic_syncbuf)
- [`std::emit_on_flush` — cppreference](https://en.cppreference.com/w/cpp/io/manip/emit_on_flush)
- [P0053 — C++ Synchronized Buffered Ostream](https://wg21.link/p0053)
- [Streams & state](streams.md) · [Print functions](print.md) ·
  [Threads](../concurrency/threads.md)
