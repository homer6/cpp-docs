# Range concepts & access

> What counts as a *range*, what a *view* is, and the customization points that
> get iterators out of one. **Header:** `<ranges>` · **Since:** C++20

## A range is anything with begin/end

```cpp
std::ranges::range<T>          // has ranges::begin(t) and ranges::end(t)
```

Every standard container is a range, as are arrays, `string_view`, `span`, and
views. The end need not be the same type as the begin iterator — it can be a
**sentinel**, which is what makes lazy/unbounded ranges possible.

## The refinement hierarchy

| Concept | Adds |
|---|---|
| `range` | `begin`/`end` |
| `input_range` … `contiguous_range` | mirror the [iterator categories](../iterators/categories.md) |
| `sized_range` | O(1) `ranges::size(r)` |
| `common_range` | `begin` and `end` are the same type |
| `borrowed_range` | iterators stay valid even if the range expression was a temporary |
| `view` | cheap-to-copy, non-owning; movable in O(1) |
| `viewable_range` | safe to turn into a view via `views::all` |

## Range access (customization points)

Use the `std::ranges::` functions, not member calls — they work uniformly and do
the right thing for arrays, members, and ADL:

```cpp
std::ranges::begin(r);   std::ranges::end(r);
std::ranges::size(r);    std::ranges::empty(r);
std::ranges::data(r);                                  // contiguous ranges
std::ranges::cbegin(r);  std::ranges::cend(r);          // const views
```

## view vs. container

- A **container** *owns* its elements; copying it copies the data.
- A **view** *refers* to elements someone else owns; copying it is O(1) and
  copies no elements. Views are lazy — work happens during iteration.

```cpp
std::vector<int> v{1,2,3,4};
auto evens = v | std::views::filter([](int x){ return x%2==0; });  // a view, owns nothing
// `evens` is invalidated if `v` is destroyed or reallocates
```

## Dangling protection

Because views don't own, the library guards against returning iterators into
temporaries. Range algorithms over a temporary return `std::ranges::dangling`,
and only `borrowed_range`s (like `span`, `string_view`, or an lvalue container)
yield usable iterators from a temporary expression.

```cpp
auto it = std::ranges::find(std::vector{1,2,3}, 2);   // std::ranges::dangling (compile-time safety)
```

## Gotchas

- **Views don't own — lifetime is on you.** Storing a view whose source
  container then dies (or reallocates) is a dangling-iterator bug, exactly like
  [`string_view`](../strings/string_view.md)/[`span`](../containers/views/span.md).
- **Use `std::ranges::begin(r)`, not `r.begin()`,** in generic code — it also
  handles raw arrays and finds non-member `begin` via ADL.
- **`view` requires O(1) copy/move.** Don't try to make an owning container model
  `view`; wrap it with `views::all` (or pass an lvalue) instead.

## See also

- [std::ranges::range — cppreference](https://en.cppreference.com/w/cpp/ranges/range)
- [std::ranges::view — cppreference](https://en.cppreference.com/w/cpp/ranges/view)
- [Range access (ranges::begin etc.) — cppreference](https://en.cppreference.com/w/cpp/ranges/begin)
