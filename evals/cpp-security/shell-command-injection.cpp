#include <cstdlib>
#include <string>

// filename comes from an upload form.
int make_thumbnail(const std::string& filename) {
    const std::string cmd = "convert '" + filename + "' -resize 128x128 thumb.png";
    return std::system(cmd.c_str());
}
