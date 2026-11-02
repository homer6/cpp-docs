# NL: Naming and layout suggestions — examples

> Our examples for the [NL suggestions](../naming.md): defaults for names,
> comments and layout. The point is consistency; where a project already has a
> style, follow it. Most BAD blocks here compile, so they're tagged
> `nocompile`: the problem is for a reader (or a formatter or linter) to catch.

## NL.1: Don't say in comments what can be clearly stated in code

A comment that repeats the code adds nothing and goes stale when the code
changes. Put the meaning in names and types, and save comments for what code
can't say.

```cpp nocompile
// BAD: compiles; the comment restates the expression and will drift from it.
double f(double a, double b) {
    return a * 1.13 + b;   // multiply a by 1.13 and add b
}
```

```cpp
constexpr double tax_multiplier = 1.13;
double price_with_tax(double net_price, double shipping) {   // GOOD: the names say it
    return net_price * tax_multiplier + shipping;
}
```

## NL.2: State intent in comments

Code shows what it does; it can't show what it was meant to do. A comment
that states the intent (the contract, the reason for an odd choice) lets a
reader check the code against it.

```cpp nocompile
#include <vector>
// BAD: compiles; the comment narrates the loop instead of saying why it's backwards.
void drop_negatives(std::vector<int>& v) {
    // loop from the end to the start
    for (std::size_t i = v.size(); i-- > 0;)
        if (v[i] < 0) v.erase(v.begin() + static_cast<std::ptrdiff_t>(i));
}
```

```cpp
#include <vector>
// Removes every negative value, keeping the others in their original order.
void drop_negatives(std::vector<int>& v) {
    std::erase_if(v, [](int x) { return x < 0; });   // GOOD: the comment states the contract
}
```

## NL.3: Keep comments crisp

Long, chatty comments are skimmed or skipped. Say the one thing the reader
needs, in plain, correct English.

```cpp nocompile
// BAD: compiles; three lines of chatter to say one thing.
// so basically this function is the one that we use whenever we kind of
// want to get the thing clamped, like between low and high, you know, so it
// doesn't go out of range lol
int clamp_level(int x) { return x < 0 ? 0 : (x > 10 ? 10 : x); }
```

```cpp
#include <algorithm>
// Clamps a volume level to the valid range [0, 10].
int clamp_level(int x) { return std::clamp(x, 0, 10); }   // GOOD
```

## NL.4: Maintain a consistent indentation style

Indentation tells the reader what's inside what. When it disagrees with the
code, the reader believes the indentation; consistent indentation (best
applied by a formatter) makes such mistakes stand out.

```cpp nocompile
// BAD: compiles; the stray ';' ends the loop, and the indentation hides it.
int count_hits(const int* v, int n) {
    int hits = 0;
    for (int i = 0; i < n; ++i);
        if (v[0] > 0) ++hits;
    return hits;
}
```

```cpp
#include <span>
int count_hits(std::span<const int> v) {
    int hits = 0;
    for (int x : v) {
        if (x > 0)
            ++hits;     // GOOD: the indentation matches the structure
    }
    return hits;
}
```

## NL.5: Avoid encoding type information in names

Names like `print_int` or `strName` repeat what the type already says, go
wrong when the type changes, and make the caller pick the function by hand.
Name the operation and let overloading pick by type.

```cpp nocompile
#include <string>
// BAD: compiles; the caller must match each name to the argument's type.
void print_int(int);
void print_string(const std::string&);
void caller() { print_int(3); print_string("hi"); }
```

```cpp
#include <string_view>
void print(int);
void print(std::string_view);                 // GOOD: one name, chosen by type
void caller() { print(3); print("hi"); }
```

## NL.7: Make the length of a name roughly proportional to the length of its scope

A short name is fine where its whole scope fits on screen; a name visible
across a program needs to say what it is and not clash with others.

```cpp nocompile
// BAD: compiles; a global named n says nothing and clashes easily,
// while the loop index has a long name for a two-line scope.
int n = 0;
int total(const int* values, int count) {
    int total_sum_of_values = 0;
    for (int index_into_values = 0; index_into_values < count; ++index_into_values)
        total_sum_of_values += values[index_into_values];
    return total_sum_of_values;
}
```

