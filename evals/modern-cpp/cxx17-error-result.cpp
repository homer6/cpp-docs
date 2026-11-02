#include <charconv>
#include <string>
#include <string_view>
#include <variant>

struct ParseError { std::string reason; };

std::variant<int, ParseError> parse_port(std::string_view s) {
    int port = 0;
    auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), port);
    if (ec != std::errc{} || end != s.data() + s.size()) return ParseError{"not a number"};
    if (port < 1 || port > 65535) return ParseError{"out of range"};
    return port;
}
