# SL: The Standard Library — examples

> Our examples for the [SL rules](../stdlib.md): use the standard library,
> use it type-safely, and pick the right container, string type and I/O.

## SL.1:  Use libraries wherever possible

Before writing an algorithm or a data structure, look for one that exists.
A library function is already written, tested and documented, and it gets
faster and safer without you doing anything.

```cpp bad
#include <vector>
int largest(const std::vector<int>& v) {
    int best = v[0];                       // BAD: a hand-written max
    for (std::size_t i = 1; i < v.size(); ++i)
        if (v[i] > best) best = v[i];
    return best;
}
```

```cpp
#include <algorithm>
#include <vector>
int largest(const std::vector<int>& v) {
    return std::ranges::max(v);   // GOOD: one call, named for what it does
}
```

The BAD version compiles and works; it's more code to read and check, for
something the library already names. (Both require a non-empty `v`.)

## SL.2: Prefer the standard library to other libraries

When the standard library and another library both do the job, use the
standard one. Every C++ programmer knows it, every toolchain ships it, and it
won't be abandoned.

```cpp nocompile
#include <boost/optional.hpp>
boost::optional<int> find_id(const char* name);   // BAD: an extra dependency for what std has
```

```cpp
#include <optional>
std::optional<int> find_id(const char* name);   // GOOD: standard since C++17
```

The BAD block needs a third-party header, so it's `nocompile`.

## SL.3: Do not add non-standard entities to namespace `std`

Declaring your own names in `std` is undefined behaviour and can clash with a
later standard. Put them in your own namespace. What *is* allowed is
specializing a standard template, such as `std::hash`, for your own type.

```cpp bad
#include <string>
namespace std {
    string trim(const string& s);   // BAD: undefined behaviour; a future std::trim could clash
}
```

```cpp
#include <cstddef>
#include <functional>
#include <string>
namespace text {
    std::string trim(const std::string& s);   // GOOD: our own namespace
}
struct UserId { int value; };
template <>
struct std::hash<UserId> {                     // GOOD: specializing for our own type is allowed
    std::size_t operator()(const UserId& u) const noexcept { return std::hash<int>{}(u.value); }
};
```

The BAD version compiles; the standard just doesn't promise what it does.

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.4: Use the standard library in a type-safe manner

Prefer the parts of the library that know the types they're given. `printf`
trusts its format string, so a mismatch is undefined behaviour at run time;
`std::format` and `std::print` (C++20/23) check the format string against
the arguments at compile time.

```cpp bad
#include <cstdio>
void report(long bytes) {
    std::printf("%d bytes\n", bytes);   // BAD: %d expects int: undefined behaviour (a warning at best)
}
```

```cpp
#include <format>
#include <string>
std::string report(long bytes) {
    return std::format("{} bytes\n", bytes);   // GOOD: the type comes from the argument
}
```

The BAD version compiles (GCC and Clang warn with `-Wformat`).

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.con.1: Prefer using STL `array` or `vector` instead of a C array

A C array turns into a pointer when passed, losing its size, and can't be
copied or compared. `std::array` (fixed size, on the stack like a C array)
and `std::vector` (growable) keep their size and behave like values.

```cpp compile-fail="no matching function for call to .?size"
#include <iterator>
int sum(const int scores[4]) {        // BAD: really a const int*; the 4 is ignored
    int total = 0;
    for (std::size_t i = 0; i < std::size(scores); ++i)   // error: a pointer has no size
        total += scores[i];
    return total;
}
```

```cpp
#include <array>
#include <numeric>
int sum(const std::array<int, 4>& scores) {   // GOOD: the size is part of the type
    return std::accumulate(scores.begin(), scores.end(), 0);
}
```

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.con.2: Prefer using STL `vector` by default unless you have a reason to use a different container

`std::vector` is contiguous, so it's fast to walk and cache-friendly, has
random access, and works with every algorithm. For small to medium sizes it
usually beats node-based containers even at their own game. Reach for
another container when you've measured a reason.

```cpp compile-fail="no match for call|no matching function for call"
#include <algorithm>
#include <list>
void rank(std::list<int>& scores) {
    std::ranges::sort(scores);   // BAD: list was picked without a reason, and has no random access
}
```

```cpp
#include <algorithm>
#include <vector>
void rank(std::vector<int>& scores) {
    std::ranges::sort(scores);   // GOOD: vector by default
}
```

