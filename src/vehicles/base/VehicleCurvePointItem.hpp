#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    /* A single (x, y) point of a linearly-interpolated characteristic curve. Shared by every FIZ
     * table section that reduces to a two-column X/Y table feeding a std::map<double, double>
     * (e.g. Pmaxlist, DMList, HTCList, V2NMAXList). */
    class VehicleCurvePointItem : public Resource {
            GDCLASS(VehicleCurvePointItem, Resource);

        public:
            static void _bind_methods();

        private:
            double x = 0.0;
            double y = 0.0;

        public:
            MAKE_GS(double, x);
            MAKE_GS(double, y);
    };
} // namespace godot
