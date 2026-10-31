// modules.md "Module linkage, visibility and reachability".
export module counter;

int step() { return 2; }
export int next(int n);

// "An inline function or template exported from a module can use non-exported helpers".
int scale_impl(int v) { return v * 10; }
export template <class T> T scaled(T v) { return static_cast<T>(scale_impl(static_cast<int>(v))); }
