#include <memory>
#include <string>

class Logger {
public:
    explicit Logger(std::string path) : path_(std::move(path)) {}
private:
    std::string path_;
};

class App {
public:
    App() : logger_(std::make_shared<Logger>("app.log")) {}
private:
    std::shared_ptr<Logger> logger_;   // never shared with anyone
};