Note the two initializations: `std::vector<int> v(20)` holds twenty zeros,
`std::vector<int> v{20}` holds one element, 20.

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.con.3: Avoid bounds errors

Reading or writing past the end of a container is undefined behaviour and a
classic security hole. Loop with range-`for` or algorithms, which can't run
off the end, and use `at()` (which throws `std::out_of_range`) for an index
you haven't checked.

```cpp bad
#include <vector>
int last(const std::vector<int>& v) {
    return v[v.size()];   // BAD: one past the end: undefined behaviour
}
```

```cpp
#include <vector>
int last(const std::vector<int>& v) {
    return v.at(v.size() - 1);   // GOOD: throws std::out_of_range when v is empty
}
int total(const std::vector<int>& v) {
    int t = 0;
    for (int x : v) t += x;      // GOOD: range-for stays in bounds
    return t;
}
```

The BAD version compiles; C++26 hardened standard libraries can trap it at
run time, but the language doesn't require it.

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.con.4: don't use `memset` or `memcpy` for arguments that are not trivially-copyable

`memset` and `memcpy` copy raw bytes. For a type with a virtual function, a
`std::string` member or a user-written copy, those bytes include pointers the
object owns or its vtable pointer, so the result is a broken object. Use
assignment and value-initialization; check the type when you do need bytes.

```cpp bad
#include <cstring>
#include <string>
struct Entry { std::string key; int hits; };
void reset(Entry& e) {
    std::memset(&e, 0, sizeof e);   // BAD: overwrites the string's internals: undefined behaviour
}
```

```cpp
#include <string>
#include <type_traits>
struct Entry { std::string key; int hits; };
static_assert(!std::is_trivially_copyable_v<Entry>);   // so no memset/memcpy
void reset(Entry& e) {
    e = {};   // GOOD: each member resets itself
}
```

The BAD version compiles (GCC warns with `-Wclass-memaccess`).

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.str.1: Use `std::string` to own character sequences

`std::string` allocates, grows, copies and frees its characters for you.
Owning text through `char*` means computing sizes, remembering the
terminator and freeing it, which is where the bugs are.

```cpp bad
#include <cstdlib>
#include <cstring>
char* join(const char* a, const char* b) {
    char* out = static_cast<char*>(std::malloc(std::strlen(a) + std::strlen(b)));   // BAD: one byte short
    std::strcpy(out, a);
    std::strcat(out, b);
    return out;   // and the caller must remember to free it
}
```

```cpp
#include <string>
#include <string_view>
std::string join(std::string_view a, std::string_view b) {
    std::string out{a};   // GOOD: sizes, terminator and freeing are handled
    out += b;
    return out;
}
```

The BAD version compiles; it writes one byte past the allocation, and
doesn't check for `malloc` failing.

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.str.2: Use `std::string_view` or `gsl::span<char>` to refer to character sequences

A function that only reads text should take `std::string_view`. It accepts a
`std::string`, a literal or part of either, without copying, and without
caring how the characters are stored. (`std::span<char>`, C++20, is the
standard counterpart of `gsl::span<char>`.)

```cpp compile-fail="cannot convert|no matching function"
#include <string>
std::size_t count_spaces(const char* s);   // BAD: only C strings
std::size_t spaces_in(const std::string& line) {
    return count_spaces(line);   // error: a std::string isn't a const char*
}
```

```cpp
#include <algorithm>
#include <string>
#include <string_view>
std::size_t count_spaces(std::string_view s) {   // GOOD: any contiguous characters
    return static_cast<std::size_t>(std::ranges::count(s, ' '));
}
std::size_t a = count_spaces("a b c");
std::size_t b = count_spaces(std::string{"x y"});
```

A `string_view` doesn't own the characters: it must not outlive what it
views.

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.str.3: Use `zstring` or `czstring` to refer to a C-style, zero-terminated, sequence of characters

A bare `char*` could be one character, a buffer, a C string or an owner. The
GSL's `zstring` (`char*`) and `czstring` (`const char*`) aliases say "a
zero-terminated string, not owned". Without the GSL, a local alias does the
same.

```cpp bad
void set_title(const char* p);   // BAD: a C string? a single char? may it be null? who frees it?
```

```cpp
using czstring = const char*;   // like gsl::czstring: zero-terminated, not owned
void set_title(czstring title);   // GOOD: the type says what is expected
```

The BAD block compiles; the problem is what the declaration doesn't say.

## SL.str.4: Use `char*` to refer to a single character

