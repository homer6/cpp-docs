// modules.md "The global and private module fragments": the macro stays in this file.
module;
#include <cstddef>
#define CONFIG_BUFFER_SIZE 64
export module config;

export constexpr std::size_t buffer_size = CONFIG_BUFFER_SIZE;
