# File streams

> Read and write files through the stream interface, with RAII open/close.
> **Header:** `<fstream>` · **Since:** C++98

## The three types

| Type | Direction |
|---|---|
| `std::ifstream` | input (read) |
| `std::ofstream` | output (write; truncates by default) |
| `std::fstream` | both |

Opening happens in the constructor; **closing happens in the destructor** — no
manual `close()` needed (though you can). That's RAII for files.

## Examples

### Write a file

```cpp
#include <fstream>

void write_file() {
    std::ofstream out{"data.txt"};        // opens (truncates) on construction
    if (!out) { /* failed to open */ }
    out << "line 1\n" << 42 << '\n';
}                                         // closed automatically: `out` goes out of scope
```

### Read a file

```cpp
#include <fstream>

void read_file() {
    std::ifstream in{"data.txt"};
    if (!in) { /* handle open failure */ }

    std::string line;
    while (std::getline(in, line)) { /* process each line */ }

    // or token by token:
    int n;
    while (in >> n) { /* each int */ }
}
```

### Read an entire file into a string

```cpp
#include <fstream>

std::string read_all(const char* filename) {
    std::ifstream in{filename, std::ios::binary};
    return std::string{std::istreambuf_iterator<char>{in},
                       std::istreambuf_iterator<char>{}};
}
```

## Open modes

Combine with `|`:

```cpp
#include <fstream>

void open_modes() {
    std::ofstream log{"log.txt", std::ios::app};             // append, don't truncate
    std::fstream  io {"f.bin",  std::ios::in | std::ios::out | std::ios::binary};
    std::ofstream bin{"g.bin",  std::ios::binary};           // no text/newline translation
}
```

| Flag | Meaning |
|---|---|
| `ios::in` / `ios::out` | read / write |
| `ios::app` | append to end |
| `ios::trunc` | discard existing contents (default for `ofstream`) |
| `ios::ate` | seek to end on open |
| `ios::binary` | no text translation (use for non-text data) |

## Binary I/O & positioning

```cpp
#include <fstream>

void binary_io(std::ofstream& out, std::ifstream& in) {   // both opened with ios::binary
    int value = 42;
    out.write(reinterpret_cast<const char*>(&value), sizeof value);   // raw bytes
    in.read (reinterpret_cast<char*>(&value), sizeof value);
    in.seekg(0, std::ios::end);   auto size = in.tellg();              // size via seek
    in.seekg(0);                                                        // back to start
}
```

## Gotchas

- **Always check the stream after opening.** `if (!in)` — a missing file or
  permission error otherwise silently produces no data.
- **Use `ios::binary` for non-text files.** On Windows, text mode translates
  `\n`↔`\r\n` and can corrupt binary data and byte counts.
- **`ofstream` truncates by default.** To preserve and append, pass `ios::app`.
- **Don't `reinterpret_cast`-write non-trivially-copyable types.** Binary dumping
  a struct with pointers/`std::string` writes garbage; serialize fields
  explicitly. Endianness/padding also make raw dumps non-portable.

## See also

- [std::basic_fstream — cppreference](https://en.cppreference.com/w/cpp/io/basic_fstream)
- [std::ios_base::openmode — cppreference](https://en.cppreference.com/w/cpp/io/ios_base/openmode)
