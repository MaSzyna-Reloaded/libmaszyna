#include "MoverRailVehicleDoors.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include <algorithm>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void MoverRailVehicleDoors::_bind_methods() {}


    int MoverRailVehicleDoors::get_permit_preset() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.permit_preset : 0;
    }

    // the door permit preset switch has one position per FIZ permit preset (Train.cpp:7304)
    void MoverRailVehicleDoors::_fill_config_dictionary(Dictionary &p_config) const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        p_config["doors_permit_preset_max"] = std::max(0, static_cast<int>(mover->Doors.permit_presets.size()) - 1);
    }

    bool MoverRailVehicleDoors::get_locked() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.is_locked : false;
    }

    bool MoverRailVehicleDoors::get_lock_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.lock_enabled : false;
    }

    bool MoverRailVehicleDoors::get_step_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.step_enabled : false;
    }

    int MoverRailVehicleDoors::get_open_control() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.open_control : 0;
    }

    bool MoverRailVehicleDoors::get_left_open() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::left].is_open : false;
    }

    bool MoverRailVehicleDoors::get_left_door_closed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::left].is_door_closed : true;
    }

    bool MoverRailVehicleDoors::get_right_door_closed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::right].is_door_closed : true;
    }

    bool MoverRailVehicleDoors::get_left_closed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::left].is_closed : true;
    }

    bool MoverRailVehicleDoors::get_left_open_permit() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::left].open_permit : false;
    }

    bool MoverRailVehicleDoors::get_left_local_open() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::left].local_open : false;
    }

    bool MoverRailVehicleDoors::get_left_remote_open() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::left].remote_open : false;
    }

    double MoverRailVehicleDoors::get_left_position() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::left].position : 0.0;
    }

    double MoverRailVehicleDoors::get_left_position_normalized() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::left].position / get_max_shift() : 0.0;
    }

    bool MoverRailVehicleDoors::get_left_operating() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr
                       ? mover->Doors.instances[side::left].is_opening || mover->Doors.instances[side::left].is_closing
                       : false;
    }

    double MoverRailVehicleDoors::get_left_step_position() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::left].step_position : 0.0;
    }

    bool MoverRailVehicleDoors::get_left_step_operating() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::left].step_folding ||
                                          mover->Doors.instances[side::left].step_unfolding
                                : false;
    }

    bool MoverRailVehicleDoors::get_right_open() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::right].is_open : false;
    }

    bool MoverRailVehicleDoors::get_right_closed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::right].is_closed : true;
    }

    bool MoverRailVehicleDoors::get_right_open_permit() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::right].open_permit : false;
    }

    bool MoverRailVehicleDoors::get_right_local_open() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::right].local_open : false;
    }

    bool MoverRailVehicleDoors::get_right_remote_open() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::right].remote_open : false;
    }

    double MoverRailVehicleDoors::get_right_position() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::right].position : 0.0;
    }

    double MoverRailVehicleDoors::get_right_position_normalized() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::right].position / get_max_shift() : 0.0;
    }

    bool MoverRailVehicleDoors::get_right_operating() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::right].is_opening ||
                                          mover->Doors.instances[side::right].is_closing
                                : false;
    }

    double MoverRailVehicleDoors::get_right_step_position() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::right].step_position : 0.0;
    }

    bool MoverRailVehicleDoors::get_right_step_operating() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.instances[side::right].step_folding ||
                                          mover->Doors.instances[side::right].step_unfolding
                                : false;
    }

    void MoverRailVehicleDoors::_fill_state_dictionary(Dictionary &p_state) const {
        TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        p_state["doors_locked"] = get_locked();
        p_state["doors_permit_preset"] = get_permit_preset();
        p_state["doors_lock_enabled"] = get_lock_enabled();
        p_state["doors_step_enabled"] = get_step_enabled();
        p_state["doors_open_control"] = get_open_control();
        p_state["doors_left_open"] = get_left_open();
        p_state["doors_left_open_permit"] = get_left_open_permit();
        p_state["doors_left_local_open"] = get_left_local_open();
        p_state["doors_left_remote_open"] = get_left_remote_open();
        p_state["doors_left_position"] = get_left_position();
        p_state["doors_left_position_normalized"] = get_left_position_normalized();
        p_state["doors_left_operating"] = get_left_operating();
        p_state["doors_left_step_position"] = get_left_step_position();
        p_state["doors_left_step_operating"] = get_left_step_operating();
        p_state["doors_right_open"] = get_right_open();
        p_state["doors_right_open_permit"] = get_right_open_permit();
        p_state["doors_right_local_open"] = get_right_local_open();
        p_state["doors_right_remote_open"] = get_right_remote_open();
        p_state["doors_right_position"] = get_right_position();
        p_state["doors_right_position_normalized"] = get_right_position_normalized();
        p_state["doors_right_operating"] = get_right_operating();
        p_state["doors_right_step_position"] = get_right_step_position();
        p_state["doors_right_step_operating"] = get_right_step_operating();
        p_state["mirror_left_position"] = get_mirror_left_position();
        p_state["mirror_right_position"] = get_mirror_right_position();
        p_state["mirrors_forbidden"] = get_mirrors_forbidden();
        p_state["doors_departure_signal"] = get_departure_signal();
        p_state["doors_departure_signal_sounding"] = get_departure_signal_sounding();
        p_state["doors_remote_only"] = get_remote_only();
        // what the cab's door permit switches check (Train.cpp:7203, 7220)
        p_state["doors_permit_preset_count"] = static_cast<int>(mover->Doors.permit_presets.size());
        p_state["doors_open_with_permit_after"] = mover->DoorsOpenWithPermitAfter;
    }

    void MoverRailVehicleDoors::_do_process_component(const double p_delta) {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->update_doors(p_delta);
        const auto &left_door = p_mover->Doors.instances[side::left];
        const auto &right_door = p_mover->Doors.instances[side::right];
        if (left_door.is_open && !left_open) {
            emit_signal(doors_opened_signal, SIDE_LEFT);
        }
        if (left_door.is_closed && !left_closed) {
            emit_signal(doors_closed_signal, SIDE_LEFT);
        }
        if (right_door.is_open && !right_open) {
            emit_signal(doors_opened_signal, SIDE_RIGHT);
        }
        if (right_door.is_closed && !right_closed) {
            emit_signal(doors_closed_signal, SIDE_RIGHT);
        }
        left_open = left_door.is_open;
        left_closed = left_door.is_closed;
        right_open = right_door.is_open;
        right_closed = right_door.is_closed;

        // DynObj.cpp:4207-4236: the mirrors fold above MirrorVelClose, with no cab active or when
        // forbidden, and unfold on the side whose doors are permitted to open - a full travel per
        // second. The original folds them with no cab active whatever its InactiveCabFlag says.
        if (p_mover->Vel > p_mover->MirrorVelClose || p_mover->CabActive == 0 || p_mover->MirrorForbidden) {
            mirror_left_position = std::max(0.0, mirror_left_position - p_delta);
            mirror_right_position = std::max(0.0, mirror_right_position - p_delta);
            return;
        }
        if (p_mover->Doors.instances[side::left].open_permit) {
            mirror_left_position = std::min(1.0, mirror_left_position + p_delta);
        }
        if (p_mover->Doors.instances[side::right].open_permit) {
            mirror_right_position = std::min(1.0, mirror_right_position + p_delta);
        }
    }

    double MoverRailVehicleDoors::get_mirror_left_position() const {
        return mirror_left_position;
    }

    double MoverRailVehicleDoors::get_mirror_right_position() const {
        return mirror_right_position;
    }

    bool MoverRailVehicleDoors::get_mirrors_forbidden() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->MirrorForbidden : false;
    }

    // Train.cpp:7726 OnCommand_mirrorstoggle flips it on a press; the cab's two-state switch sets it
    void MoverRailVehicleDoors::forbid_mirrors(const bool p_state) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->MirrorForbidden = p_state;
    }

    void MoverRailVehicleDoors::next_permit_preset() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->ChangeDoorPermitPreset(1);
    }

    void MoverRailVehicleDoors::previous_permit_preset() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->ChangeDoorPermitPreset(-1);
    }

    void MoverRailVehicleDoors::permit_step(const bool p_state) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->PermitDoorStep(p_state);
    }

    void MoverRailVehicleDoors::operate_doors_locally(const bool p_state, const Side p_side) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->OperateDoors(p_side == Side::SIDE_LEFT ? side::left : side::right, p_state, range_t::local);
    }

    void MoverRailVehicleDoors::permit_doors(const bool p_state, const Side p_side) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->PermitDoors(p_side == Side::SIDE_LEFT ? side::left : side::right, p_state);
    }

    void MoverRailVehicleDoors::operate_doors(const bool p_state, const Side p_side) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->OperateDoors(p_side == Side::SIDE_LEFT ? side::left : side::right, p_state);
    }

    void MoverRailVehicleDoors::door_lock(const bool p_state) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->LockDoors(p_state);
    }

    void MoverRailVehicleDoors::door_remote_control(const bool p_state) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->ChangeDoorControlMode(p_state);
    }

    void MoverRailVehicleDoors::signal_departure(const bool p_state) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->signal_departure(p_state);
    }

    bool MoverRailVehicleDoors::get_departure_signal() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->DepartureSignal : false;
    }

    bool MoverRailVehicleDoors::get_departure_signal_sounding() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Doors.has_warning && mover->DepartureSignal &&
               (mover->Power24vIsAvailable || mover->Power110vIsAvailable);
    }

    bool MoverRailVehicleDoors::get_remote_only() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Doors.remote_only : false;
    }

    void MoverRailVehicleDoors::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        if (door_controls_map.find(get_open_method()) != door_controls_map.end()) {
            p_mover->Doors.open_control = door_controls_map.at(get_open_method());
        } else {
            log_error("Unhandled door open controls position: " + String::num(get_open_method()));
        }

        if (door_controls_map.find(get_close_method()) != door_controls_map.end()) {
            p_mover->Doors.close_control = door_controls_map.at(get_close_method());
        } else {
            log_error("Unhandled door close controls position: " + String::num(get_close_method()));
        }

        p_mover->Doors.auto_duration = get_open_time();
        p_mover->Doors.auto_velocity = get_close_auto_close_velocity();
        p_mover->Doors.auto_include_remote = get_close_auto_close_remote();
        p_mover->Doors.permit_needed = get_permit_required();
        p_mover->Doors.permit_presets.clear();
        for (int i = 0; i < get_permit_list().size(); i++) {
            if (get_permit_list()[i] != Variant()) {
                p_mover->Doors.permit_presets.emplace_back(static_cast<int>(get_permit_list()[i]));
            }
        }

        if (!p_mover->Doors.permit_presets.empty()) {
            p_mover->Doors.permit_preset = get_permit_default();
            p_mover->Doors.permit_preset =
                    std::min<int>(
                            static_cast<int>(p_mover->Doors.permit_presets.size()), p_mover->Doors.permit_preset) -
                    1;
        }

        p_mover->Doors.open_rate = get_open_speed();
        p_mover->Doors.open_delay = get_open_delay();
        p_mover->Doors.close_rate = get_close_speed();
        p_mover->Doors.close_delay = get_close_delay();
        p_mover->Doors.range = get_max_shift();
        p_mover->Doors.range_out = get_max_shift_plug();

        if (door_type_map.find(get_type()) != door_type_map.end()) {
            p_mover->Doors.type = door_type_map.at(get_type());
        } else {
            log_error("Unhandled door get_type(): " + String::num(get_type()));
        }

        p_mover->Doors.has_warning = get_close_warning();
        p_mover->Doors.has_autowarning = get_close_auto_close_warning();
        p_mover->Doors.has_lock = get_has_lock();
        bool const remote_control = {
                (get_open_method() == CONTROLS_DRIVER || get_open_method() == CONTROLS_CONDUCTOR ||
                 get_open_method() == CONTROLS_MIXED)};

        if (voltage_map.find(get_voltage()) != voltage_map.end()) {
            p_mover->Doors.voltage = voltage_map.at(get_voltage());
        } else {
            // Mover.cpp:10599 - remote-controlled doors run on 24 V by default
            p_mover->Doors.voltage = voltage_map.at(remote_control ? VOLTAGE_24 : VOLTAGE_0);
        }
        p_mover->Doors.step_rate = get_platform_speed();
        p_mover->Doors.step_range = get_platform_max_shift();

        if (door_platform_type_map.find(get_platform_type()) != door_platform_type_map.end()) {
            p_mover->Doors.step_type = door_platform_type_map.at(get_platform_type());
        }

        p_mover->MirrorMaxShift = get_mirror_max_shift();
        p_mover->MirrorVelClose = get_mirror_close_velocity();
        p_mover->DoorsOpenWithPermitAfter = get_open_with_permit();
        p_mover->DoorsPermitLightBlinking = get_permit_light_blinking();
    }
} // namespace godot
