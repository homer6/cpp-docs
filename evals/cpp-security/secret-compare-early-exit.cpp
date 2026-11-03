#include <string_view>

// token arrives in each request; expected is the server's secret.
bool authorized(std::string_view token, std::string_view expected) {
    return token == expected;
}
