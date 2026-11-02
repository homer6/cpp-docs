#include <algorithm>
#include <array>

constexpr std::array<std::array<int, 3>, 2> grid{{{1, 2, 3}, {4, 5, 6}}};

constexpr bool has(int v) {
    return std::ranges::any_of(grid, [v](const auto& row) { return std::ranges::find(row, v) != row.end(); });
}
static_assert(has(5) && !has(7));
