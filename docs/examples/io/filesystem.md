# std::filesystem

> Portable paths, file metadata, and directory operations.
> **Header:** `<filesystem>` · **Since:** C++17 · usually aliased `namespace fs = std::filesystem;`

## Paths

`fs::path` is the portable path type — it handles separators and conversions, and
composes with `/`:

```cpp
#include <filesystem>
namespace fs = std::filesystem;

fs::path p = "dir/sub/file.txt";
p / "more";                      // append a component with operator/
p.filename();                    // "file.txt"
p.stem();                        // "file"
p.extension();                   // ".txt"
p.parent_path();                 // "dir/sub"
p.is_absolute();
```

## Querying the filesystem

```cpp
fs::exists(p);
fs::is_directory(p);  fs::is_regular_file(p);
fs::file_size(p);                            // bytes (regular files)
fs::last_write_time(p);                       // a chrono file_time_point
fs::current_path();                           // CWD
fs::absolute(p);  fs::canonical(p);            // resolve to absolute/real path
```

## Directory iteration

```cpp
for (const fs::directory_entry& e : fs::directory_iterator{"some/dir"})
    if (e.is_regular_file())
        /* e.path(), e.file_size() */;

for (const auto& e : fs::recursive_directory_iterator{"some/dir"})
    /* walks subdirectories too */;
```

## Modifying operations

```cpp
fs::create_directory("out");
fs::create_directories("a/b/c");              // makes intermediate dirs
fs::copy("src.txt", "dst.txt");
fs::copy("dir", "backup", fs::copy_options::recursive);
fs::rename("old", "new");
fs::remove("file.txt");                        // returns false if it didn't exist
fs::remove_all("dir");                         // recursive delete; returns count
```

## Error handling — two flavors

Every operation has a throwing form and a non-throwing `error_code` overload:

```cpp
try {
    fs::create_directory("out");              // throws fs::filesystem_error on failure
} catch (const fs::filesystem_error& e) { /* e.path1(), e.code() */ }

std::error_code ec;
fs::create_directory("out", ec);              // no throw; check ec
if (ec) { /* handle */ }
```

## Gotchas

- **Filesystem calls race with the world.** `exists()` then open is a TOCTOU
  race — another process can change things in between. Prefer "try the operation,
  handle the error" over "check then act."
- **Pick the throwing or `error_code` overload deliberately.** The `error_code`
  forms are the way to avoid exceptions on expected failures (missing file, no
  permission).
- **Paths have a native encoding.** Don't assume `path::string()` is UTF-8 on all
  platforms (Windows uses wide natively); use `u8string()` when you need UTF-8.
- **`remove_all` is recursive and unforgiving** — double-check the target before
  calling it.

## See also

- [Filesystem library — cppreference](https://en.cppreference.com/w/cpp/filesystem)
- [std::filesystem::path — cppreference](https://en.cppreference.com/w/cpp/filesystem/path)
- [std::filesystem::directory_iterator — cppreference](https://en.cppreference.com/w/cpp/filesystem/directory_iterator)
