// modules.md "Partitions": implementation unit importing the internal partition.
module geo;
import :detail;

namespace geo {
    int manhattan(point p) { return detail::abs(p.x) + detail::abs(p.y); }
}
