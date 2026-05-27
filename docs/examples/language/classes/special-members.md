# Special member functions & RAII

> The six functions the compiler can generate for you, and the rules for when to
> write (or suppress) them. This is the heart of C++ resource management.

## The six special members

```cpp
struct S {
    S();                          // default constructor
    ~S();                         // destructor
    S(const S&);                  // copy constructor
    S& operator=(const S&);       // copy assignment
    S(S&&) noexcept;              // move constructor
    S& operator=(S&&) noexcept;   // move assignment
};
```

The compiler synthesizes these when you don't, following rules about which
declarations suppress which generations.

## RAII — the reason it all matters

A class acquires a resource in its constructor and releases it in its destructor.
Because destructors run deterministically at scope exit (even on exceptions),
resources can't leak:

```cpp
class File {
    std::FILE* f_;
public:
    explicit File(const char* path) : f_{std::fopen(path, "r")} {}
    ~File() { if (f_) std::fclose(f_); }          // always runs
    // ... (and you must decide copy/move — see Rule of Five)
};
```

This is why `unique_ptr`, `lock_guard`, `fstream`, and `vector` clean up
automatically.

## Rule of Zero / Three / Five

- **Rule of Zero (preferred):** if your members already manage their own resources
  (`std::string`, `std::vector`, `unique_ptr`), write **none** of the special
  members — the defaults compose correctly.
  ```cpp
  struct Widget {                    // no special members needed
      std::string name;
      std::vector<int> data;
      std::unique_ptr<Impl> impl;     // move-only, handled automatically
  };
  ```
- **Rule of Five:** if you manually manage a resource (raw pointer, handle) and
  thus need a custom destructor, you almost certainly need **all five** (dtor +
  copy ctor/assign + move ctor/assign) — or to `= delete`/`= default` them
  deliberately.
- **Rule of Three** is the pre-C++11 version (dtor + copy ctor + copy assign).

## default and delete

```cpp
struct Move {
    Move() = default;
    Move(const Move&) = delete;           // non-copyable
    Move(Move&&) = default;                // but movable
    Move& operator=(Move&&) = default;
};
```

## Construction details

```cpp
struct Point {
    int x, y;
    Point(int x, int y) : x{x}, y{y} {}    // member initializer list — initializes, not assigns
    explicit Point(int v) : Point{v, v} {} // delegating constructor (C++11)
};
```

Initialize members in the **member initializer list** (in declaration order), not
by assignment in the body — it's required for `const`/reference members and avoids
double-initialization.

## Gotchas

- **Prefer the Rule of Zero.** Reach for member types that already manage
  resources; hand-written special members are error-prone. Only write them for
  genuine resource wrappers.
- **Declaring a destructor (or copy op) suppresses move generation.** A class with
  a user destructor won't get implicit move members — it silently falls back to
  *copying*, a quiet performance bug. Re-declare the moves (`= default`) if you
  need them.
- **Member initializer list runs in *declaration* order**, not the order you write
  it — mismatches cause use-before-init bugs (enable `-Wreorder`).
- **Mark move operations `noexcept`.** Containers (e.g. `vector` reallocation)
  only move (instead of copy) elements when the move is `noexcept` — otherwise you
  silently get copies.
- **A copy/move that throws mid-way must leave the object valid** — see
  [exception safety](../exceptions/noexcept-and-safety.md).

## See also

- [The rule of three/five/zero — cppreference](https://en.cppreference.com/w/cpp/language/rule_of_three)
- [Constructors — cppreference](https://en.cppreference.com/w/cpp/language/constructor)
- [Destructors — cppreference](https://en.cppreference.com/w/cpp/language/destructor)
