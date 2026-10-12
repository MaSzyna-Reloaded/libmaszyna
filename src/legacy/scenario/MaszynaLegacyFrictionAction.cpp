#include "MaszynaLegacyFrictionAction.hpp"
#include "simulation/SimulationServer.hpp"

namespace godot {
    void MaszynaLegacyFrictionAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_friction", "friction"), &MaszynaLegacyFrictionAction::set_friction);
        ClassDB::bind_method(D_METHOD("get_friction"), &MaszynaLegacyFrictionAction::get_friction);

        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "friction"), "set_friction", "get_friction");
    }

    void MaszynaLegacyFrictionAction::run(const RID &p_event, const RID &p_activator) {
        SimulationServer *simulation = SimulationServer::get_instance();
        ERR_FAIL_NULL(simulation);
        simulation->set_friction(friction);
    }

    void MaszynaLegacyFrictionAction::set_friction(const double p_friction) {
        friction = p_friction;
    }

    double MaszynaLegacyFrictionAction::get_friction() const {
        return friction;
    }
} // namespace godot
