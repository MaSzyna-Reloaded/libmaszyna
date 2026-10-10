#include "MoverRailVehicleDieselElectricEngine.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include <algorithm>
#include <cmath>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void MoverRailVehicleDieselElectricEngine::_bind_methods() {}

    void MoverRailVehicleDieselElectricEngine::_apply_configuration() {
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
            const Ref<RailVehicleWWListItem> &row = get_wwlist()[i];
            if (row == nullptr || !row.is_valid() || row.is_null()) {
                UtilityFunctions::push_warning(
                        "[MoverRailVehicleDieselElectricEngine]: wwlist property is null at index " + String::num(i));
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
} // namespace godot
