# Members & access control

> Data and function members, static members, the access specifiers, and `friend`.

## Members

```cpp
class Account {
public:
    explicit Account(double balance) : balance_{balance} {}
    double balance() const { return balance_; }          // const member function
    void deposit(double amount) { balance_ += amount; }

    static int count();                                   // static member function
private:
    double balance_ = 0.0;                                // data member (in-class initializer)
    static inline int count_ = 0;                         // static data member (inline → defined here, C++17)
};
```

- **Non-static data members** belong to each object; initialize them with in-class
  initializers or the constructor's member-init list.
- **`static` members** belong to the class, not any object; one shared instance.
  `static inline` (C++17) lets you define them in the header.
- **`const` member functions** promise not to modify `*this` and are callable on
  `const` objects — mark every non-mutating method `const`.

## Access specifiers

```cpp
class C {
public:    // accessible to everyone
protected: // accessible to this class and derived classes
private:   // accessible only within this class (and friends)  ← class default
};
struct S { /* members public by default */ };
```

`class` defaults to `private`, `struct` to `public` — otherwise identical. Use
`struct` for plain aggregates, `class` for types with invariants.

## friend — selective access

```cpp
class Vault {
    int secret_;
    friend class Auditor;                       // Auditor may touch privates
    friend void inspect(const Vault&);          // so may this free function
    friend std::ostream& operator<<(std::ostream&, const Vault&);  // common for operator<<
};
```

`friend` grants access without making the member public — most often used for
non-member `operator<<`/`operator==` that need internals.

## this and ref-qualified members

```cpp
struct Builder {
    Builder& set(int) &;          // only callable on lvalues
    Builder  build() &&;          // only callable on rvalues (move out)
};
```

## Gotchas

- **Mark non-mutating methods `const`** and use in-class member initializers — the
  baseline of clean class design. A non-`const` getter can't be called on a
  `const` object.
- **`static` data members need a definition** (pre-C++17) outside the class, or
  use `static inline` (C++17) / `static constexpr` to define in-class.
- **Encapsulate invariants behind `private` data + public methods.** Public data
  members are fine only for true aggregates with no invariant.
- **`protected` is weaker than people think** — derived classes get access, which
  can break base-class invariants; prefer `private` + a protected interface.
- **A non-member `operator<<`/`==` often needs `friend`**, or accessor methods —
  member `operator<<` would put the stream on the wrong side.

## See also

- [Member access — cppreference](https://en.cppreference.com/w/cpp/language/access)
- [static members — cppreference](https://en.cppreference.com/w/cpp/language/static)
- [friend declaration — cppreference](https://en.cppreference.com/w/cpp/language/friend)
