# Function objects (`<functional>`)

> Calling, adapting and composing callables: `std::invoke`, `bind_front` /
> `bind_back`, `not_fn`, `mem_fn`, `std::ref`, and the standard operator
> functors.
> **Header:** `<functional>`

## Callable wrappers

`std::function`, `std::move_only_function` (C++23), `std::copyable_function`
and `std::function_ref` (C++26) have their own page, with a "which wrapper?"
table: [Callable wrappers](callable-wrappers.md).

## std::invoke — call anything uniformly (C++17)

One spelling to call free functions, function objects, **and** pointers-to-member
(which use different syntax otherwise):

```cpp body
struct S { int f(int x) const { return x + data; } int data; };
S s{1};
auto some_lambda = [](int a, int b) { return a * b; };
std::invoke(&S::f, s, 10);       // calls s.f(10)
std::invoke(&S::data, s);        // accesses s.data
std::invoke(some_lambda, 3, 4);  // calls the lambda
```

`std::invoke` is the foundation the library calls callables through (e.g.
algorithms, `std::apply`). `std::invoke_r<R>` (C++23) fixes the return type.

## ref / cref — pass references through value-copying APIs

```cpp body
int counter = 0;
auto inc = [](int& c){ ++c; };
std::invoke(inc, std::ref(counter));    // reference_wrapper → counter is modified
```

Many library facilities copy their arguments (e.g. `std::thread`, `std::bind`);
`std::ref`/`std::cref` wrap a reference so it survives the copy.

## Operator functors

Ready-made callables for algorithms and containers:

```cpp body
std::plus<>{}(2, 3);             // 5  (transparent: deduces operand types, C++14)
std::less<>{};                   // default comparator for std::sort, std::set, ...
std::greater<int>{};             // descending sort / min-heap
// also: minus, multiplies, divides, modulus, negate,
//       equal_to, not_equal_to, logical_and/or/not, bit_and/or/xor
```

## bind_front / bind_back (C++20 / C++23)

`std::bind_front(f, args...)` returns a callable that calls `f(args..., rest...)`;
`std::bind_back` (C++23) appends instead: `f(rest..., args...)`. Unlike
`std::bind` there are no placeholders, and the result perfectly forwards its
call arguments.

```cpp body
auto sub = [](int a, int b){ return a - b; };
auto from10  = std::bind_front(sub, 10);   // from10(3)  == sub(10, 3) == 7
auto minus10 = std::bind_back(sub, 10);    // minus10(3) == sub(3, 10) == -7

struct Counter { int n = 0; int add(int k){ return n += k; } };
Counter c;
auto add_to_c = std::bind_front(&Counter::add, &c);   // member function + object
add_to_c(5);                                           // c.n == 5
```

Bound arguments are stored by value (decayed); pass `std::ref(x)` to bind a
reference.

## not_fn (C++17)

`std::not_fn(f)` returns a callable that returns `!f(args...)`. It replaces the
deprecated (and in C++20 removed) `not1` / `not2`.

```cpp body
auto is_even = [](int x){ return x % 2 == 0; };
std::vector<int> xs{1, 2, 3, 4, 5};
auto odd = std::count_if(xs.begin(), xs.end(), std::not_fn(is_even));   // 3
```

## mem_fn (C++11)

`std::mem_fn(&T::member)` turns a pointer to member into a callable that takes
the object (by reference, pointer or smart pointer) as its first argument. A
data member gives back the member's value.

```cpp body
struct Person { std::string name; int age; bool adult() const { return age >= 18; } };
std::vector<Person> people{{"Ann", 30}, {"Bo", 10}};
auto adults = std::count_if(people.begin(), people.end(), std::mem_fn(&Person::adult));  // 1
auto age = std::mem_fn(&Person::age);
age(people[0]);     // 30: through a reference
age(&people[1]);    // 10: through a pointer
```

Algorithms and ranges already call through `std::invoke`, so
`std::ranges::count_if(people, &Person::adult)` needs no `mem_fn`.

## Constant-callable forms (C++26)

P2714 adds overloads that take the callable as a template argument:
`std::not_fn<f>()`, `std::bind_front<f>(args...)`, `std::bind_back<f>(args...)`.
The target isn't stored, so the result is as small as its bound arguments, and
the call isn't through a pointer.

```cpp needs=nttp_callables
#include <functional>

bool is_even(int x) { return x % 2 == 0; }
int  sub(int a, int b) { return a - b; }

auto odd     = std::not_fn<is_even>();     // odd(3) == true
auto from10  = std::bind_front<sub>(10);   // from10(3)  == 7; sizeof == sizeof(int)
auto minus10 = std::bind_back<sub>(10);    // minus10(3) == -7
```

GCC 16 has all three. Clang 22's libc++ has `not_fn<f>()` but not the
`bind_front<f>` overload, even though it defines `__cpp_lib_bind_front` as
202306L, and its `bind_back` is still the C++23 version (202202L). GCC 15 has
none.

## std::bind vs. lambdas

`std::bind` (C++11) pre-binds arguments with placeholders. Prefer a lambda, or
`bind_front` / `bind_back` when you're fixing the leading or trailing
arguments:

```cpp body
auto add = [](int a, int b){ return a + b; };
auto add5 = [add](int b){ return add(5, b); };        // prefer this
// std::bind equivalent (placeholders), kept mostly for legacy code:
auto add5b = std::bind(add, 5, std::placeholders::_1);
```

## Gotchas

- **`bind_front` copies its arguments.** `bind_front(&T::f, obj)` stores a copy
  of `obj`; pass `&obj` or `std::ref(obj)` to act on the original.
- **Don't trust `__cpp_lib_bind_front` alone on libc++ 22.** It says 202306L,
  but `bind_front<f>(...)` doesn't compile there.
- **Prefer lambdas to `std::bind`.** `bind`'s placeholders, nested binds, and
  argument-decay rules are error-prone; a lambda is explicit.
- **Dangling captures / refs.** `std::ref` and reference-capturing lambdas don't
  extend lifetimes — the referent must outlive every call.

## See also

- [Callable wrappers](callable-wrappers.md): `function`, `move_only_function`, `copyable_function`, `function_ref`
- [std::invoke — cppreference](https://en.cppreference.com/w/cpp/utility/functional/invoke)
- [std::ref / std::cref — cppreference](https://en.cppreference.com/w/cpp/utility/functional/ref)
- [std::bind_front, std::bind_back — cppreference](https://en.cppreference.com/w/cpp/utility/functional/bind_front)
- [std::not_fn — cppreference](https://en.cppreference.com/w/cpp/utility/functional/not_fn)
- [std::mem_fn — cppreference](https://en.cppreference.com/w/cpp/utility/functional/mem_fn)
- [P2714R1: Bind front and back to NTTP callables](https://wg21.link/P2714R1)
