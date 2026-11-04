#include <cstdlib>
#include <ctime>
#include <string>

std::string new_session_token() {
    static const bool seeded = (std::srand(static_cast<unsigned>(std::time(nullptr))), true);
    (void)seeded;
    std::string token;
    for (int i = 0; i < 32; ++i) token += "0123456789abcdef"[std::rand() % 16];
    return token;
}