```cpp
#include <span>
int active_connection_count = 0;           // GOOD: wide scope, descriptive name
int total(std::span<const int> values) {
    int sum = 0;
    for (int v : values) sum += v;         // GOOD: tiny scope, short name
    return sum;
}
```

## NL.8: Use a consistent naming style

Pick one style for the project and use it everywhere, so the shape of a name
tells you nothing surprising. Leave imported libraries in their own style. Also
avoid names with a double underscore or a leading underscore and capital
letter: those are reserved for the implementation.

```cpp nocompile
// BAD: compiles, but mixes three styles, and _Buffer is a reserved name.
struct http_request {};
struct HttpResponse {};
struct _Buffer {};
int getStatusCode(const HttpResponse&);
int read_body(http_request&);
```

```cpp
struct Http_request {};                        // GOOD: one style throughout
struct Http_response {};
struct Buffer {};
int status_code(const Http_response&);
int read_body(Http_request&);
```

## NL.9: Use `ALL_CAPS` for macro names only

Macros ignore scopes and types, so they get a name that looks different.
Giving an ordinary constant an ALL_CAPS name invites a clash: if a header
defines a macro of that name, the preprocessor rewrites your declaration.

```cpp compile-fail="expected unqualified-id"
#define MAX_USERS 100                 // from some header
constexpr int MAX_USERS = 50;         // BAD: becomes "constexpr int 100 = 50;"
```

```cpp
#define MAX_USERS 100                 // a macro may keep ALL_CAPS
constexpr int max_users = 50;         // GOOD: an ordinary constant gets an ordinary name
```

## NL.10: Prefer `underscore_style` names

With no house style to follow, the default is the standard library's:
lower case words separated by underscores. Consistency with an existing style
beats this default.

```cpp nocompile
// BAD (only as a default): compiles; camelCase where the standard library uses underscores.
#include <string>
std::size_t wordCount(const std::string& inputText);
```

```cpp
#include <string>
std::size_t word_count(const std::string& input_text);   // GOOD: matches std::
```

## NL.11: Make literals readable

Digit separators (`'`, C++14) make long numbers readable and typos visible.
Literal suffixes say what type you mean: `"text"s` is a `std::string`,
`250ms` a `std::chrono::milliseconds`.

```cpp nocompile
// BAD: compiles; is that 100 million or 10 million? And is the timeout in ms or s?
long long budget = 100000000;
int timeout = 250;
```

```cpp
#include <chrono>
#include <string>
using namespace std::literals;
long long budget = 100'000'000;                  // GOOD: readable at a glance
auto timeout = 250ms;                            // GOOD: the unit is in the type
auto greeting = "hello"s;                        // a std::string, not a const char*
auto mask = 0b1111'0000;
```

Tested by [`tests/guidelines/naming_test.cpp`](../../../../tests/guidelines/naming_test.cpp).

## NL.15: Use spaces sparingly

Space should group what belongs together; too much of it spreads code out
and can even change meaning: inside `< >`, an `#include` looks for a header
whose name has spaces in it.

```cpp compile-fail="No such file or directory|file not found"
#include < vector >                   // BAD: looks for a header named " vector "
int first_or_zero( const std::vector < int > & v ) { return v . empty ( ) ? 0 : v [ 0 ]; }
```

```cpp
#include <vector>
int first_or_zero(const std::vector<int>& v) { return v.empty() ? 0 : v[0]; }   // GOOD
```

## NL.16: Use a conventional class member declaration order

A familiar order lets readers find things: types, then constructors and
assignments and the destructor, then functions, then data; and `public`
before `protected` before `private`, each in one block.

```cpp nocompile
#include <string>
// BAD: compiles; data first, then the interface split across two public blocks.
class Player {
    std::string name_;
public:
    void rename(std::string n) { name_ = std::move(n); }
private:
    int score_ = 0;
public:
    explicit Player(std::string n) : name_{std::move(n)} {}
    int score() const { return score_; }
};
```

```cpp
#include <string>
#include <utility>
class Player {
public:
    explicit Player(std::string n) : name_{std::move(n)} {}   // GOOD: constructors,
    void rename(std::string n) { name_ = std::move(n); }       // then functions,
    int score() const { return score_; }
private:
    std::string name_;                                          // then data
    int score_ = 0;
};
```

