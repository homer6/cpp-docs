# ES: Expressions and statements — examples

> Our examples for the [ES rules](../expr.md): declarations, expressions,
> statements and arithmetic, written so that mistakes are hard to make and,
> where possible, rejected by the compiler.

Several BAD examples below are rejected at compile time only because they are
evaluated in a constant expression (`constexpr` or `static_assert`): the
compiler must diagnose undefined behaviour there. The same code in an ordinary
function compiles and is undefined at run time.

## ES.1: Prefer the standard library to other libraries and to "handcrafted code"

The standard library is already written, tested and known to every reader.
A hand-written loop for a well-known algorithm is longer and hides what it
computes.

```cpp bad
double total(const std::vector<double>& prices) {
    double sum = 0;
    for (std::size_t i = 0; i != prices.size(); i++)   // BAD: a hand-made accumulate
        sum = sum + prices[i];
    return sum;
}
```

```cpp
double total(const std::vector<double>& prices) {
    return std::accumulate(prices.begin(), prices.end(), 0.0);   // GOOD: says what it does
}
```

## ES.2: Prefer suitable abstractions to direct use of language features

Code written with a fitting abstraction (a container, a string, a stream) is
shorter and closer to the problem than the same job done with raw arrays and
pointers, and it has fewer places to get wrong.

```cpp nocompile
// BAD: fixed limits, manual allocation, and the caller must delete[] every string
char** read_words(std::istream& in, int max_words, int max_len, int* count) {
    char** words = new char*[max_words];
    int n = 0;
    while (n < max_words) {
        char* w = new char[max_len];
        if (!(in >> std::setw(max_len) >> w)) { delete[] w; break; }
        words[n++] = w;
    }
    *count = n;
    return words;
}
```

```cpp
std::vector<std::string> read_words(std::istream& in) {
    std::vector<std::string> words;       // GOOD: grows as needed, frees itself
    for (std::string w; in >> w;)
        words.push_back(w);
    return words;
}
```

## ES.3: Don't repeat yourself, avoid redundant code

Duplicated code has to be read twice, and a later fix often reaches only one
copy. Pull the common part out so each step is written once.

```cpp nocompile
void save(bool compress) {
    if (compress) {
        open_file();          // BAD: the same steps in both branches
        write_compressed();
        close_file();
    } else {
        open_file();
        write_plain();
        close_file();
    }
}
```

```cpp
void open_file();
void close_file();
void write_compressed();
void write_plain();

void save(bool compress) {
    open_file();              // GOOD: only the part that differs is chosen
    if (compress)
        write_compressed();
    else
        write_plain();
    close_file();
}
```


## ES.5: Keep scopes small

A name that lives longer than it is needed can be misused later, and an object
that owns a resource holds it for the whole scope. Give each object the
smallest scope that does the job.

```cpp bad
std::mutex m;
std::vector<int> pending;

void drain() {
    std::lock_guard lock{m};               // BAD: held for the whole function
    auto items = std::exchange(pending, {});
    for (int x : items) process(x);        // slow work still runs under the lock
}
```

```cpp
std::mutex m;
std::vector<int> pending;

void drain() {
    auto items = [] {
        std::lock_guard lock{m};           // GOOD: the lock lives only here
        return std::exchange(pending, {});
    }();
    for (int x : items) process(x);
}
```

## ES.6: Declare names in for-statement initializers and conditions to limit scope

A variable declared in the `for`, `if` or `switch` header exists only for that
statement, so it can't be reused by mistake afterwards.

```cpp bad
void report(const std::map<std::string, int>& ages, const std::string& name) {
    auto it = ages.find(name);             // BAD: it stays in scope for the rest of the function
    if (it != ages.end())
        std::cout << it->second << '\n';
}
```

```cpp
void report(const std::map<std::string, int>& ages, const std::string& name) {
    if (auto it = ages.find(name); it != ages.end())   // GOOD: it exists only in the if
        std::cout << it->second << '\n';
}
```

## ES.7: Keep common and local names short, and keep uncommon and non-local names longer

Short conventional names (`i`, `v`, `x`) read best in a few lines of local
code. A name used far from its declaration needs enough words to be
understood there.

```cpp bad
std::string tt(std::string s);    // BAD: a public function with a cryptic name

void print(const std::vector<int>& the_vector_of_values) {   // BAD: long names for local, generic things
    for (std::size_t current_element_index = 0;
         current_element_index < the_vector_of_values.size(); ++current_element_index)
        std::cout << the_vector_of_values[current_element_index] << '\n';
}
```

```cpp
std::string trim_trailing_spaces(std::string s);   // GOOD: says what it does where it's called

void print(const std::vector<int>& v) {            // GOOD: short names in a short scope
    for (int x : v)
        std::cout << x << '\n';
}
```

## ES.8: Avoid similar-looking names

Names that differ by one easily confused character (`l` and `1`, `O` and `0`)
or a type and a variable with the same name invite mistakes. Here a function
named like a type hides the type.

```cpp compile-fail="does not name a type|must use 'struct' tag"
struct config { int level; };
config config();          // BAD: a function with the same name as the type
config current;           // error: config now names the function
```

```cpp
struct Config { int level; };
Config load_config();     // GOOD: distinct names for the type and the function
Config current = load_config();
```

## ES.9: Avoid `ALL_CAPS` names