Keep `char*` for what it literally is, a pointer to one `char`. An array of
characters without a terminator passed as `const char*` is read until some
zero byte happens to turn up.

```cpp bad
#include <iostream>
const char letters[] = {'a', 'b', 'c'};   // no '\0'
void show() {
    std::cout << letters << '\n';   // BAD: treated as a C string: reads past the end
}
```

```cpp
#include <iostream>
#include <string_view>
const char letters[] = {'a', 'b', 'c'};
void bump(char* c) { ++*c; }        // GOOD: char* for one character
void show() {
    std::cout << std::string_view{letters, std::size(letters)} << '\n';   // GOOD: the length is explicit
}
```

The BAD version compiles; its output is undefined.

## SL.str.5: Use `std::byte` to refer to byte values that do not necessarily represent characters

Raw memory isn't text. `std::byte` (C++17) is a byte with only bitwise
operations: no arithmetic, and it doesn't print as a character, so it can't
be mistaken for a `char` or a small number.

```cpp bad
#include <iostream>
void dump(const unsigned char* data, int n) {
    for (int i = 0; i < n; ++i)
        std::cout << data[i] << ' ';   // BAD: prints bytes as characters, not values
}
```

```cpp
#include <cstddef>
#include <iostream>
#include <span>
void dump(std::span<const std::byte> data) {   // GOOD: bytes, not characters
    for (std::byte b : data)
        std::cout << std::to_integer<int>(b) << ' ';
}
```

The BAD version compiles; it prints control characters and garbage for
binary data.

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.str.10: Use `std::string` when you need to perform locale-sensitive string operations

The `<locale>` facilities are written around `std::basic_string`: a
`std::locale` compares two strings with its collation rules, and the facets
take string data. A `std::string_view` doesn't fit those interfaces, so
convert to `std::string` when a locale is involved.

```cpp compile-fail="no match for call|no matching function for call"
#include <locale>
#include <string_view>
bool before(std::string_view a, std::string_view b, const std::locale& loc) {
    return loc(a, b);   // BAD: std::locale compares basic_strings, not views
}
```

```cpp
#include <locale>
#include <string>
bool before(const std::string& a, const std::string& b, const std::locale& loc) {
    return loc(a, b);   // GOOD: collation order of loc
}
```

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.str.11: Use `gsl::span<char>` rather than `std::string_view` when you need to mutate a string

`std::string_view` is read-only: the compiler rejects a write through it. To
let a function change characters in place, pass a `std::span<char>` (the
standard version of `gsl::span<char>`) or the string itself.

```cpp compile-fail="read-only|returns a const value"
#include <string_view>
void capitalize(std::string_view word) {
    if (!word.empty()) word[0] = 'A';   // BAD: a string_view's characters are const
}
```

```cpp
#include <cctype>
#include <span>
void capitalize(std::span<char> word) {   // GOOD: a mutable view
    if (!word.empty())
        word[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(word[0])));
}
```

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.str.12: Use the `s` suffix for string literals meant to be standard-library `string`s

A string literal is an array of `const char`, so type deduction gives you
`const char*`, not `std::string`. When you mean a `std::string`, say so with
the `s` suffix from `std::string_literals`.

```cpp compile-fail="non-class type|not a structure or union"
#include <utility>
auto city = std::make_pair("Tokyo", 9.0);   // BAD: first is const char*
auto letters = city.first.size();           // error: a pointer has no members
```

```cpp
#include <string>
#include <utility>
using namespace std::string_literals;
auto city = std::make_pair("Tokyo"s, 9.0);   // GOOD: first is std::string
auto letters = city.first.size();
```

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.io.1: Use character-level input only when you have to

Reading one character at a time and assembling tokens yourself is long,
easy to get wrong (buffer sizes, whitespace) and slower. Let `>>` or
`std::getline` read whole words and lines into a `std::string`.

```cpp bad
#include <cctype>
#include <iostream>
void read_word(char (&buf)[16]) {
    int i = 0;
    char c;
    while (std::cin.get(c) && !std::isspace(static_cast<unsigned char>(c)))
        buf[i++] = c;   // BAD: no bound check and no terminator: overflows on a long word
}
```

```cpp
#include <iostream>
#include <string>
std::string read_word(std::istream& in) {
    std::string word;
    in >> word;   // GOOD: skips whitespace, any length
    return word;
}
```

