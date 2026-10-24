#include "MaszynaMoverRailVehicleDieselEngine.hpp"

namespace godot {
    void MaszynaMoverRailVehicleDieselEngine::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("get_circuit_nmax_rpm"), &MaszynaMoverRailVehicleDieselEngine::get_circuit_nmax_rpm);
        ClassDB::bind_method(D_METHOD("get_eimic_real"), &MaszynaMoverRailVehicleDieselEngine::get_eimic_real);
        ClassDB::bind_method(D_METHOD("get_clutch_desired"), &MaszynaMoverRailVehicleDieselEngine::get_clutch_desired);
        ClassDB::bind_method(
                D_METHOD("get_engine_configuration"), &MaszynaMoverRailVehicleDieselEngine::get_engine_configuration);
        ClassDB::bind_method(
                D_METHOD("set_engine_configuration", "value"),
                &MaszynaMoverRailVehicleDieselEngine::set_engine_configuration);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::OBJECT, "engine_configuration", PROPERTY_HINT_RESOURCE_TYPE,
                        "MaszynaMoverEngineConfiguration"),
                "set_engine_configuration", "get_engine_configuration");
        ClassDB::bind_method(
                D_METHOD("get_diesel_configuration"), &MaszynaMoverRailVehicleDieselEngine::get_diesel_configuration);
        ClassDB::bind_method(
                D_METHOD("set_diesel_configuration", "value"),
                &MaszynaMoverRailVehicleDieselEngine::set_diesel_configuration);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::OBJECT, "diesel_configuration", PROPERTY_HINT_RESOURCE_TYPE,
                        "MaszynaMoverDieselConfiguration"),
                "set_diesel_configuration", "get_diesel_configuration");
    }

    Ref<MaszynaMoverEngineConfiguration> MaszynaMoverRailVehicleDieselEngine::get_engine_configuration() const {
        return engine_configuration;
    }

    void
    MaszynaMoverRailVehicleDieselEngine::set_engine_configuration(const Ref<MaszynaMoverEngineConfiguration> &p_value) {
        engine_configuration = p_value;
    }

    Ref<MaszynaMoverDieselConfiguration> MaszynaMoverRailVehicleDieselEngine::get_diesel_configuration() const {
        return diesel_configuration;
    }

    void
    MaszynaMoverRailVehicleDieselEngine::set_diesel_configuration(const Ref<MaszynaMoverDieselConfiguration> &p_value) {
        diesel_configuration = p_value;
    }

    double MaszynaMoverRailVehicleDieselEngine::get_circuit_nmax_rpm() const {
        return drive_unit_impl.get_circuit_nmax_rpm();
    }

    double MaszynaMoverRailVehicleDieselEngine::get_eimic_real() const {
        return drive_unit_impl.get_eimic_real();
    }

    double MaszynaMoverRailVehicleDieselEngine::get_clutch_desired() const {
        return diesel_engine_unit_impl.get_clutch_desired();
    }
} // namespace godot
