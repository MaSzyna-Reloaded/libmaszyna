#pragma once
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    /* What a person aboard a vehicle is there for. A class of its own, holding nothing, for the
     * reason VehicleComponentType is one: GDScript sees an enum's constants only as members of a
     * registered class, and VehicleServer and VehiclePerson both name it. */
    class VehiclePersonRole : public Object {
            GDCLASS(VehiclePersonRole, Object)

        protected:
            static void _bind_methods();

        public:
            enum Role {
                /* Not a role anybody has - a query's filter matching every role */
                VEHICLE_PERSON_ROLE_ANY,
                /* At the controls of the cabin; one a cabin */
                VEHICLE_PERSON_ROLE_DRIVER,
                /* Aboard, touching nothing */
                VEHICLE_PERSON_ROLE_OBSERVER,
            };
    };
} // namespace godot

VARIANT_ENUM_CAST(VehiclePersonRole::Role);
