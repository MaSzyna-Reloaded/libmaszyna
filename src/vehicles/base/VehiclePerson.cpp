#include "VehiclePerson.hpp"

namespace godot {
    void VehiclePerson::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_person"), &VehiclePerson::get_person);
        ClassDB::bind_method(D_METHOD("get_cabin"), &VehiclePerson::get_cabin);
        ClassDB::bind_method(D_METHOD("get_role"), &VehiclePerson::get_role);
    }

    Ref<VehiclePerson>
    VehiclePerson::create(const RID &p_person, const RID &p_cabin, const VehiclePersonRole::Role p_role) {
        Ref<VehiclePerson> result;
        result.instantiate();
        result->person = p_person;
        result->cabin = p_cabin;
        result->role = p_role;
        return result;
    }

    RID VehiclePerson::get_person() const {
        return person;
    }

    RID VehiclePerson::get_cabin() const {
        return cabin;
    }

    VehiclePersonRole::Role VehiclePerson::get_role() const {
        return role;
    }
} // namespace godot
