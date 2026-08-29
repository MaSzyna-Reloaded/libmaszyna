#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    /* A single (x, y) point of a linearly-interpolated characteristic curve. Shared by every FIZ
     * table section that reduces to a two-column X/Y table feeding a std::map<double, double>
     * (e.g. Pmaxlist, DMList, HTCList, V2NMAXList). */
    class CurvePointItem : public Resource {
            GDCLASS(CurvePointItem, Resource);

        public:
            static void _bind_methods();
            MAKE_MEMBER_GS(double, x, 0.0);
            MAKE_MEMBER_GS(double, y, 0.0);
    };
} // namespace godot
