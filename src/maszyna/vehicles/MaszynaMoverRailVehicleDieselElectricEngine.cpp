#include "MaszynaMoverRailVehicleDieselElectricEngine.hpp"
#include "maszyna/vehicles/MaszynaMoverBackend.hpp"
#include <algorithm>
#include <cmath>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void MaszynaMoverRailVehicleDieselElectricEngine::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("get_circuit_nmax_rpm"), &MaszynaMoverRailVehicleDieselElectricEngine::get_circuit_nmax_rpm);
        ClassDB::bind_method(D_METHOD("get_eimic_real"), &MaszynaMoverRailVehicleDieselElectricEngine::get_eimic_real);
        ClassDB::bind_method(
                D_METHOD("get_clutch_desired"), &MaszynaMoverRailVehicleDieselElectricEngine::get_clutch_desired);
        ClassDB::bind_method(
                D_METHOD("get_engine_configuration"),
                &MaszynaMoverRailVehicleDieselElectricEngine::get_engine_configuration);
        ClassDB::bind_method(
                D_METHOD("set_engine_configuration", "value"),
                &MaszynaMoverRailVehicleDieselElectricEngine::set_engine_configuration);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::OBJECT, "engine_configuration", PROPERTY_HINT_RESOURCE_TYPE,
                        "MaszynaMoverEngineConfiguration"),
                "set_engine_configuration", "get_engine_configuration");
        ClassDB::bind_method(
                D_METHOD("get_diesel_configuration"),
                &MaszynaMoverRailVehicleDieselElectricEngine::get_diesel_configuration);
        ClassDB::bind_method(
                D_METHOD("set_diesel_configuration", "value"),
                &MaszynaMoverRailVehicleDieselElectricEngine::set_diesel_configuration);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::OBJECT, "diesel_configuration", PROPERTY_HINT_RESOURCE_TYPE,
                        "MaszynaMoverDieselConfiguration"),
                "set_diesel_configuration", "get_diesel_configuration");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                MaszynaMoverRailVehicleDieselElectricEngine, Variant::ARRAY, wwlist, PROPERTY_HINT_TYPE_STRING,
                "MaszynaMoverWWListItem");
        BIND_PROPERTY(MaszynaMoverRailVehicleDieselElectricEngine, Variant::BOOL, generator_voltage_flat);
        BIND_PROPERTY(MaszynaMoverRailVehicleDieselElectricEngine, Variant::FLOAT, hyperbolic_speed);
        BIND_PROPERTY(MaszynaMoverRailVehicleDieselElectricEngine, Variant::FLOAT, additional_speed);
        BIND_PROPERTY(MaszynaMoverRailVehicleDieselElectricEngine, Variant::FLOAT, power_correction_ratio);
        BIND_PROPERTY(MaszynaMoverRailVehicleDieselElectricEngine, Variant::INT, shunt_relay_type);
    }

    void MaszynaMoverRailVehicleDieselElectricEngine::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        RailVehicleDieselElectricEngine::_apply_configuration();

        p_mover->Flat = get_generator_voltage_flat();
        p_mover->Vhyp = get_hyperbolic_speed();
        p_mover->Vadd = get_additional_speed();
        p_mover->PowerCorRatio = get_power_correction_ratio();
        p_mover->RelayType = get_shunt_relay_type();
        p_mover->ShuntModeAllow = get_shunt_mode_allowed();
        p_mover->EngineHeatingRPM = get_heating_rpm();

        /* WWList: tablica rezystorow rozr. (eng. Starting resistor array) aka DEList aka TDESchemeTable */
        constexpr int MAX = sizeof(p_mover->DElist) / sizeof(Maszyna::TDEScheme);
        const int wwlist_size = static_cast<int>(get_wwlist().size());
        for (int i = 0; i < std::min(MAX, wwlist_size); i++) {
            const Ref<MaszynaMoverWWListItem> &row = get_wwlist()[i];
            if (row == nullptr || !row.is_valid() || row.is_null()) {
                UtilityFunctions::push_warning(
                        "[MaszynaMoverRailVehicleDieselElectricEngine]: wwlist property is null at index " +
                        String::num(i));
                continue;
            }

            p_mover->DElist[i].RPM = row->get_rpm();
            p_mover->DElist[i].GenPower = row->get_max_power();
            p_mover->DElist[i].Umax = row->get_max_voltage();
            p_mover->DElist[i].Imax = row->get_max_current();
            if (row->get_has_shunting()) {
                p_mover->SST[i].Umin = row->get_min_wakeup_voltage();
                p_mover->SST[i].Umax = row->get_max_wakeup_voltage();
                p_mover->SST[i].Pmax = row->get_max_wakeup_power();
                p_mover->SST[i].Pmin = std::sqrt(std::pow(p_mover->SST[i].Umin, 2) / WWLIST_SHUNT_POWER_DIVISOR);
                p_mover->SST[i].Pmax =
                        std::min(p_mover->SST[i].Pmax, std::pow(p_mover->SST[i].Umax, 2) / WWLIST_SHUNT_POWER_DIVISOR);
            }
        }
    }

    Ref<MaszynaMoverEngineConfiguration> MaszynaMoverRailVehicleDieselElectricEngine::get_engine_configuration() const {
        return engine_configuration;
    }

    void MaszynaMoverRailVehicleDieselElectricEngine::set_engine_configuration(
            const Ref<MaszynaMoverEngineConfiguration> &p_value) {
        engine_configuration = p_value;
    }

    Ref<MaszynaMoverDieselConfiguration> MaszynaMoverRailVehicleDieselElectricEngine::get_diesel_configuration() const {
        return diesel_configuration;
    }

    void MaszynaMoverRailVehicleDieselElectricEngine::set_diesel_configuration(
            const Ref<MaszynaMoverDieselConfiguration> &p_value) {
        diesel_configuration = p_value;
    }

    double MaszynaMoverRailVehicleDieselElectricEngine::get_circuit_nmax_rpm() const {
        return drive_unit_impl.get_circuit_nmax_rpm();
    }

    double MaszynaMoverRailVehicleDieselElectricEngine::get_eimic_real() const {
        return drive_unit_impl.get_eimic_real();
    }

    double MaszynaMoverRailVehicleDieselElectricEngine::get_clutch_desired() const {
        return diesel_engine_unit_impl.get_clutch_desired();
    }
} // namespace godot
