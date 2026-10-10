#include "MoverRailVehicleWipers.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include <algorithm>

namespace godot {
    void MoverRailVehicleWipers::_bind_methods() {}


    void MoverRailVehicleWipers::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
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
        if (!switch_initialized) {
            // Mover.cpp:11942
            _set_switch_position(get_default_position());
            switch_initialized = get_positions().size() > 0;
        }
    }

    void MoverRailVehicleWipers::_set_switch_position(const int p_position) {
        switch_position = std::clamp(p_position, 0, std::max(0, static_cast<int>(get_positions().size()) - 1));
    }

    void MoverRailVehicleWipers::switch_increase() {
        _set_switch_position(switch_position + 1);
    }

    void MoverRailVehicleWipers::switch_decrease() {
        _set_switch_position(switch_position - 1);
    }

    // DynObj.cpp:4048-4115
    // The wiper movement is the vehicle layer's, not the Mover's - the vendored Mover has no
    // wipers at all. From the Mover it reads Battery and CabActive only.
    void MoverRailVehicleWipers::_do_process_component(const double p_delta) {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        if (get_positions().size() == 0) {
            return;
        }
        sweep_clock += p_delta;
        const Ref<RailVehicleWiperListItem> switched = get_positions()[switch_position];
        const bool switched_on =
                switched.is_valid() && p_mover->Battery && !(p_mover->CabActive == 0) && switched->get_wiper_mask() > 0;
        if (parked && !switched_on) {
            return;
        }
        parked = true;
        const int count = static_cast<int>(wipers.size());
        for (int i = 0; i < count; i++) {
            Wiper &wiper = wipers[i];
            // the wipers are numbered from the side of the active cab
            bool active = false;
            if (switched.is_valid() && p_mover->Battery) {
                if (p_mover->CabActive == 1) {
                    active = (switched->get_wiper_mask() & (1 << i)) != 0;
                } else if (p_mover->CabActive == -1) {
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


    int MoverRailVehicleWipers::get_switch_position() const {
        return switch_position;
    }

    /* 0..1 sweeping out, 1..2 on the way back - wiper_pos of the original scene uniforms
     * (opengl33renderer.cpp:762-766) */
    PackedFloat64Array MoverRailVehicleWipers::get_sweep_positions() const {
        PackedFloat64Array sweep;
        for (const Wiper &wiper: wipers) {
            sweep.push_back(wiper.position > 0.0 && wiper.returning ? wiper.position + 1.0 : wiper.position);
        }
        return sweep;
    }

    void MoverRailVehicleWipers::_fill_state_dictionary(Dictionary &p_state) const {
        p_state["wipers_switch_position"] = get_switch_position();
        p_state["wiper_positions"] = get_sweep_positions();
    }

    void MoverRailVehicleWipers::_fill_config_dictionary(Dictionary &p_config) const {
        TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        p_config["wipers_switch_position_max"] = std::max(0, static_cast<int>(get_positions().size()) - 1);
        p_config["wipers_angle"] = get_angle();
    }
} // namespace godot
