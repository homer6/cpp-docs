#include <string>
#include <utility>

class User {
public:
    void set_name(std::string name) { name_ = std::move(name); }   // sink
private:
    std::string name_;
};
