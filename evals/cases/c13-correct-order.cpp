#include <memory>
#include <thread>

class Reporter {
public:
    Reporter()
        : value_{std::make_unique<int>(42)},
          worker_{[this] { (void)*value_; }} {}
private:
    std::unique_ptr<int> value_;     // declared first: outlives worker_
    std::jthread worker_;
};
