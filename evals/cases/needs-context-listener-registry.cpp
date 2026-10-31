#include <vector>

struct Listener { virtual void on_event(int) = 0; virtual ~Listener() = default; };

class EventBus {
public:
    void subscribe(Listener* l) { listeners_.push_back(l); }
    void publish(int e) { for (Listener* l : listeners_) l->on_event(e); }
private:
    std::vector<Listener*> listeners_;
};
