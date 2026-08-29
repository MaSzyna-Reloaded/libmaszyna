#include "TrainDieselElectricEngine.hpp"
#include <algorithm>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void TrainDieselElectricEngine::_bind_methods() {
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                Variant::ARRAY, "wwlist", "wwlist", &TrainDieselElectricEngine::set_wwlist,
                &TrainDieselElectricEngine::get_wwlist, "wwlist", PROPERTY_HINT_TYPE_STRING, "WWListItem");
        BIND_PROPERTY(
                Variant::BOOL, "generator_voltage_flat", "generator_voltage_flat",
                &TrainDieselElectricEngine::set_generator_voltage_flat,
                &TrainDieselElectricEngine::get_generator_voltage_flat, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "hyperbolic_speed", "hyperbolic_speed", &TrainDieselElectricEngine::set_hyperbolic_speed,
                &TrainDieselElectricEngine::get_hyperbolic_speed, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "additional_speed", "additional_speed",
                &TrainDieselElectricEngine::set_additional_speed, &TrainDieselElectricEngine::get_additional_speed,
                "value");
        BIND_PROPERTY(
                Variant::FLOAT, "rpm_change_rate", "rpm_change_rate", &TrainDieselElectricEngine::set_rpm_change_rate,
                &TrainDieselElectricEngine::get_rpm_change_rate, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "power_correction_ratio", "power_correction_ratio",
                &TrainDieselElectricEngine::set_power_correction_ratio,
                &TrainDieselElectricEngine::get_power_correction_ratio, "value");
        BIND_PROPERTY(
                Variant::INT, "shunt_relay_type", "shunt_relay_type", &TrainDieselElectricEngine::set_shunt_relay_type,
                &TrainDieselElectricEngine::get_shunt_relay_type, "value");
        BIND_PROPERTY(
                Variant::BOOL, "shunt_mode_allowed", "shunt_mode_allowed",
                &TrainDieselElectricEngine::set_shunt_mode_allowed, &TrainDieselElectricEngine::get_shunt_mode_allowed,
                "value");
        BIND_PROPERTY(
                Variant::FLOAT, "heating_rpm", "heating_rpm", &TrainDieselElectricEngine::set_heating_rpm,
                &TrainDieselElectricEngine::get_heating_rpm, "value");
    }

    TrainEngine::EngineType TrainDieselElectricEngine::get_engine_type() {
        return TrainEngine::EngineType::DIESEL_ELECTRIC;
    }

    void TrainDieselElectricEngine::_do_update_internal_mover(TMoverParameters *p_mover) {
        TrainDieselEngine::_do_update_internal_mover(p_mover);

        p_mover->Flat = generator_voltage_flat;
        p_mover->Vhyp = hyperbolic_speed;
        p_mover->Vadd = additional_speed;
        p_mover->dizel_RevolutionsDecreaseRate = rpm_change_rate;
        p_mover->PowerCorRatio = power_correction_ratio;
        p_mover->RelayType = shunt_relay_type;
        p_mover->ShuntModeAllow = shunt_mode_allowed;
        p_mover->EngineHeatingRPM = heating_rpm;

        /* WWList: tablica rezystorow rozr. (eng. Starting resistor array) aka DEList aka TDESchemeTable */
        constexpr int MAX = sizeof(p_mover->DElist) / sizeof(Maszyna::TDEScheme);
        const int wwlist_size = static_cast<int>(wwlist.size());
        p_mover->MainCtrlPosNo = wwlist_size - 1;
        for (int i = 0; i < std::min(MAX, wwlist_size); i++) {
            const Ref<WWListItem> &row = wwlist[i];
            if (row == nullptr || !row.is_valid() || row.is_null()) {
                UtilityFunctions::push_warning(
                        "[TrainDieselElectricEngine]: wwlist property is null at index " + String::num(i));
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
                p_mover->SST[i].Pmin = std::sqrt(std::pow(p_mover->SST[i].Umin, 2) / 47.6);
                p_mover->SST[i].Pmax = std::min(p_mover->SST[i].Pmax, std::pow(p_mover->SST[i].Umax, 2) / 47.6);
            }
        }
    }
} // namespace godot
