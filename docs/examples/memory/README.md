# Memory management library

Owning resources safely (RAII via smart pointers) and controlling where/how
memory comes from (allocators). Mirrors
[cppreference's memory library](https://en.cppreference.com/w/cpp/memory).

| Page | Header | Since | What |
|---|---|---|---|
| [`unique_ptr`](unique_ptr.md) | `<memory>` | C++11 | Sole ownership, zero overhead |
| [`shared_ptr`](shared_ptr.md) | `<memory>` | C++11 | Shared ownership, reference-counted |
| [`weak_ptr`](weak_ptr.md) | `<memory>` | C++11 | Non-owning observer of a `shared_ptr` |
| [Allocators](allocators.md) | `<memory>` | C++11 | How containers obtain raw storage |
| [Memory resources (PMR)](memory-resources.md) | `<memory_resource>` | C++17 | Runtime-polymorphic allocation strategies |
| [Raw memory utilities](raw-memory.md) | `<memory>` | C++11+ | `addressof`, `construct_at`, `uninitialized_*` |

## Which owner do I use?

- **One owner, transfers by move** → [`unique_ptr`](unique_ptr.md). The default;
  free (same size/speed as a raw pointer).
- **Genuinely shared lifetime, last one out frees it** → [`shared_ptr`](shared_ptr.md).
  Only when ownership is truly shared — it's not free.
- **Observe without owning / break a cycle** → [`weak_ptr`](weak_ptr.md).
- **Don't own at all (just point)** → a **raw pointer or reference**. Raw pointers
  are fine for non-owning parameters; smart pointers are about *ownership*.
