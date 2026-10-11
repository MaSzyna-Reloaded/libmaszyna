#include "RailVehicleWipers.hpp"
#include <algorithm>

namespace godot {
    void RailVehicleWipers::_bind_methods() {
        ClassDB::bind_method(D_METHOD("switch_increase"), &RailVehicleWipers::switch_increase);
        ClassDB::bind_method(D_METHOD("switch_decrease"), &RailVehicleWipers::switch_decrease);
        BIND_PROPERTY(RailVehicleWipers, Variant::FLOAT, angle);
        BIND_PROPERTY(RailVehicleWipers, Variant::INT, default_position);
        BIND_PROPERTY(RailVehicleWipers, Variant::INT, wiper_count);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleWipers, Variant::ARRAY, positions, PROPERTY_HINT_TYPE_STRING, "RailVehicleWiperListItem");

        ClassDB::bind_method(D_METHOD("get_switch_position"), &RailVehicleWipers::get_switch_position);
        ClassDB::bind_method(D_METHOD("get_sweep_positions"), &RailVehicleWipers::get_sweep_positions);
    }

    void RailVehicleWipers::_register_commands() {
        register_command("wipers_switch_increase", Callable(this, "switch_increase"));
        register_command("wipers_switch_decrease", Callable(this, "switch_decrease"));
    }

    void RailVehicleWipers::_unregister_commands() {
        unregister_command("wipers_switch_increase");
        unregister_command("wipers_switch_decrease");
    }

    void RailVehicleWipers::_apply_configuration() {
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

    int RailVehicleWipers::get_default_position() const {
        return default_position;
    }

    void RailVehicleWipers::set_default_position(const int p_value) {
        default_position = p_value;
        _set_switch_position(default_position);
    }

    TypedArray<RailVehicleWiperListItem> RailVehicleWipers::get_positions() const {
        return positions;
    }

    void RailVehicleWipers::set_positions(const TypedArray<RailVehicleWiperListItem> &p_value) {
        positions = p_value;
        _set_switch_position(default_position);
    }

    void RailVehicleWipers::_set_switch_position(const int p_position) {
        switch_position = std::clamp(p_position, 0, std::max(0, static_cast<int>(get_positions().size()) - 1));
    }

    void RailVehicleWipers::switch_increase() {
        _set_switch_position(switch_position + 1);
    }

    void RailVehicleWipers::switch_decrease() {
        _set_switch_position(switch_position - 1);
    }

    // DynObj.cpp:4048-4115 - the wipers are the vehicle's, the simulation has none: it only says
    // whether the battery is on, and the vehicle which cab is switched on
    void RailVehicleWipers::_do_process_component(const double p_delta) {
        const RailVehicleController *controller = get_rail_vehicle_controller();
        ERR_FAIL_NULL(controller);
        if (get_positions().size() == 0) {
            return;
        }
        sweep_clock += p_delta;
        const RailVehicleCabinKind::Kind active_cabin = controller->get_active_cabin_kind();
        const bool battery = _get_battery_enabled();
        const Ref<RailVehicleWiperListItem> switched = get_positions()[switch_position];
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
                wiper.working_switch_position = switch_position;
            }
        }
    }

    int RailVehicleWipers::get_switch_position() const {
        return switch_position;
    }

    PackedFloat64Array RailVehicleWipers::get_sweep_positions() const {
        PackedFloat64Array sweep;
        for (const Wiper &wiper: wipers) {
            sweep.push_back(wiper.position > 0.0 && wiper.returning ? wiper.position + 1.0 : wiper.position);
        }
        return sweep;
    }

    void RailVehicleWipers::_fill_state_dictionary(Dictionary &p_state) const {
        p_state["wipers_switch_position"] = get_switch_position();
        p_state["wiper_positions"] = get_sweep_positions();
    }

    void RailVehicleWipers::_fill_config_dictionary(Dictionary &p_config) const {
        p_config["wipers_switch_position_max"] = std::max(0, static_cast<int>(get_positions().size()) - 1);
        p_config["wipers_angle"] = get_angle();
    }
} // namespace godot