The BAD version compiles; a word of 16 characters or more writes past `buf`.

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.io.2: When reading, always consider ill-formed input

Input comes from outside the program, so assume it can be wrong. Check the
stream after each read, and validate the value, before using it. On a failed
`>>` into a number, the stream is in a failed state and the target is set to
0.

```cpp bad
#include <istream>
int read_age(std::istream& in) {
    int age;
    in >> age;
    return age;   // BAD: "abc" or "-5" go straight through
}
```

```cpp
#include <istream>
#include <optional>
std::optional<int> read_age(std::istream& in) {
    int age = 0;
    if (!(in >> age) || age < 0 || age > 150) return std::nullopt;   // GOOD: the read and the value are checked
    return age;
}
```

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.io.3: Prefer `iostream`s for I/O

Streams are type-safe, manage their own memory and extend to your types: give
a type an `operator<<` and it prints like a built-in. The `printf` family
trusts a format string and can't print a `std::string` or a user type. Since
C++23, `std::print` is a type-safe, extensible alternative too.

```cpp nocompile
#include <cstdio>
#include <string>
void greet(const std::string& name) {
    std::printf("Hello, %s\n", name);   // BAD: passes a std::string to %s: undefined behaviour
}
```

```cpp
#include <ostream>
#include <string>
struct Point { int x, y; };
std::ostream& operator<<(std::ostream& os, const Point& p) {   // GOOD: extends streams to Point
    return os << '(' << p.x << ", " << p.y << ')';
}
void greet(std::ostream& os, const std::string& name, Point where) {
    os << "Hello, " << name << " at " << where << '\n';
}
```

The BAD block compiles with GCC (with a warning) and is rejected by Clang
(`-Wnon-pod-varargs` is an error by default), so it's `nocompile` rather
than `compile-fail`.

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.io.10: Unless you use `printf`-family functions call `ios_base::sync_with_stdio(false)`

By default the standard streams stay synchronized with C's `stdio`, which
costs time on every operation. A program that uses only streams can turn that
off once, at the start, before any I/O. After that, mixing `printf` and
`std::cout` can interleave output out of order.

```cpp bad
#include <cstdio>
#include <iostream>
int main() {
    std::ios_base::sync_with_stdio(false);
    std::cout << "first ";
    std::printf("second\n");   // BAD: unsynchronized: may print before "first "
}
```

```cpp run
#include <iostream>
int main() {
    std::ios_base::sync_with_stdio(false);   // GOOD: once, before any I/O; streams only from here
    std::cout << "first " << "second\n";
}
```

The BAD version compiles; the order of its output isn't specified.

## SL.io.50: Avoid `endl`

`std::endl` writes `'\n'` and then flushes the stream. The flush is rarely
needed (the terminal and file buffers flush on their own) and can make
line-by-line output much slower. Write `'\n'`; call `std::flush` when you
really need it.

```cpp bad
#include <iostream>
#include <vector>
void print_all(const std::vector<int>& v) {
    for (int x : v) std::cout << x << std::endl;   // BAD: a flush per line
}
```

```cpp
#include <iostream>
#include <vector>
void print_all(const std::vector<int>& v) {
    for (int x : v) std::cout << x << '\n';   // GOOD: same text, no flush per line
}
```

The BAD version compiles and prints the same text; it's only slower. `endl`
doesn't write `"\r\n"` on Windows either: both write `'\n'`.

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).

## SL.C.1: Don't use setjmp/longjmp

`longjmp` jumps back to a `setjmp` point without running the destructors of
the objects in between, so RAII owners leak; when any of them has a
non-trivial destructor, the behaviour is undefined. Use exceptions, which
unwind and destroy.

```cpp bad
#include <csetjmp>
#include <vector>
std::jmp_buf on_error;
void parse() {
    std::vector<int> tokens(1000);
    std::longjmp(on_error, 1);   // BAD: skips ~vector: undefined behaviour, and a leak in practice
}
int run() {
    if (setjmp(on_error) == 0) parse();
    return 0;
}
```

```cpp
#include <stdexcept>
#include <vector>
void parse() {
    std::vector<int> tokens(1000);
    throw std::runtime_error{"parse error"};   // GOOD: unwinding destroys tokens
}
int run() {
    try { parse(); } catch (const std::runtime_error&) { return 1; }
    return 0;
}
```

The BAD version compiles.

Tested by [`tests/guidelines/stdlib_test.cpp`](../../../../tests/guidelines/stdlib_test.cpp).
