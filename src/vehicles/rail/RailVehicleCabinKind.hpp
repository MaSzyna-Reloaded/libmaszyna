#pragma once
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    /* What a cabin of a rail vehicle is - the railway's own words for the cabins VehicleServer
     * knows only by their handles. A class of its own, holding nothing, as VehicleComponentType is:
     * RailVehicleServer names it and RailVehicleController takes it, and the server includes the
     * controller. */
    class RailVehicleCabinKind : public Object {
            GDCLASS(RailVehicleCabinKind, Object)

        protected:
            static void _bind_methods();

        public:
            enum Kind {
                /* A cabin the railway has no word for, or no cabin */
                RAIL_VEHICLE_CABIN_NONE,
                /* The front cab, the MMD's cab1definition: (CabOccupied 1) */
                RAIL_VEHICLE_CABIN_FRONT,
                /* The machine room, cab0definition: (CabOccupied 0) */
                RAIL_VEHICLE_CABIN_MACHINE,
                /* The rear cab, cab2definition: (CabOccupied -1) */
                RAIL_VEHICLE_CABIN_REAR,
            };
    };
} // namespace godot

VARIANT_ENUM_CAST(RailVehicleCabinKind::Kind);
