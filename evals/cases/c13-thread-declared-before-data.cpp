#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

class Reporter {
public:
    Reporter()
        : value_{std::make_unique<int>(42)},
          worker_{[this] {
              std::this_thread::sleep_for(std::chrono::milliseconds(10));
              std::cout << *value_ << '\n';
          }} {}
private:
    std::jthread worker_;            // constructed first, destroyed last
    std::unique_ptr<int> value_;     // used by worker_
};
