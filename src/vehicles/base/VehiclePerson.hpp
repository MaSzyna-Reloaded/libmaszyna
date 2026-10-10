#pragma once
#include "vehicles/base/VehiclePersonRole.hpp"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /* One person aboard, as VehicleServer answered a query: who, in which cabin, in what role. A
     * snapshot - it does not follow the person, a holder asks again after the occupancy changed. */
    class VehiclePerson : public RefCounted {
            GDCLASS(VehiclePerson, RefCounted)

        private:
            RID person;
            RID cabin;
            VehiclePersonRole::Role role = VehiclePersonRole::VEHICLE_PERSON_ROLE_ANY;

        protected:
            static void _bind_methods();

        public:
            static Ref<VehiclePerson> create(const RID &p_person, const RID &p_cabin, VehiclePersonRole::Role p_role);

            RID get_person() const;
            RID get_cabin() const;
            VehiclePersonRole::Role get_role() const;
    };
} // namespace godot
