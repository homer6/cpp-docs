#include <iostream>
#include <string>

struct Widget { std::string name; };

// Prints the widget if there is one. Does not own it.
void print(const Widget* w) {
    if (w) std::cout << w->name << '\n';
    else   std::cout << "(none)\n";
}
