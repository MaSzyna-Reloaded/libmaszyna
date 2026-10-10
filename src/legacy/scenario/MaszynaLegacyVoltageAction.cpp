#include "MaszynaLegacyVoltageAction.hpp"
#include "traction/TractionServer.hpp"

namespace godot {
    void MaszynaLegacyVoltageAction::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("set_power_sources", "power_sources"), &MaszynaLegacyVoltageAction::set_power_sources);
        ClassDB::bind_method(D_METHOD("get_power_sources"), &MaszynaLegacyVoltageAction::get_power_sources);
        ClassDB::bind_method(D_METHOD("set_voltage", "voltage"), &MaszynaLegacyVoltageAction::set_voltage);
        ClassDB::bind_method(D_METHOD("get_voltage"), &MaszynaLegacyVoltageAction::get_voltage);

        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "power_sources", PROPERTY_HINT_ARRAY_TYPE, "RID"), "set_power_sources",
                "get_power_sources");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "voltage"), "set_voltage", "get_voltage");
    }

    void MaszynaLegacyVoltageAction::run(const RID &p_event, const RID &p_activator) {
        TractionServer *server = TractionServer::get_instance();
        ERR_FAIL_NULL(server);
        for (int i = 0; i < power_sources.size(); i++) {
            server->power_source_set_nominal_voltage(power_sources[i], voltage);
        }
    }

    void MaszynaLegacyVoltageAction::set_power_sources(const TypedArray<RID> &p_power_sources) {
        power_sources = p_power_sources;
    }

    TypedArray<RID> MaszynaLegacyVoltageAction::get_power_sources() const {
        return power_sources;
    }

    void MaszynaLegacyVoltageAction::set_voltage(const double p_voltage) {
        voltage = p_voltage;
    }

    double MaszynaLegacyVoltageAction::get_voltage() const {
        return voltage;
    }
} // namespace godot
