# Classes

User-defined types: lifecycle, members, inheritance, and the special forms.
Mirrors [cppreference's classes](https://en.cppreference.com/w/cpp/language/classes).

| Page | What |
|---|---|
| [Special members & RAII](special-members.md) | constructors, destructor, copy/move, rule of 0/3/5 |
| [Members & access](members-access.md) | data/function members, `static`, access specifiers, `friend` |
| [Inheritance & polymorphism](inheritance-polymorphism.md) | `virtual`, `override`, abstract classes, dynamic dispatch |
| [Unions & aggregates](unions-aggregates.md) | plain structs, aggregates, unions (and why to prefer `variant`) |
| [Explicit object parameters](explicit-object-parameter.md) | deducing `this` (C++23): one accessor for every value category, recursive lambdas, CRTP without templates |
| [Bit-fields](bit-fields.md) | packed integer members, implementation-defined layout, C++20 initializers, `[[no_unique_address]]` |
| [Operator overloading](operator-overloading.md) | member vs hidden-friend operators, `++`, `[]` (multi-arg, C++23), `()`, `explicit operator bool`, `->` |
