#include <functional>
#include <string>
#include <vector>

struct Executor { std::vector<std::function<void()>> queue; void post(std::function<void()> f) { queue.push_back(std::move(f)); } };

class Session {
public:
    explicit Session(Executor& ex) : ex_{ex} {}
    void on_request(std::string body) {
        ex_.post([this, body = std::move(body)] { reply_ += body; });   // runs later
    }
private:
    Executor& ex_;
    std::string reply_;
};
