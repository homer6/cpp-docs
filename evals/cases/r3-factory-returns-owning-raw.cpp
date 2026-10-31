#include <string>

struct Connection { std::string host; };

Connection* open_connection(const std::string& host) {
    return new Connection{host};   // caller must remember to delete
}
