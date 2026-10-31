#include <functional>
#include <memory>
#include <string>
#include <vector>

struct Image { std::string pixels; };

// Several independent consumers hold the same decoded image for as long as
// each of them needs it; none of them knows who finishes last.
class ImageCache {
public:
    std::shared_ptr<const Image> get(const std::string& key);
    void prefetch(const std::string& key,
                  std::function<void(std::shared_ptr<const Image>)> on_ready);
private:
    std::vector<std::shared_ptr<const Image>> entries_;
};
