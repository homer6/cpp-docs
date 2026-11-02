# A: Architectural ideas — examples

> Our examples for the [A entries](../a.md): how a code base is divided into
> parts and libraries.

## A.1: Separate stable code from less stable code

Code that changes often (a file format, a vendor API, a business rule)
should sit behind an interface that changes rarely, so the rest of the
program doesn't change with it and can be tested without it.

```cpp bad
#include <string>
// BAD: compiles, but the report logic is tied to one storage format, so a new
// format means editing (and re-testing) the report code too.
std::string monthly_report(const std::string& csv_text) {
    std::string out;
    for (char c : csv_text) out += (c == ',') ? '\t' : c;   // format parsing mixed into the report
    return out;
}
```

```cpp
#include <string>
#include <vector>
struct Sales { std::string region; double amount; };

class SalesSource {                          // GOOD: the stable interface
public:
    virtual ~SalesSource() = default;
    virtual std::vector<Sales> load() const = 0;
};

double monthly_total(const SalesSource& source) {   // stable code depends only on the interface
    double total = 0;
    for (const auto& s : source.load()) total += s.amount;
    return total;
}
```

## A.2: Express potentially reusable parts as a library

*No example: it's about packaging code as a library (headers, object files, documentation, shipped together), which a code snippet can't show.*

## A.4: There should be no cycles among libraries

*No example: it's about dependencies between libraries in a build, which a single code snippet can't show.*