`ALL_CAPS` is the conventional style for macros, so an ordinary name in capitals
can be silently replaced by a macro someone else defined.

```cpp compile-fail="expected identifier"
#define MAX 100                 // somewhere in a header
enum class Limit { MAX = 5 };   // BAD: MAX becomes 100 here, so this doesn't compile
```

```cpp
#define MAX 100
enum class Limit { max = 5 };   // GOOD: a lower-case enumerator can't collide with the macro
constexpr int max_items = 100;
```

## ES.10: Declare one name (only) per declaration

In `int* p, q;` the `*` belongs to `p` only, so `q` is an `int`. One name per
declaration avoids that trap and leaves room for a comment.

```cpp compile-fail="invalid conversion|incompatible pointer to integer"
int value = 1;
void f() {
    int* p, q;    // BAD: q is an int, not a pointer
    p = &value;
    q = &value;   // error
}
```

```cpp
int value = 1;
void f() {
    int* p = &value;   // GOOD: one name, one type, initialized
    int* q = &value;
    use(p, q);
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.11: Use `auto` to avoid redundant repetition of type names

When the initializer already says the type, repeating it adds nothing and can
be wrong. Here the spelled-out type is the non-`const` iterator, which a
`const` map doesn't give.

```cpp compile-fail="non-scalar type|no viable conversion"
int first(const std::map<std::string, int>& m) {
    std::map<std::string, int>::iterator it = m.begin();   // BAD: it's a const_iterator
    return it->second;
}
```

```cpp
int first(const std::map<std::string, int>& m) {
    auto it = m.begin();          // GOOD: the compiler picks const_iterator
    return it->second;
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.12: Do not reuse names in nested scopes

A name declared again in an inner block hides the outer one, and a later
assignment silently goes to the wrong variable.

```cpp bad
int score(const std::vector<int>& rounds) {
    int total = 0;
    for (int r : rounds) {
        int total = r * 2;   // BAD: a new total hides the outer one
        total += 1;          // changes the inner total; the outer stays 0
    }
    return total;
}
```

```cpp
int score(const std::vector<int>& rounds) {
    int total = 0;
    for (int r : rounds) {
        const int points = r * 2 + 1;   // GOOD: its own name
        total += points;
    }
    return total;
}
```

## ES.20: Always initialize an object

An uninitialized variable can be read before it is set; at run time that's
undefined behaviour (erroneous behaviour since C++26). In a constant
expression the compiler must reject the read.

```cpp compile-fail="not usable in a constant expression|read of uninitialized"
constexpr int count_items() {
    int n;        // BAD: no initial value
    return n;     // read before it's set
}
static_assert(count_items() == 0);
```

```cpp
constexpr int count_items() {
    int n = 0;    // GOOD
    return n;
}
static_assert(count_items() == 0);
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.21: Don't introduce a variable (or constant) before you need to use it

A variable declared far ahead of its first use widens its scope and makes the
reader carry it in mind for no reason.

```cpp nocompile
void ship(const Order& order) {
    const double tax = order.total * 0.05;   // BAD: not used until the end
    validate(order);
    reserve_stock(order);
    charge(order.total + tax);
}
```

```cpp
struct Order { double total; };
void validate(const Order&);
void reserve_stock(const Order&);
void charge(double);

void ship(const Order& order) {
    validate(order);
    reserve_stock(order);
    const double tax = order.total * 0.05;   // GOOD: declared where it's used
    charge(order.total + tax);
}
```

## ES.22: Don't declare a variable until you have a value to initialize it with

Declaring first and assigning later leaves a window where the variable holds
a meaningless value, and every path must remember to set it.

```cpp bad
std::string label(int errors) {
    std::string text;          // BAD: empty until some branch assigns it
    if (errors == 0) text = "ok";
    else text = "failed";
    return text;
}
```

```cpp
std::string label(int errors) {
    const std::string text = errors == 0 ? "ok" : "failed";   // GOOD: born with its value
    return text;
}
```

## ES.23: Prefer the `{}`-initializer syntax

`{}` works in every initialization context, refuses narrowing conversions and
can't be mistaken for a function declaration, which `()` can.

```cpp compile-fail="non-class type|base of member reference is a function"
void f() {
    Widget w();   // BAD: declares a function named w, not a Widget
    w.use();      // error
}
```

```cpp
void f(double ratio) {
    Widget w{};                              // GOOD: a value-initialized Widget
    w.use();
    int percent{static_cast<int>(ratio * 100)};   // narrowing must be written out
    use(percent);
}
```

Containers are the known exception: `std::vector<int>(3)` has three elements,
`std::vector<int>{3}` has one.

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.24: Use a `unique_ptr<T>` to hold pointers

A raw owning pointer leaks on every early return or exception that skips its
`delete`. `std::unique_ptr` frees the object on every path at no extra cost.

```cpp bad
void load(bool cancel) {
    int* buffer = new int[256]{};   // BAD: owned by a raw pointer
    if (cancel) return;             // leak
    process(buffer);                // leak if this throws
    delete[] buffer;
}
```

```cpp
void load(bool cancel) {
    auto buffer = std::make_unique<int[]>(256);   // GOOD: freed on every path
    if (cancel) return;
    process(buffer.get());
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.25: Declare an object `const` or `constexpr` unless you want to modify its value later on

A `const` object can't be changed by mistake, and it tells the reader the value
is fixed. Leave it non-`const` only when you mean to change it.

```cpp bad
int buffer_size(int lines) {
    int width = 80;                 // BAD: never changes, but nothing says so
    int size = lines * width + 1;
    return size;
}
```

```cpp
int buffer_size(int lines) {
    constexpr int width = 80;       // GOOD: known at compile time
    const int size = lines * width + 1;
    return size;
}
```

## ES.26: Don't use a variable for two unrelated purposes

Reusing one variable for a second meaning makes both uses harder to follow,
and leftover state from the first use leaks into the second.

```cpp bad
void greet(const std::string& user) {
    std::string s = "Hello, " + user;   // s is a greeting...
    use(s);
    s = "/home/" + user;                 // BAD: ...and now a path
    use(s);
}
```

```cpp
void greet(const std::string& user) {
    const std::string greeting = "Hello, " + user;   // GOOD: one name per meaning
    use(greeting);
    const std::string home = "/home/" + user;
    use(home);
}
```

## ES.27: Use `std::array` or `stack_array` for arrays on the stack

`std::array` keeps its size and doesn't decay to a pointer. An array whose
bound is a run-time value (a variable-length array) isn't standard C++ at all.
`stack_array` is a proposed GSL type; for a run-time size, `std::vector` is the
standard choice.

```cpp compile-fail="variable length array"
void fill(int n) {
    int values[n];   // BAD: a VLA, a compiler extension
    values[0] = 1;
}
```

```cpp
constexpr int slots = 8;
void fill(int n) {
    std::array<int, slots> fixed{};   // GOOD: size known, no decay
    std::vector<int> values(n);       // GOOD: size known only at run time
    fixed[0] = values.empty() ? 0 : 1;
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.28: Use lambdas for complex initialization, especially of `const` variables

When a value takes several statements to compute, an immediately invoked
lambda keeps the scratch variables inside it and lets the result be `const`.

```cpp bad
void plan(const std::vector<int>& sizes) {
    int largest_even = 0;              // BAD: should be const once computed, but can't be
    for (int s : sizes)
        if (s % 2 == 0 && s > largest_even) largest_even = s;
    use(largest_even);
}
```

```cpp
void plan(const std::vector<int>& sizes) {
    const int largest_even = [&] {     // GOOD: computed once, then fixed
        int best = 0;
        for (int s : sizes)
            if (s % 2 == 0 && s > best) best = s;
        return best;
    }();
    use(largest_even);
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.30: Don't use macros for program text manipulation

A macro rewrites text before the compiler sees it, ignoring scopes and types,
so what you read isn't what gets compiled.

```cpp bad
#define Case break; case       // BAD: hides a break inside a "case"
#define NAME_OF(x) #x          // BAD: text tricks tools can't follow

int f(int n) {
    switch (n) {
    case 0: return 10;
    Case 1: return 20;         // writing "case" instead of "Case" silently changes the flow
    }
    return 0;
}
```

```cpp
enum class Color { red, green };

constexpr std::string_view name_of(Color c) {   // GOOD: typed, scoped, visible to tools
    switch (c) {
    case Color::red: return "red";
    case Color::green: return "green";
    }
    return "?";
}
```

## ES.31: Don't use macros for constants or "functions"

A function-like macro pastes its argument's text, so precedence and repeated
evaluation surprise the caller. A `constexpr` variable or function obeys the
normal rules and costs nothing at run time.

```cpp bad
#define PI 3.14159
#define SQUARE(x) x * x       // BAD: SQUARE(1 + 2) is 1 + 2 * 1 + 2, which is 5
```

```cpp
constexpr double pi = 3.14159;
constexpr int square(int x) { return x * x; }   // GOOD: square(1 + 2) is 9
static_assert(square(1 + 2) == 9);
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.32: Use `ALL_CAPS` for all macro names

If you must have a macro, an `ALL_CAPS` name warns readers that it's a macro.
A lower-case macro looks like an ordinary name and replaces ordinary names.

```cpp compile-fail="expected unqualified-id"
#define max(a, b) ((a) > (b) ? (a) : (b))   // BAD: a lower-case macro
int biggest = std::max(3, 4);               // error: std::max is rewritten by the macro
```

```cpp
#ifndef MYLIB_ENABLE_TRACE
#define MYLIB_ENABLE_TRACE 0                // GOOD: visibly a macro, used only for configuration
#endif
int biggest = std::max(3, 4);
```

## ES.33: If you must use macros, give them unique names

Macros have no scope, so a short, common macro name will one day replace a name
in someone else's code. Prefix it with something only your project uses.

```cpp compile-fail="expected unqualified-id|expected member name"
#define VERSION 2                       // BAD: from one library's header
struct Config { int VERSION = 1; };     // another library's member becomes "int 2 = 1;"
```

```cpp
#define ACME_NETLIB_VERSION 2           // GOOD: prefixed, unlikely to clash
struct Config { int VERSION = 1; };
```

## ES.34: Don't define a (C-style) variadic function

A `...` function gets no type checking: the callee guesses each argument's type
and where the list ends. A variadic template knows every type at compile time.

```cpp bad
#include <cstdarg>
void log_all(int count, ...) {        // BAD: trusts the caller for count and types
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; ++i)
        std::fputs(va_arg(args, const char*), stderr);
    va_end(args);
}
// log_all(2, "a", std::string{"b"});  // undefined behaviour: a std::string isn't a const char*
```

```cpp
template <class... Args>
std::string join(const Args&... args) {   // GOOD: every argument's type is checked
    std::string out;
    ((out += args), ...);
    return out;
}
std::string line = join("a", std::string{"b"}, 'c');
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).


## ES.40: Avoid complicated expressions

An expression that assigns inside sub-expressions, mixes many operators or
leans on subtle rules is easy to misread and easy to get wrong.

```cpp nocompile
// BAD: two assignments hidden in the condition of a loop
while ((n = read_next()) >= 0 && (total += n) < limit) {}
```

```cpp body
int total = 0;
const int limit = 100;
for (int n : {10, 20, 30}) {      // GOOD: one step per statement
    if (total + n >= limit) break;
    total += n;
}
```

## ES.41: If in doubt about operator precedence, parenthesize

`==` and `!=` bind tighter than `&`, `|` and `^`, so a bit test without
parentheses compares first and masks afterwards.

```cpp bad
bool is_set(unsigned flags, unsigned mask) {
    return flags & mask != 0;     // BAD: means flags & (mask != 0)
}
```

```cpp
bool is_set(unsigned flags, unsigned mask) {
    return (flags & mask) != 0;   // GOOD: mask first, then compare
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.42: Keep use of pointers simple and straightforward

Pointer arithmetic and subscripts on a bare pointer can't be checked: nothing
knows how many elements there are. Pass a `std::span`, which carries its size.

```cpp bad
void clear(int* p, int n) {
    for (int i = 0; i <= n; ++i)  // BAD: writes one element past the end
        *(p + i) = 0;
}
```

```cpp
void clear(std::span<int> values) {
    for (int& v : values)          // GOOD: the size travels with the data
        v = 0;
}
void clear_tail(std::span<int> values) {
    if (values.size() > 1) clear(values.subspan(1));
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.43: Avoid expressions with undefined order of evaluation

If an expression changes a variable and reads it again where the two aren't
sequenced, the behaviour is undefined. C++17 sequenced more operators, but not
all of them (`+`, for one), so don't read a variable in the same expression
that writes it.

```cpp bad
int next_pair(int i) {
    return i++ + i;   // BAD: undefined: the read of i isn't sequenced with i++
}
```

```cpp
int next_pair(int i) {
    const int first = i;   // GOOD: each step in its own statement
    ++i;
    return first + i;
}
```

## ES.44: Don't depend on order of evaluation of function arguments

Function arguments are evaluated in an unspecified order, so two arguments
with side effects can run either way round. Evaluate them in separate
statements first. (A braced initializer list, and since C++17 `<<` chains, are
evaluated left to right.)

```cpp bad
int next_id();
// BAD: which call gets the smaller id is unspecified
auto ids = std::make_pair(next_id(), next_id());
```

```cpp
int next_id();
std::pair<int, int> two_ids() {
    const int first = next_id();    // GOOD: the order is written down
    const int second = next_id();
    return {first, second};
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.45: Avoid "magic constants"; use symbolic constants

A bare number in an expression says nothing about where it came from, and
changing it means finding every copy. Name it once.

```cpp bad
double shipping(double kg) {
    return kg > 20 ? kg * 1.75 : 4.99;   // BAD: what are 20, 1.75 and 4.99?
}
```

```cpp
constexpr double free_weight_limit_kg = 20;
constexpr double rate_per_kg = 1.75;
constexpr double flat_fee = 4.99;

double shipping(double kg) {
    return kg > free_weight_limit_kg ? kg * rate_per_kg : flat_fee;   // GOOD
}
```

## ES.46: Avoid lossy (narrowing, truncating) arithmetic conversions

An implicit conversion to a smaller or less precise type drops information
without a word. Brace initialization turns such a conversion into an error,
and a deliberate one should be explicit and checked.

```cpp compile-fail="narrowing|cannot be narrowed"
void store(int code) {
    char c{code};          // BAD: int to char can lose the value; braces reject it
    use(c);
}
```

```cpp
#include <stdexcept>
char to_char(int code) {
    if (code < std::numeric_limits<char>::min() || code > std::numeric_limits<char>::max())
        throw std::out_of_range{"code"};
    return static_cast<char>(code);   // GOOD: checked first, then explicit
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.47: Use `nullptr` rather than `0` or `NULL`

`0` is an `int` first and a null pointer only by conversion, and `NULL` is an
integer constant too, so overload resolution can pick the integer overload.
`nullptr` has its own type, which converts only to pointers.

```cpp bad
void notify(int user_id);
void notify(const char* message);
void reset() { notify(0); }   // BAD: calls notify(int), not the pointer overload
```

```cpp
void notify(int user_id);
void notify(const char* message);
void reset() { notify(nullptr); }   // GOOD: calls notify(const char*)
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.48: Avoid casts

A cast tells the compiler to stop checking. Reading a `double`'s bytes through
an integer pointer is undefined behaviour; `std::bit_cast` copies the bits with
the types checked. To discard a `[[nodiscard]]` result, assign it to
`std::ignore` instead of casting to `void`.

```cpp bad
std::uint64_t bits_of(double d) {
    return *(std::uint64_t*)&d;   // BAD: undefined: reads a double through the wrong type
}
```

```cpp
#include <bit>
std::uint64_t bits_of(double d) {
    return std::bit_cast<std::uint64_t>(d);   // GOOD: sizes checked, no aliasing
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.49: If you must use a cast, use a named cast

A C-style cast does whatever conversion makes the code compile, even between
unrelated classes. `static_cast` and the other named casts do one kind of
conversion each, so the compiler rejects the wrong kind.

```cpp bad
struct Account {};
struct Report {};             // used to derive from Account, not any more
Report* as_report(Account* a) {
    return (Report*)a;        // BAD: still compiles, now a reinterpretation
}
```

```cpp
struct Account { virtual ~Account() = default; };
struct Savings : Account {};
Savings* as_savings(Account* a) {
    return dynamic_cast<Savings*>(a);   // GOOD: checked; static_cast would refuse unrelated types
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.50: Don't cast away `const`

Writing through a `const_cast` breaks the promise `const` made, and if the
object itself was declared `const` the write is undefined behaviour. For a
cache inside a `const` member function, use `mutable`.

```cpp bad
struct Prices {
    std::map<int, double> cache;
    double lookup(int id) const {
        auto& c = const_cast<std::map<int, double>&>(cache);   // BAD: lies about const
        return c[id] = id * 1.5;
    }
};
```

```cpp
struct Prices {
    mutable std::map<int, double> cache;   // GOOD: the cache isn't part of the value
    double lookup(int id) const {
        auto [it, added] = cache.try_emplace(id, id * 1.5);
        return it->second;
    }
};
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.55: Avoid the need for range checking

Constructs that can't go out of range (range-`for`, algorithms) need no index
checks at all, and are usually as fast or faster.

```cpp bad
bool contains(const std::vector<int>& v, int x) {
    for (std::size_t i = 0; i < v.size() + 1; ++i)    // BAD: the bound is easy to get wrong
        if (i < v.size() && v[i] == x) return true;  // and needs a check to stay in range
    return false;
}
```

```cpp
bool contains(const std::vector<int>& v, int x) {
    return std::ranges::find(v, x) != v.end();       // GOOD: nothing to check
}
```

## ES.56: Write `std::move()` only when you need to explicitly move an object to another scope

Returning a local and using a function's result already move implicitly; an
extra `std::move` adds noise and can block copy elision. Write it when you
hand a named object to a sink, and never on a `const` object, where it
silently copies.

```cpp bad
std::vector<int> make() {
    std::vector<int> result{1, 2, 3};
    return std::move(result);                  // BAD: prevents copy elision
}
std::vector<int> v = std::move(make());        // BAD: the result is already an rvalue
```

```cpp
std::vector<int> make() {
    std::vector<int> result{1, 2, 3};
    return result;                             // GOOD: moved or elided automatically
}
void sink(std::unique_ptr<Widget> w);
void hand_over() {
    auto w = std::make_unique<Widget>();
    sink(std::move(w));                        // GOOD: a named object leaves this scope
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.60: Avoid `new` and `delete` outside resource management functions

A naked `new` in application code needs a matching `delete` on every path,
including exceptions. Let a container or smart pointer own the memory.

```cpp bad
void render(int n) {
    Widget* items = new Widget[n];   // BAD: a throw below skips the delete[]
    for (int i = 0; i < n; ++i) items[i].run();
    delete[] items;
}
```

```cpp
void render(int n) {
    std::vector<Widget> items(n);    // GOOD: freed on every path
    for (const auto& w : items) w.run();
}
```

## ES.61: Delete arrays using `delete[]` and non-arrays using `delete`

Memory from `new T[n]` must be released with `delete[]` and memory from
`new T` with `delete`; a mismatch is undefined behaviour. During constant
evaluation the compiler catches it.

```cpp compile-fail="non-array (deallocation|delete)"
constexpr int sum3() {
    int* p = new int[3]{1, 2, 3};
    int s = p[0] + p[1] + p[2];
    delete p;      // BAD: allocated with new[], so it needs delete[]
    return s;
}
static_assert(sum3() == 6);
```

```cpp
constexpr int sum3() {
    std::vector<int> v{1, 2, 3};   // GOOD: no new or delete to match up
    return v[0] + v[1] + v[2];
}
static_assert(sum3() == 6);
```

## ES.62: Don't compare pointers into different arrays

`<` on pointers to different arrays has no meaningful result (it's unspecified),
and subtracting them is undefined. In a constant expression the compiler
refuses. `std::less` does give a consistent total order when you need one.

```cpp compile-fail="is not a constant expression|unrelated objects"
int first[4];
int second[4];
constexpr bool before = &first[1] < &second[1];   // BAD: different arrays
```

```cpp
int buffer[8];
constexpr bool before = &buffer[1] < &buffer[5];   // GOOD: same array
static_assert(before);
bool ordered(const int* a, const int* b) {
    return std::less<const int*>{}(a, b);          // GOOD: a total order for any pointers
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.63: Don't slice

Copying a derived object into a base object keeps only the base part: the
derived data and the derived overrides are lost. Use references or pointers to
the base, and make a polymorphic base non-copyable so slicing doesn't compile.

```cpp bad
struct Shape { virtual ~Shape() = default; virtual double area() const { return 0; } };
struct Square : Shape { double side = 2; double area() const override { return side * side; } };

double area_of(Shape s) { return s.area(); }   // BAD: takes a copy of the Shape part only
// area_of(Square{}) returns 0, not 4
```

```cpp
struct Shape {
    Shape() = default;
    Shape(const Shape&) = delete;              // GOOD: can't be sliced by copying
    Shape& operator=(const Shape&) = delete;
    virtual ~Shape() = default;
    virtual double area() const = 0;
};
struct Square : Shape { double side = 2; double area() const override { return side * side; } };

double area_of(const Shape& s) { return s.area(); }   // GOOD: no copy
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.64: Use the `T{e}`notation for construction

`T{e}` constructs a `T` and refuses narrowing. `T(e)` and `(T)e` are casts that
also allow truncation and even pointer-to-integer conversions.

```cpp bad
int whole(double d) { return int(d); }   // BAD: silently truncates 7.9 to 7
```

```cpp compile-fail="narrowing|cannot be narrowed"
int whole(double d) { return int{d}; }   // T{e} rejects the lossy conversion
```

```cpp
int whole(double d) { return static_cast<int>(std::lround(d)); }   // GOOD: the rounding is chosen and visible
std::int64_t widen(int i) { return std::int64_t{i}; }              // GOOD: no information lost
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.65: Don't dereference an invalid pointer

Using a pointer whose object has gone (out of scope, deleted, or moved by a
reallocating container) is undefined behaviour. In a constant expression the
compiler must reject it.

```cpp compile-fail="outside its lifetime"
constexpr int latest() {
    int* p = nullptr;
    {
        int value = 42;
        p = &value;
    }              // value is gone
    return *p;     // BAD: dangling
}
static_assert(latest() == 42);
```

```cpp
constexpr int latest() {
    int value = 42;   // GOOD: lives as long as the pointer is used
    int* p = &value;
    return *p;
}
static_assert(latest() == 42);
```


## ES.70: Prefer a `switch`-statement to an `if`-statement when there is a choice

A `switch` on constants is easier to read than an `if` chain, usually compiles
better, and lets the compiler warn when an enumerator is missing.

```cpp bad
enum class Level { low, mid, high };
int fan_speed(Level l) {
    if (l == Level::low) return 1;          // BAD: a chain of comparisons with constants
    else if (l == Level::mid) return 2;
    else return 3;
}
```

```cpp
enum class Level { low, mid, high };
int fan_speed(Level l) {
    switch (l) {                            // GOOD: -Wswitch flags a missing enumerator
    case Level::low: return 1;
    case Level::mid: return 2;
    case Level::high: return 3;
    }
    return 0;
}
```

## ES.71: Prefer a range-`for`-statement to a `for`-statement when there is a choice

When a loop just visits each element, range-`for` removes the index and its
off-by-one and signed/unsigned mistakes. Bind by `const&` to avoid copies.

```cpp bad
void print(const std::vector<std::string>& names) {
    for (int i = 0; i < names.size(); ++i)   // BAD: an index nobody needs
        std::cout << names[i] << '\n';
}
```

```cpp
void print(const std::vector<std::string>& names) {
    for (const auto& name : names)           // GOOD: no index, no copies
        std::cout << name << '\n';
}
```

## ES.72: Prefer a `for`-statement to a `while`-statement when there is an obvious loop variable

A `for` header shows the start, the end and the step together, and keeps the
loop variable inside the loop.

```cpp bad body
int i = 0;
while (i < 10) {    // BAD: start, test and step are spread out
    use(i);
    ++i;
}
```

```cpp body
for (int i = 0; i < 10; ++i)   // GOOD: the whole loop logic in one line
    use(i);
```

## ES.73: Prefer a `while`-statement to a `for`-statement when there is no obvious loop variable

If the loop is controlled by a condition rather than a counter, a `for` header
suggests a loop variable that doesn't drive the loop.

```cpp nocompile
int handled = 0;
for (; poll_event(); ++handled) {}   // BAD: handled has nothing to do with stopping
```

```cpp
bool poll_event();
int run() {
    int handled = 0;
    while (poll_event())   // GOOD: the condition is the whole story
        ++handled;
    return handled;
}
```

## ES.74: Prefer to declare a loop variable in the initializer part of a `for`-statement

A loop variable declared before the `for` outlives the loop and can be used by
mistake afterwards (see also ES.6).

```cpp nocompile
std::size_t i = 0;
for (; i < names.size(); ++i) use(names[i]);   // BAD: i is still around after the loop
```

```cpp
void show(const std::vector<std::string>& names) {
    for (std::size_t i = 0; i < names.size(); ++i)   // GOOD: i ends with the loop
        std::cout << i << ": " << names[i] << '\n';
}
```

## ES.75: Avoid `do`-statements

A `do` loop runs its body before checking anything and hides its condition at
the bottom, which is how a sentinel value gets processed by mistake.

```cpp nocompile
std::string line;
do {
    line = next_line();
    handle(line);           // BAD: also handles the empty line that ends the input
} while (!line.empty());
```

```cpp
std::string next_line();
void handle(const std::string&);
void run() {
    for (auto line = next_line(); !line.empty(); line = next_line())   // GOOD: checked first
        handle(line);
}
```

## ES.76: Avoid `goto`

`goto` jumps make the flow hard to follow; the C "goto cleanup" idiom is a
hand-made destructor. A `goto` that jumps over an initialization doesn't even
compile. Let an RAII handle do the cleanup.

```cpp compile-fail="jump to label|cannot jump"
int read_version(const char* path) {
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return -1;
    if (std::fgetc(f) == EOF) goto cleanup;   // BAD: manual cleanup by jumping
    int version = std::fgetc(f);              // error: the jump skips this initialization
    std::fclose(f);
    return version;
cleanup:
    std::fclose(f);
    return -1;
}
```

```cpp
struct file_closer { void operator()(std::FILE* f) const { std::fclose(f); } };

int read_version(const char* path) {
    std::unique_ptr<std::FILE, file_closer> f{std::fopen(path, "rb")};
    if (!f || std::fgetc(f.get()) == EOF) return -1;   // GOOD: closed on every path
    return std::fgetc(f.get());
}
```

## ES.77: Minimize the use of `break` and `continue` in loops

Each `break` or `continue` is an extra exit to spot in the loop body. A loop
that searches is often a named algorithm, and a run of `continue`s is often
one condition.

```cpp nocompile
for (int n : values) {
    if (n < 0) continue;      // BAD: three ways to skip, easy to miss one
    if (n % 2 != 0) continue;
    if (n > 100) continue;
    use(n);
}
```

```cpp
void use_small_evens(const std::vector<int>& values) {
    for (int n : values)
        if (n >= 0 && n % 2 == 0 && n <= 100)   // GOOD: one condition, read once
            use(n);
}
std::optional<int> first_negative(const std::vector<int>& values) {
    auto it = std::ranges::find_if(values, [](int n) { return n < 0; });   // GOOD: no break
    return it == values.end() ? std::nullopt : std::optional{*it};
}
```

## ES.78: Don't rely on implicit fallthrough in `switch` statements

A non-empty `case` without `break` runs into the next one, which is usually a
forgotten `break`. Mark a deliberate fallthrough with `[[fallthrough]]`.

```cpp nocompile
switch (level) {
case Level::warning:
    log_warning();     // BAD: falls into the error case; was that intended?
case Level::error:
    show_error();
    break;
}
```

```cpp
enum class Level { info, warning, error };
void log_warning();
void show_error();

void report(Level level) {
    switch (level) {
    case Level::info:
        break;
    case Level::warning:
        log_warning();
        [[fallthrough]];   // GOOD: deliberate and visible
    case Level::error:
        show_error();
        break;
    }
}
```

## ES.79: Use `default` to handle common cases (only)

Use `default` for what all the other values share. If there is nothing to do
for them, write an empty `default` with a comment; otherwise a reader (and the
compiler) will assume every case was meant to be listed.

```cpp nocompile
enum class Key { up, down, left, right };
void move(Key k) {
    switch (k) {       // BAD: left and right are missing; forgotten or deliberate?
    case Key::up: step(-1); break;
    case Key::down: step(+1); break;
    }
}
```

```cpp
enum class Key { up, down, left, right };
void step(int);

void move(Key k) {
    switch (k) {
    case Key::up: step(-1); break;
    case Key::down: step(+1); break;
    default: break;    // GOOD: horizontal keys do nothing here, on purpose
    }
}
```

## ES.84: Don't try to declare a local variable with no name

There is no unnamed local variable: `T{args};` creates a temporary that is
destroyed at the end of the statement. For a lock guard, that means the lock is
released at once.

```cpp bad
std::mutex m;
int counter = 0;
void bump() {
    std::lock_guard<std::mutex>{m};   // BAD: locks and unlocks right here
    ++counter;                        // not protected: a data race
}
```

```cpp
std::mutex m;
int counter = 0;
void bump() {
    std::lock_guard lock{m};          // GOOD: named, held until the end of the scope
    ++counter;
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.85: Make empty statements visible

A lone `;` after a loop header is easy to miss, and the next line then looks
like the loop body when it isn't.

```cpp nocompile
std::size_t i = 0;
for (; i < s.size() && s[i] == ' '; ++i);   // BAD: the ; is the body
s.erase(0, i);
```

```cpp
std::string trim_front(std::string s) {
    std::size_t i = 0;
    while (i < s.size() && s[i] == ' ') {
        ++i;                                  // GOOD: the body is visible
    }
    s.erase(0, i);
    return s;
}
```

## ES.86: Avoid modifying loop control variables inside the body of raw for-loops

The `for` header should tell the whole story of the loop variable. Changing it
in the body as well makes the iteration count hard to reason about.

```cpp nocompile
for (std::size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "-o")
        output = args[++i];   // BAD: i also changes here, and can run past the end
}
```

```cpp
std::string output_of(const std::vector<std::string>& args) {
    std::string output;
    bool next_is_output = false;            // GOOD: a second variable for a second idea
    for (const auto& arg : args) {
        if (next_is_output) { output = arg; next_is_output = false; }
        else if (arg == "-o") next_is_output = true;
    }
    return output;
}
```

## ES.87: Don't add redundant `==` or `!=` to conditions

A condition already tests for `true`, a non-null pointer or a non-zero value,
so `!= nullptr` or `== true` only adds words. An integer with more than two
meaningful values is the exception: compare it explicitly.

```cpp nocompile
if (ptr != nullptr && ready == true) start();   // BAD: redundant comparisons
```

```cpp
void start();
void maybe_start(const Widget* ptr, bool ready) {
    if (ptr && ready) start();                  // GOOD
}
```


## ES.100: Don't mix signed and unsigned arithmetic

When an `int` meets an `unsigned` in an operation, the `int` is converted to
`unsigned`, so a negative value becomes a huge one and comparisons go wrong.

```cpp nocompile
int balance = -3;
unsigned fee = 7;
auto left = balance - fee;           // BAD: unsigned, 4294967286 with a 32-bit unsigned
bool smaller = -1 < sizes.size();    // BAD: false: -1 becomes the largest size_t
```

```cpp
bool fits(int offset, const std::vector<int>& v) {
    return std::cmp_less(offset, v.size());   // GOOD: compares the mathematical values
}
std::ptrdiff_t last_index(const std::vector<int>& v) {
    return std::ssize(v) - 1;                 // GOOD: signed throughout; -1 for an empty vector
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.101: Use unsigned types for bit manipulation

On an unsigned type every bit is a value bit, so shifts and masks do what the
bit pattern says. A signed right shift copies the sign bit in, and small
unsigned types are promoted to `int` before `~`.

```cpp bad
int top_nibble(int word) {
    return word >> 28;               // BAD: for a negative word the sign bit is copied in: -1, not 15
}
```

```cpp
std::uint32_t top_nibble(std::uint32_t word) {
    return word >> 28;               // GOOD: zeros shift in
}
std::uint8_t invert(std::uint8_t b) {
    return static_cast<std::uint8_t>(~b);   // ~ works on the promoted int, so convert back
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.102: Use signed types for arithmetic

People expect `a - b` to be negative when `b > a`. With unsigned types the
result wraps around to a large positive value instead. Use unsigned only when
you really want arithmetic modulo 2ⁿ, and say so.

```cpp bad
unsigned in_stock = 5;
unsigned ordered = 7;
auto shortfall = in_stock - ordered;   // BAD: wraps to 4294967294, not -2
```

```cpp
int shortfall(int in_stock, int ordered) {
    return in_stock - ordered;         // GOOD: -2 when 7 are ordered and 5 are in stock
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.103: Don't overflow

Signed overflow is undefined behaviour, and an index past the end of an array
corrupts memory. Check before the operation, or use a wider type. In a constant
expression the compiler rejects an overflow.

```cpp compile-fail="overflow in constant expression|outside the range of representable"
constexpr int area(int w, int h) { return w * h; }
constexpr int big = area(100'000, 100'000);   // BAD: 10^10 doesn't fit in an int
```

```cpp
constexpr std::int64_t area(std::int32_t w, std::int32_t h) {
    return std::int64_t{w} * h;                // GOOD: computed in a type wide enough
}
constexpr auto big = area(100'000, 100'000);
static_assert(big == 10'000'000'000);

constexpr std::optional<int> checked_add(int a, int b) {
    if (b > 0 ? a > std::numeric_limits<int>::max() - b
              : a < std::numeric_limits<int>::min() - b)
        return std::nullopt;                   // GOOD: checked before it can overflow
    return a + b;
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.104: Don't underflow

Going below the minimum is the same mistake from the other side: an index
below zero, or a signed value decremented past its minimum, is undefined
behaviour.

```cpp compile-fail="outside the bounds|cannot refer to element -1"
constexpr int before_first() {
    int a[3]{1, 2, 3};
    int i = 0;
    return a[i - 1];   // BAD: index -1
}
static_assert(before_first() == 0);
```

```cpp
constexpr int sum_backwards() {
    int a[3]{1, 2, 3};
    int sum = 0;
    for (int i = 2; i >= 0; --i)   // GOOD: stops at 0, never reads a[-1]
        sum += a[i];
    return sum;
}
static_assert(sum_backwards() == 6);
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.105: Don't divide by integer zero

Integer division or remainder by zero is undefined behaviour and usually a
crash. Check the divisor first (or state it as a precondition).

```cpp compile-fail="division by zero|/ 0\)' is not a constant"
constexpr int per_person(int total, int people) { return total / people; }
static_assert(per_person(10, 0) == 0);   // BAD: divides by zero
```

```cpp
constexpr std::optional<int> per_person(int total, int people) {
    if (people == 0) return std::nullopt;   // GOOD: the zero case is handled
    return total / people;
}
static_assert(!per_person(10, 0));
static_assert(*per_person(10, 2) == 5);
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.106: Don't try to avoid negative values by using `unsigned`

An `unsigned` parameter doesn't stop a negative number: it converts it to a
huge positive one without complaint. Take a signed value and check it.

```cpp bad
unsigned area(unsigned w, unsigned h) { return w * h; }
int height = -2;             // e.g. from input
auto a = area(height, 2);    // BAD: -2 becomes 4294967294; a is 4294967292
```

```cpp
#include <stdexcept>
int area(int w, int h) {
    if (w < 0 || h < 0) throw std::invalid_argument{"negative side"};   // GOOD: caught
    return w * h;
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).

## ES.107: Don't use `unsigned` for subscripts, prefer `gsl::index`

An unsigned index can't go below zero, so a loop that counts down to `0` never
ends, and `size() - 1` on an empty container wraps. Use a signed index of
pointer size (`gsl::index` is `std::ptrdiff_t`) and `std::ssize`.

```cpp nocompile
for (auto i = v.size() - 1; i >= 0; --i)   // BAD: i >= 0 is always true; wraps on an empty v
    use(v[i]);
```

```cpp
void print_reversed(const std::vector<int>& v) {
    for (std::ptrdiff_t i = std::ssize(v) - 1; i >= 0; --i)   // GOOD: stops, even for an empty v
        std::cout << v[i] << '\n';
}
```

Tested by [`tests/guidelines/expr_test.cpp`](../../../../tests/guidelines/expr_test.cpp).
