#include "MaszynaMoverRailVehicleWipers.hpp"
#include "maszyna/vehicles/MaszynaMoverBackend.hpp"
#include <algorithm>
#include <godot_cpp/core/math.hpp>

namespace godot {
    namespace {
        /* A wiper's air motor (not the original's): the main reservoir's pressure it stands below
         * and goes in full from [bar] */
        constexpr double VALVE_AIR_PRESSURE_MIN = 1.5;
        constexpr double VALVE_AIR_PRESSURE_FULL = 3.5;
    } // namespace

    void MaszynaMoverRailVehicleWipers::_bind_methods() {
        BIND_PROPERTY(MaszynaMoverRailVehicleWipers, Variant::FLOAT, valve_slowest_sweep_time);
        BIND_PROPERTY(MaszynaMoverRailVehicleWipers, Variant::FLOAT, valve_fastest_sweep_time);
        BIND_PROPERTY(MaszynaMoverRailVehicleWipers, Variant::FLOAT, valve_sensitivity);
    }

    void MaszynaMoverRailVehicleWipers::_apply_configuration() {
        size_t count = static_cast<size_t>(std::max(0, get_wiper_count()));
        if (count == 0) {
            int mask = 0;
            for (int i = 0; i < get_positions().size(); i++) {
                const Ref<RailVehicleWiperListItem> item = get_positions()[i];
                if (item.is_valid()) {
                    mask |= item->get_wiper_mask();
                }
            }
            while (mask > 0) {
                count++;
                mask >>= 1;
            }
        }
        if (count != wipers.size()) {
            wipers.assign(count, Wiper());
        }
    }

    // DynObj.cpp:4048-4115 - the wipers are the vehicle's, the simulation has none: it only says
    // whether the battery is on, and the vehicle which cab is switched on
    void MaszynaMoverRailVehicleWipers::_do_process_component(const double p_delta) {
        const RailVehicleController *controller = get_rail_vehicle_controller();
        ERR_FAIL_NULL(controller);
        // driven by hand: each wiper stands where its handle does, the linkage moving it at once;
        // with its valve open and the air there, its motor takes it there and back
        if (get_drive() == DRIVE_MANUAL) {
            const TMoverParameters *air_mover = get_mover();
            const double air = air_mover != nullptr
                                       ? Math::clamp(
                                                 (air_mover->Compressor - VALVE_AIR_PRESSURE_MIN) /
                                                         (VALVE_AIR_PRESSURE_FULL - VALVE_AIR_PRESSURE_MIN),
                                                 0.0, 1.0)
                                       : 0.0;
            for (int i = 0; i < static_cast<int>(wipers.size()); i++) {
                Wiper &wiper = wipers[i];
                const double opening = valve_get_opening(i);
                const double share = 1.0 - Math::pow(1.0 - opening, valve_sensitivity);
                const double step =
                        opening > 0.0 ? air * p_delta / valve_slowest_sweep_time *
                                                Math::pow(valve_slowest_sweep_time / valve_fastest_sweep_time, share)
                                      : 0.0;
                if (step > 0.0) {
                    double position = manual_get_position(i) + (wiper.valve_returning ? -step : step);
                    if (position >= 1.0 || position <= 0.0) {
                        wiper.valve_returning = position >= 1.0;
                        position = Math::clamp(position, 0.0, 1.0);
                    }
                    valve_move_wiper(position, i);
                }
                wiper.position = manual_get_position(i);
                wiper.returning = false;
            }
            return;
        }
        if (get_positions().size() == 0) {
            return;
        }
        sweep_clock += p_delta;
        const RailVehicleCabinKind::Kind active_cabin = controller->get_active_cabin_kind();
        const TMoverParameters *mover = get_mover();
        const bool battery = mover != nullptr && mover->Battery;
        const Ref<RailVehicleWiperListItem> switched = get_positions()[get_switch_position()];
        const bool switched_on = switched.is_valid() && battery &&
                                 active_cabin != RailVehicleCabinKind::RAIL_VEHICLE_CABIN_NONE &&
                                 switched->get_wiper_mask() > 0;
        if (parked && !switched_on) {
            return;
        }
        parked = true;
        const int count = static_cast<int>(wipers.size());
        for (int i = 0; i < count; i++) {
            Wiper &wiper = wipers[i];
            // the wipers are numbered from the side of the active cab
            bool active = false;
            if (switched.is_valid() && battery) {
                if (active_cabin == RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT) {
                    active = (switched->get_wiper_mask() & (1 << i)) != 0;
                } else if (active_cabin == RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR) {
                    active = (switched->get_wiper_mask() & (1 << (count - 1 - i))) != 0;
                }
            }

            if (!active && wiper.position <= 0.0) {
                continue;
            }
            parked = false;

            // the parameters of the position the wiper started its sweep with
            const Ref<RailVehicleWiperListItem> working = get_positions()[wiper.working_switch_position];
            if (working.is_null() || working->get_transit_time() <= 0.0) {
                continue;
            }
            const double step = p_delta / working->get_transit_time();

            if (!active) {
                // switched off on the way: back to the parked position at once
                wiper.position = std::max(0.0, wiper.position - step);
                continue;
            }
            if (wiper.position < 1.0 && !wiper.returning && sweep_clock - wiper.park_since > working->get_period()) {
                wiper.position = std::min(1.0, wiper.position + step);
            }
            if (wiper.position > 0.0 && wiper.returning &&
                sweep_clock - wiper.out_since > working->get_return_delay()) {
                wiper.position = std::max(0.0, wiper.position - step);
            }
            if (wiper.position >= 1.0) {
                wiper.park_since = sweep_clock;
                wiper.returning = true;
            }
            if (wiper.position <= 0.0) {
                wiper.out_since = sweep_clock;
                wiper.returning = false;
                wiper.working_switch_position = get_switch_position();
            }
        }
    }

    PackedFloat64Array MaszynaMoverRailVehicleWipers::get_sweep_positions() const {
        PackedFloat64Array sweep;
        for (const Wiper &wiper: wipers) {
            sweep.push_back(wiper.position > 0.0 && wiper.returning ? wiper.position + 1.0 : wiper.position);
        }
        return sweep;
    }
} // namespace godot