## NL.17: Use K&R-derived layout

The default layout is K&R's (often called "Stroustrup" style in C++): the
opening brace of a class or a control statement on the same line, a function's
opening brace on its own line, one statement per line, and a space after `if`.

```cpp nocompile
// BAD (only as a default): compiles; braces and statements packed in no particular style.
struct Point{int x;int y;};
int sign(int x){if(x<0){return -1;}else if(x>0)return 1;return 0;}
```

```cpp
struct Point {
    int x;
    int y;
};

int sign(int x)
{
    if (x < 0)
        return -1;
    if (0 < x)
        return 1;
    return 0;
}
```

## NL.18: Use C++-style declarator layout

C++ style attaches `*` and `&` to the type (`int* p`, `T& r`), which reads as
"p is an int pointer". Combined with one name per declaration
([NL.21](#nl21-declare-one-name-only-per-declaration)), it avoids the
confusion that C style was designed around.

```cpp nocompile
#include <string>
// BAD (only as a default): compiles; C-style and mixed spacing.
std::string &longest(std::string &a, std::string & b);
int *find_slot(int *table);
```

```cpp
#include <string>
std::string& longest(std::string& a, std::string& b);   // GOOD
int* find_slot(int* table);
```

## NL.19: Avoid names that are easily misread

`l` and `1`, `O` and `0`, and names a letter apart (`item_count` and
`items_count`) are easy to confuse, on screen and in review. Pick names that
differ clearly.

```cpp nocompile
// BAD: compiles; l1 and ll, O0 and OO, look alike.
int l1 = 1, ll = 2;
int O0 = 0, OO = 3;
int itemcount = 4, itemscount = 5;
```

```cpp
int first_line = 1;              // GOOD: names that differ clearly
int line_count = 2;
int selected_items = 4;
int total_items = 5;
```

## NL.20: Don't place two statements on the same line

A second statement on a line is easy to miss, especially after an `if`,
where it looks guarded but isn't.

```cpp nocompile
// BAD: compiles; reset() looks like part of the if, but it always runs.
void reset();
void notify();
void check(bool failed) {
    if (failed) notify(); reset();
}
```

```cpp
void reset();
void notify();
void check(bool failed) {
    if (failed) {
        notify();
        reset();    // GOOD: one statement per line; the braces show what's guarded
    }
}
```

## NL.21: Declare one name (only) per declaration

In `int* p, q;` only `p` is a pointer: the `*` belongs to the declarator, not
to the type. One name per declaration removes the trap (and leaves room for an
initializer each).

```cpp compile-fail="nullptr_t"
void f() {
    int* p, q;        // BAD: q is an int, not an int*
    p = nullptr;
    q = nullptr;      // error: q isn't a pointer
}
```

```cpp
void f() {
    int* p = nullptr;   // GOOD: one name, one type, one initializer
    int* q = nullptr;
    use(p, q);
}
```

Tested by [`tests/guidelines/naming_test.cpp`](../../../../tests/guidelines/naming_test.cpp).

## NL.25: Don't use `void` as an argument type

In C++, `f()` already means "no arguments"; `f(void)` is a C habit that
only matters in a header shared with C.

```cpp nocompile
// BAD (in C++-only code): compiles; the void is noise.
int next_id(void);
```

```cpp
int next_id();   // GOOD: means the same thing
```

Tested by [`tests/guidelines/naming_test.cpp`](../../../../tests/guidelines/naming_test.cpp).

## NL.26: Use conventional `const` notation

`const int` and `int const` are the same type, but most code and teaching
material writes `const` first, so that's what readers expect.

```cpp nocompile
// BAD (only as a default): compiles; east const where readers expect west const.
int const limit = 10;
char const* const name = "x";
```

```cpp
const int limit = 10;              // GOOD
const char* const name = "x";
```

Tested by [`tests/guidelines/naming_test.cpp`](../../../../tests/guidelines/naming_test.cpp).

## NL.27: Use a `.cpp` suffix for code files and `.h` for interface files

*No example: it's about file names, which a code snippet can't show; what belongs in a header is covered by the SF rules.*
