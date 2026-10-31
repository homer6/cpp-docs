#include <memory>
#include <vector>

struct Node {
    Node* parent = nullptr;                       // non-owning back pointer
    std::vector<std::unique_ptr<Node>> children;  // owning

    Node& add_child() {
        auto& c = children.emplace_back(std::make_unique<Node>());
        c->parent = this;
        return *c;
    }
};
