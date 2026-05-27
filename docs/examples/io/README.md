# Input/output library

The stream hierarchy for console, file, and in-memory I/O, plus the C++17
filesystem. Mirrors [cppreference's I/O library](https://en.cppreference.com/w/cpp/io).

| Page | Header | Since | What |
|---|---|---|---|
| [Streams & state](streams.md) | `<iostream>` | C++98 | `cin`/`cout`, `<<`/`>>`, stream state flags |
| [File streams](files.md) | `<fstream>` | C++98 | reading/writing files, open modes |
| [String streams](string-streams.md) | `<sstream>` | C++98 | build/parse strings via the stream API |
| [Manipulators](manipulators.md) | `<iomanip>` | C++98 | `setw`, `setprecision`, `hex`, `boolalpha` |
| [Filesystem](filesystem.md) | `<filesystem>` | C++17 | paths, directory iteration, file ops |

> For modern formatted output prefer [`std::print` / `std::format`](../text-processing/formatting.md)
> (C++20/23) over `cout <<` chains — type-safe, faster, and far more readable.
