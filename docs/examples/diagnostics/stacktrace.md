# std::stacktrace

> Capture and print the current call stack — portable stack traces, finally in the
> standard. **Header:** `<stacktrace>` · **Since:** C++23

## Mental model

`std::stacktrace::current()` snapshots the chain of calls that led to this point.
Each frame (`std::stacktrace_entry`) can report its function name, source file,
and line (when debug info is available). It's a container of entries — iterate it,
or stream the whole thing.

## Examples

```cpp
#include <stacktrace>
#include <iostream>

void inner() {
    auto trace = std::stacktrace::current();
    std::cout << trace;                 // pretty-prints all frames
}

// inspect frames individually:
for (const std::stacktrace_entry& f : std::stacktrace::current()) {
    f.description();                    // function name
    f.source_file();                    // file
    f.source_line();                    // line
}
```

### In error reporting

```cpp
try {
    risky();
} catch (const std::exception& e) {
    std::cerr << e.what() << "\n at:\n" << std::stacktrace::current();
}
```

(`std::format("{}", trace)` also works — there's a formatter.)

## Gotchas

- **Needs debug/symbol info to be useful.** Without it (release builds stripped of
  symbols), entries may lack names, files, and lines — you get addresses only.
- **Capturing a trace is relatively expensive.** Don't grab one on a hot path;
  reserve it for error/diagnostic paths.
- **Support and quality vary.** It's C++23 and depends on a platform backend; some
  toolchains need a linker flag or a backtrace library to populate symbols.
- **`std::pmr::stacktrace`** exists if you want to control the allocation of the
  frame storage.

## See also

- [std::stacktrace — cppreference](https://en.cppreference.com/w/cpp/utility/basic_stacktrace)
- [std::stacktrace_entry — cppreference](https://en.cppreference.com/w/cpp/utility/stacktrace_entry)
