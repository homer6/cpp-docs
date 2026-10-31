// building.md "import std; and import std.compat;": the C names in the global namespace.
export module stdcompat_demo;
import std.compat;

export unsigned long c_strlen(const char* s) { return ::strlen(s); }
