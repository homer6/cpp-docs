# Type aliases (`using` / `typedef`)

> Give a type a new name — for readability, abstraction, or templating. Prefer
> `using` over `typedef`.

## using vs typedef

```cpp
using Bytes = std::vector<std::uint8_t>;          // preferred
typedef std::vector<std::uint8_t> Bytes;           // equivalent, older spelling

using Callback = int(*)(int, int);                 // function-pointer alias — readable!
typedef int (*Callback)(int, int);                  // ...the typedef version is cryptic
```

`using` reads left-to-right (`name = type`) and — crucially — can be templated;
`typedef` cannot.

## Alias templates (only `using`)

```cpp
template<class T>
using Vec = std::vector<T>;          // a parameterized alias

Vec<int> v;                           // == std::vector<int>

template<class K, class V>
using Dict = std::unordered_map<K, V>;
```

This is why the standard library exposes `_t` helpers as alias templates
(`std::remove_cvref_t<T>` = `typename std::remove_cvref<T>::type`).

## Member type aliases

Types expose their associated types via member aliases — the basis of generic
code:

```cpp
struct MyContainer {
    using value_type = int;
    using iterator   = int*;
};

template<class C>
typename C::value_type first(const C& c);    // generic code reads C::value_type
```

## Gotchas

- **Prefer `using` to `typedef`** — clearer syntax, and it's the only one that can
  be templated. There's no reason to write new `typedef`s.
- **An alias is not a new type** — it's a synonym. `using Meters = double;` does
  *not* prevent passing a plain `double`; for a distinct type use a struct/enum
  (a "strong typedef").
- **Templated aliases aren't deduced like classes.** An alias template can't be
  partially specialized, and CTAD through aliases has limits (improved in C++20).
- **`typename` is needed for dependent member aliases** in templates
  (`typename C::value_type`).

## See also

- [Type alias / `using` — cppreference](https://en.cppreference.com/w/cpp/language/type_alias)
- [typedef — cppreference](https://en.cppreference.com/w/cpp/language/typedef)
