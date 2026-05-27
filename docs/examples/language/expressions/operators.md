# Operators, precedence & evaluation order

> The operators, how tightly they bind, how to overload them, and the rules (and
> traps) of evaluation order.

## Precedence — the high-frequency ones

From tightest to loosest (abbreviated):

```
 ::                                   scope
 a() a[] a.b a->b  postfix ++ --
 !  ~  unary + -  *deref &addr  (cast)  sizeof  prefix ++ --   (right-to-left)
 .*  ->*
 *  /  %
 +  -
 <<  >>
 <=>                                  three-way
 <  <=  >  >=
 ==  !=
 &  ^  |                              bitwise (LOWER than comparisons!)
 &&  ||
 ?:  =  +=  ...                       assignment (right-to-left)
 ,                                    comma
```

When in doubt, parenthesize — see Gotchas.

## Overloading operators

Define operators for your own types as members or free functions:

```cpp
struct Vec2 {
    double x, y;
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    bool operator==(const Vec2&) const = default;     // C++20: defaulted
    auto operator<=>(const Vec2&) const = default;     // gives < <= > >=
};
Vec2 operator+(Vec2 a, const Vec2& b) { return a += b; }   // free function, symmetric
```

Conventions: return `*this` from compound assignment; implement `+` in terms of
`+=`; make comparisons `= default` when memberwise is right (see
[`operator<=>`](../../language-support/comparison.md)).

## Evaluation order

C++17 tightened some rules, but most binary operators still **don't** order their
operands:

```cpp
f() + g();             // unspecified which of f/g runs first
a[i] = i++;            // UB pre-C++17; defined in C++17 (RHS sequenced before assignment)
std::cout << f() << g();// C++17: left-to-right for << chains
i = i++ + 1;            // still UB-ish territory — don't
```

- `&&`, `||`, `,`, and `?:` **do** guarantee left-to-right with a sequence point
  (and short-circuit for `&&`/`||`).
- Function argument evaluation order is **unspecified** (but indeterminately
  sequenced, not interleaved, since C++17).

## Gotchas

- **Bitwise `&`/`|`/`^` bind looser than comparisons.** `if (x & mask == 0)`
  parses as `x & (mask == 0)` — almost never what you mean. Parenthesize:
  `(x & mask) == 0`.
- **Don't read and modify the same object without sequencing** (`i = i++ + ++i;`)
  — UB or unspecified. One side effect per object per full expression.
- **Don't rely on argument evaluation order.** `log(next(), next())` may call
  `next` in either order.
- **Overload symmetrically and minimally.** Prefer defaulted `==`/`<=>` (C++20);
  define `+` via `+=`; don't overload `&&`/`||`/`,` (you lose short-circuit /
  ordering).

## See also

- [Operator precedence — cppreference](https://en.cppreference.com/w/cpp/language/operator_precedence)
- [Operator overloading — cppreference](https://en.cppreference.com/w/cpp/language/operators)
- [Evaluation order — cppreference](https://en.cppreference.com/w/cpp/language/eval_order)
