# Streams & stream state

> The `iostream` model: objects you shift data into (`<<`) and out of (`>>`),
> with a state you must check. **Header:** `<iostream>` · **Since:** C++98

## Mental model

A stream is a typed channel. `std::cout`/`cerr`/`clog` are output streams,
`std::cin` an input stream. `<<` formats and writes; `>>` parses and reads
(skipping leading whitespace by default). Every stream carries **state flags**
telling you whether the last operation worked.

## The basics

```cpp
#include <iostream>

std::cout << "x = " << 42 << '\n';        // chained output
std::cerr << "error\n";                    // unbuffered-ish error stream

int n;
std::cin >> n;                             // parse an int (skips whitespace)
std::string word;
std::cin >> word;                          // one whitespace-delimited token
std::getline(std::cin, line);              // a whole line (into std::string)
```

## Stream state — always check reads

```cpp
int n;
if (std::cin >> n) {                        // the stream is truthy iff the read succeeded
    // use n
} else {
    // parse failed or EOF
}

std::cin.good();   // no flags set
std::cin.eof();    // hit end of input
std::cin.fail();   // last formatted op failed (e.g. letters where an int was expected)
std::cin.bad();    // unrecoverable stream error
std::cin.clear();  // reset the flags (after handling a failed read)
```

The idiom for reading many values:

```cpp
int x;
while (std::cin >> x) { /* process x until EOF or bad input */ }
```

## Newlines and flushing

```cpp
std::cout << "line\n";           // newline, NO flush — prefer this in loops
std::cout << "line" << std::endl;// newline AND flush — slower; use sparingly
std::cout << std::flush;         // explicit flush without a newline
```

## Gotchas

- **`std::endl` flushes every time** — in a tight output loop that's a real
  performance hit. Use `'\n'`; flush explicitly only when you need it (e.g.
  before a prompt or a crash-prone section).
- **`>>` leaves the newline in the buffer.** Mixing `cin >> n` then
  `std::getline` reads an empty line — consume the leftover with
  `std::cin.ignore(...)` or `std::ws`.
- **A failed `>>` leaves the variable unchanged and sets `failbit`.** Subsequent
  reads no-op until you `clear()` (and usually `ignore()` the bad input).
- **Sync with C stdio costs speed.** `std::ios_base::sync_with_stdio(false);` (and
  not mixing `printf`/`cout`) speeds up heavy I/O — but then don't interleave C
  and C++ I/O.

## See also

- [std::cout / cin — cppreference](https://en.cppreference.com/w/cpp/io/cout)
- [std::basic_ios state functions — cppreference](https://en.cppreference.com/w/cpp/io/basic_ios)
- [std::getline — cppreference](https://en.cppreference.com/w/cpp/string/basic_string/getline)
