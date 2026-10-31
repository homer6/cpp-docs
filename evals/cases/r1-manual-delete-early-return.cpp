#include <string>

struct Parser { bool parse(const std::string&) { return true; } };

bool load(const std::string& text) {
    Parser* p = new Parser{};
    if (text.empty()) {
        return false;          // leaks p
    }
    bool ok = p->parse(text);
    delete p;
    return ok;
}
