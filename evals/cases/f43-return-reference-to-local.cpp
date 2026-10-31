#include <string>

const std::string& greeting(bool formal) {
    std::string s = formal ? "Good day" : "Hi";
    return s;                  // dangling
}
