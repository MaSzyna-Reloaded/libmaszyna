#include "MoverRailVehicleBrake.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include "utils/LibMaszynaUnits.hpp"
#include "utils/utils.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/rail/RailVehicleBrake.hpp"
#include <algorithm>
#include <cmath>
#include <godot_cpp/classes/gd_extension.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    namespace {
        /* LoadFIZ_Brake (Mover.cpp:10394-10475) */
        /* A pressure below it means the data gives none (Mover.cpp:10416, 10423) */
        constexpr double MIN_BRAKE_PRESSURE = 0.01;
        /* The control pipe starts slightly off its nominal pressure (Mover.cpp:10469) */
        constexpr double DEFAULT_CONTROL_PIPE_PRESSURE = 5.0;
        constexpr double CONTROL_PIPE_PRESSURE_JITTER = 0.001;
        constexpr double CONTROL_PIPE_PRESSURE_JITTER_RANGE = 10.0;
    } // namespace

    void MoverRailVehicleBrake::_bind_methods() {}


    void MoverRailVehicleBrake::alarm_chain(const bool p_pulled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        // Train.cpp:1839-1872 (OnCommand_alarmchaintoggle/enable/disable) ->
        // AlarmChainSwitch(State) - manual emergency brake pull cord.
        mover->AlarmChainSwitch(p_pulled);
    }

    // Original engine: TTrain::OnCommand_universalbrakebutton1..3 (Train.cpp:1897) -> UniversalBrakeButton()
    void MoverRailVehicleBrake::universal_brake_button(const int p_button, const bool p_pressed) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        mover->UniversalBrakeButton(p_button, p_pressed ? 1 : 0);
    }

    void MoverRailVehicleBrake::brake_releaser(const bool p_pressed) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        mover->BrakeReleaser(p_pressed ? 1 : 0);
    }

    // Original engine: simulation.cpp:184 consistreleaser - the valve's releaser, not
    // BrakeReleaser(): no cab conditions, nothing sent along the control line; held while its
    // button is held (vehicleparams.cpp:289-293)
    void MoverRailVehicleBrake::consist_releaser(const bool p_active) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        mover->Hamulec->Releaser(p_active ? 1 : 0);
    }

    void MoverRailVehicleBrake::compressor(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->CompressorSwitch(p_enabled);
    }

    void MoverRailVehicleBrake::brake_level_set(const double p_level) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        const double level = CLAMP(p_level, 0.0, 1.0);
        const double brake_controller_min = mover->Handle->GetPos(bh_MIN);
        const double brake_controller_max = mover->Handle->GetPos(bh_MAX);
        const double brake_controller_pos =
                brake_controller_min + (level * (brake_controller_max - brake_controller_min));
        mover->BrakeLevelSet(brake_controller_pos);
    }

    void MoverRailVehicleBrake::brake_level_set_position(const String &p_position) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        const std::unordered_map<std::string, int>::const_iterator it =
                brake_handle_position_string_map.find(std::string(p_position.utf8().get_data()));
        if (it != brake_handle_position_string_map.end()) {
            mover->BrakeLevelSet(mover->Handle->GetPos(it->second));
        } else {
            log_error("Unhandled brake level position: " + p_position);
        }
    }

    void MoverRailVehicleBrake::brake_level_increase() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        mover->BrakeLevelAdd(get_handle_step());
    }

    void MoverRailVehicleBrake::brake_level_decrease() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        mover->BrakeLevelAdd(-get_handle_step());
    }

    // Original engine: "localbrake:"/ggLocalBrake (Train.cpp:10026) is the independent/loco
    // brake handle - a draggable gauge like mainctrl/brakectrl, not a passive display. Real
    // input is OnCommand_independentbrakeincrease/decrease (Train.cpp:1447-1524), bound by
    // default to num_1/num_7 (eu07_input-keyboard.ini) - there was previously no equivalent
    // command in this wrapper at all, so the handle could never move.
    void MoverRailVehicleBrake::local_brake_set(const double p_level) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        // the original's independentbrakeset does not check (Train.cpp:1826), but it comes only from a
        // cab's localbrake: gauge; the keys go through independentbrakeincrease, refused on a vehicle
        // whose local brake is the hand wheel (Train.cpp:1724) - here the keys hold the handle by "set"
        if (mover->LocalBrake == TLocalBrake::ManualBrake) {
            return;
        }
        // LocalBrakePosA is already normalized 0..1 in the mover (unlike the main brake's
        // arbitrary Handle-position units), so no range conversion is needed here.
        mover->LocalBrakePosA = CLAMP(p_level, 0.0, 1.0);
    }

    void MoverRailVehicleBrake::local_brake_increase() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        // no independent brake on a vehicle whose local brake is the hand wheel (Train.cpp:1724)
        if (mover->LocalBrake == TLocalBrake::ManualBrake) {
            return;
        }
        // One notch per call, mirroring this wrapper's main_controller_increase(step=1)
        // convention for a single cab-click/command invocation.
        mover->IncLocalBrakeLevel(1);
    }

    void MoverRailVehicleBrake::local_brake_decrease() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        mover->DecLocalBrakeLevel(1);
    }

    // Original engine: TTrain::OnCommand_manualbrakeincrease/decrease (Train.cpp:1809-1837) - one notch,
    // only on a vehicle with a manual brake
    void MoverRailVehicleBrake::manual_brake_increase() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        if (mover->LocalBrake == TLocalBrake::ManualBrake || mover->MBrake) {
            mover->IncManualBrakeLevel(1);
        }
    }

    // Original engine: the per-vehicle part of TController::AutoRewident() (Driver.cpp:2193-2246) - brake
    // delay setting chosen for the train, manual and spring brake released without any power condition
    void MoverRailVehicleBrake::auto_rewident(const int p_brake_delay) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        mover->BrakeDelaySwitch(p_brake_delay);
        mover->DecManualBrakeLevel(ManualBrakePosNo);
        mover->SpringBrake.Activate = false;
    }

    // Original engine: TTrain::OnCommand_trainbrakeoperationmodeincrease (Train.cpp:2447-2460)
    void MoverRailVehicleBrake::brake_operation_mode_increase() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        if (((mover->BrakeOpModeFlag << 1) & mover->BrakeOpModes) != 0) {
            mover->BrakeOpModeFlag <<= 1;
        }
    }

    // Original engine: TTrain::OnCommand_trainbrakeoperationmodedecrease (Train.cpp:2463-2476)
    void MoverRailVehicleBrake::brake_operation_mode_decrease() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        if (((mover->BrakeOpModeFlag >> 1) & mover->BrakeOpModes) != 0) {
            mover->BrakeOpModeFlag >>= 1;
        }
    }

    bool MoverRailVehicleBrake::ep_brake(const bool p_applied) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover, false);
        return mover->SwitchEPBrake(p_applied ? 1 : 0);
    }

    int MoverRailVehicleBrake::get_operation_mode() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->BrakeOpModeFlag : 0;
    }

    // Original engine: TTrain::OnCommand_trainbrakecharging (Train.cpp:1686) - held, the handle stays in the
    // charging position -1; released, only self-returning EP handles go back to the running position
    // (zero_charging_train_brake(), Train.cpp:960), an FV4a stays where it is
    void MoverRailVehicleBrake::brake_level_charging(const bool p_active) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        if (p_active) {
            mover->BrakeLevelSet(-1);
            return;
        }
        if (mover->BrakeCtrlPos == -1 &&
            (mover->BrakeHandle == TBrakeHandle::FVel6 || mover->BrakeHandle == TBrakeHandle::MHZ_EN57 ||
             mover->BrakeHandle == TBrakeHandle::MHZ_K8P)) {
            mover->BrakeLevelSet(0);
        }
    }

    void MoverRailVehicleBrake::manual_brake_decrease() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        if (mover->LocalBrake == TLocalBrake::ManualBrake || mover->MBrake) {
            mover->DecManualBrakeLevel(1);
        }
    }

    void MoverRailVehicleBrake::_fill_config_dictionary(Dictionary &p_config) const {
        TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        // Hardware/setup facts - change only when the vehicle's brake config is (re)applied, not
        // every tick, so they belong here rather than in _do_fetch_state_from_mover. Read from
        // RailVehicleBrake's own already-bound properties (the authoring source of truth) rather than
        // re-deriving from mover internals a second time.
        p_config["brake_ep_enabled"] = mover->BrakeSystem == TBrakeSystem::ElectroPneumatic;
        p_config["brake_handle_type"] = get_cntrl_brake_handle_type();
        p_config["brake_local_handle_type"] = get_cntrl_local_brake_handle_type();
        p_config["brake_valve_type"] = get_valve_type();
        // available brake delay settings (bdelay_* flags) and main reservoir, used by MaszynaLegacyAutoRewident
        p_config["brake_delays"] = mover->BrakeDelays;
        // brakeopmode_sw's last position: the highest mode the vehicle has
        p_config["brake_operation_mode_position_max"] =
                mover->BrakeOpModes > 0 ? static_cast<int>(std::log2(mover->BrakeOpModes)) : 0;
        p_config["brake_main_reservoir_volume"] = mover->VeselVolume;
        // the train's brake system and its brake's delays [s], per delay setting (BDelay1-4)
        p_config["brake_system"] = get_cntrl_brake_system();
        p_config["brake_delay_times"] = PackedFloat64Array(
                {get_cntrl_brake_delay_1(), get_cntrl_brake_delay_2(), get_cntrl_brake_delay_3(),
                 get_cntrl_brake_delay_4()});
        // LocHandle is unconditionally non-null after mover init (Mover.cpp's own switch always
        // assigns a TDriverHandle default), so "!= nullptr" never actually distinguishes "has a
        // real local handle" from "has none" - get_cntrl_local_brake_handle_type() is the real signal.
        p_config["brake_local_handle_available"] = get_cntrl_local_brake_handle_type() != BRAKE_HANDLE_TYPE_NO_HANDLE;
        p_config["brake_max_cylinder_pressure"] = get_max_cylinder_pressure();
        p_config["brake_max_control_pressure"] =
                get_max_aux_pressure() >= MIN_BRAKE_PRESSURE ? get_max_aux_pressure() : get_max_cylinder_pressure();

        if (mover->Handle == nullptr) {
            return;
        }
        p_config["brakes_controller_position_min"] = get_handle_position(HANDLE_POSITION_MIN);
        p_config["brakes_controller_position_max"] = get_handle_position(HANDLE_POSITION_MAX);
        // the handle's named positions, per its type (TFV4aM::pos_table, hamulce.h:1140)
        p_config["brakes_controller_position_filling"] = get_handle_position(HANDLE_POSITION_FILLING);
        p_config["brakes_controller_position_drive"] = get_handle_position(HANDLE_POSITION_DRIVE);
        p_config["brakes_controller_position_cutoff"] = get_handle_position(HANDLE_POSITION_CUTOFF);
        p_config["brakes_controller_position_first_step"] = get_handle_position(HANDLE_POSITION_FIRST_STEP);
        p_config["brakes_controller_position_full"] = get_handle_position(HANDLE_POSITION_FULL);
        p_config["brakes_controller_position_emergency"] = get_handle_position(HANDLE_POSITION_EMERGENCY);
        // the electro-pneumatic range: releasing, holding and full braking (bh_EPR/EPN/EPB); a
        // handle whose holding equals releasing works the EP brake by a switch (hamulce.h:173-177)
        p_config["brakes_controller_position_ep_release"] = get_handle_position(HANDLE_POSITION_EP_RELEASE);
        p_config["brakes_controller_position_ep_hold"] = get_handle_position(HANDLE_POSITION_EP_HOLD);
        p_config["brakes_controller_position_ep_brake"] = get_handle_position(HANDLE_POSITION_EP_BRAKE);
        // the EP brake is applied by how long the handle is held (TDriverHandle::TimeEP)
        p_config["brake_handle_ep_time_controlled"] = get_handle_ep_time_controlled();
        // a key moves the handle a position per press, or while held (Train.cpp:1960-1966)
        p_config["brake_handle_movement"] = get_handle_movement();
        // a handle that sets the pipe pressure by how long it is held, not by where it stands
        // (TDriverHandle::Time, hamulce.cpp: MHZ_K5P, MHZ_6P, M394, H14K1, St113, H1405)
        p_config["brake_handle_time_controlled"] = get_handle_time_controlled();
        // the pipe's running pressure and its working range (HighPipePress, DeltaPipePress)
        p_config["brake_pipe_pressure_high"] = mover->HighPipePress;
        p_config["brake_pipe_pressure_delta"] = mover->DeltaPipePress;
    }

    void MoverRailVehicleBrake::_do_process_component(const double p_delta) {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        // GetSoundFlag() clears the flag it returns, so it is read once per tick, here, and never
        // in a getter (DynObj.cpp:4718-4723)
        if (p_mover->Hamulec && (p_mover->Hamulec->GetSoundFlag() & Maszyna::sf_Acc) != 0) {
            emit_signal(accelerator_activated_signal);
        }
    }

    bool MoverRailVehicleBrake::get_alarm_chain_pulled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->AlarmChainFlag : false;
    }

    double MoverRailVehicleBrake::get_air_pressure() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->BrakePress : 0.0;
    }

    double MoverRailVehicleBrake::get_loco_pressure() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->LocBrakePress : 0.0;
    }

    double MoverRailVehicleBrake::get_pipe_brake_pressure() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->PipeBrakePress : 0.0;
    }

    double MoverRailVehicleBrake::get_pipe_pressure() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->PipePress : 0.0;
    }

    double MoverRailVehicleBrake::get_feed_pipe_pressure() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->ScndPipePress : 0.0;
    }

    double MoverRailVehicleBrake::get_tank_volume() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Volume : 0.0;
    }

    double MoverRailVehicleBrake::get_compressor_pressure() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Compressor : 0.0;
    }

    bool MoverRailVehicleBrake::get_compressor_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->CompressorFlag : false;
    }

    bool MoverRailVehicleBrake::get_compressor_allowed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->CompressorAllow : false;
    }

    double MoverRailVehicleBrake::get_controller_position() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->fBrakeCtrlPos : 0.0;
    }

    double MoverRailVehicleBrake::get_controller_position_normalized() const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return 0.0;
        }
        const double minimum = mover->Handle->GetPos(bh_MIN);
        const double maximum = mover->Handle->GetPos(bh_MAX);
        if (maximum == minimum) {
            return 0.0;
        }
        return (mover->fBrakeCtrlPos - minimum) / (maximum - minimum);
    }

    double MoverRailVehicleBrake::get_local_position_normalized() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->LocalBrakePosA : 0.0;
    }

    int MoverRailVehicleBrake::get_manual_position() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->ManualBrakePos : 0;
    }

    double MoverRailVehicleBrake::get_unit_force() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->UnitBrakeForce : 0.0;
    }

    double MoverRailVehicleBrake::get_force_ratio() const {
        TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return 0.0;
        }
        const double max_per_block =
                mover->BrakeForceR(1.0, mover->Vel) / (std::max(1, mover->NAxles) * std::max(1, mover->NBpA));
        return std::clamp(mover->UnitBrakeForce / std::max(1.0, max_per_block), 0.0, 1.0);
    }

    double MoverRailVehicleBrake::get_emergency_valve_flow() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->EmergencyValveFlow : 0.0;
    }

    double MoverRailVehicleBrake::get_main_valve_flow() const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr || !std::isfinite(mover->dpMainValve)) {
            return 0.0;
        }
        return mover->dpMainValve;
    }

    double MoverRailVehicleBrake::get_local_valve_flow() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->dpLocalValve : 0.0;
    }

    double MoverRailVehicleBrake::get_handle_braking_flow() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Handle ? mover->Handle->GetSound(Maszyna::s_fv4a_b) : 0.0;
    }

    double MoverRailVehicleBrake::get_handle_release_flow() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Handle ? mover->Handle->GetSound(Maszyna::s_fv4a_u) : 0.0;
    }

    double MoverRailVehicleBrake::get_handle_emergency_flow() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Handle ? mover->Handle->GetSound(Maszyna::s_fv4a_e) : 0.0;
    }

    double MoverRailVehicleBrake::get_handle_control_chamber_flow() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Handle ? mover->Handle->GetSound(Maszyna::s_fv4a_x) : 0.0;
    }

    double MoverRailVehicleBrake::get_handle_timing_reservoir_flow() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Handle ? mover->Handle->GetSound(Maszyna::s_fv4a_t) : 0.0;
    }

    double MoverRailVehicleBrake::get_control_pressure() const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr || !mover->LocHandle) {
            return 0.0;
        }
        return mover->LocHandle->GetCP();
    }

    double MoverRailVehicleBrake::get_handle_control_pressure() const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr || !mover->Handle) {
            return 0.0;
        }
        return mover->Handle->GetCP();
    }

    double MoverRailVehicleBrake::get_local_aeim_position() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->LocalBrakePosAEIM : 0.0;
    }

    double MoverRailVehicleBrake::get_edb_cylinder_pressure() const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr || !mover->Hamulec) {
            return 0.0;
        }
        return mover->Hamulec->GetEDBCP();
    }

    bool MoverRailVehicleBrake::get_releaser_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Hamulec && mover->Hamulec->Releaser() : false;
    }

    bool MoverRailVehicleBrake::get_main_pipe_locked() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->LockPipe : false;
    }

    double MoverRailVehicleBrake::get_force() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Fb : 0.0;
    }

    double MoverRailVehicleBrake::get_force_at(const double p_ratio, const double p_velocity) const {
        // BrakeForceR() computes and changes nothing, but is not declared const
        TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->BrakeForceR(p_ratio, p_velocity) : 0.0;
    }

    bool MoverRailVehicleBrake::is_braking() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Hamulec && (mover->Hamulec->GetBrakeStatus() & Maszyna::b_on) != 0;
    }

    bool MoverRailVehicleBrake::is_holding() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Hamulec && (mover->Hamulec->GetBrakeStatus() & Maszyna::b_hld) != 0;
    }

    bool MoverRailVehicleBrake::is_cut_off() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Hamulec && (mover->Hamulec->GetBrakeStatus() & Maszyna::b_dmg) != 0;
    }

    int MoverRailVehicleBrake::get_delay_setting() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->BrakeDelayFlag : 0;
    }

    double MoverRailVehicleBrake::get_control_reservoir_pressure() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Hamulec ? mover->Hamulec->GetCRP() : 0.0;
    }

    double MoverRailVehicleBrake::get_handle_position(const HandlePosition p_position) const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Handle ? mover->Handle->GetPos(static_cast<int>(p_position)) : 0.0;
    }

    bool MoverRailVehicleBrake::get_handle_time_controlled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Handle && mover->Handle->Time;
    }

    bool MoverRailVehicleBrake::get_handle_ep_time_controlled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Handle && mover->Handle->TimeEP;
    }

    void MoverRailVehicleBrake::_fill_state_dictionary(Dictionary &p_state) const {
        TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        p_state["alarm_chain_pulled"] = get_alarm_chain_pulled();
        p_state["brake_air_pressure"] = get_air_pressure();
        p_state["brake_loco_pressure"] = get_loco_pressure();
        p_state["brake_pipe_pressure"] = get_pipe_brake_pressure();
        p_state["pipe_pressure"] = get_pipe_pressure();
        p_state["feed_pipe_pressure"] = get_feed_pipe_pressure();
        p_state["brake_tank_volume"] = get_tank_volume();
        p_state["compressor_pressure"] = get_compressor_pressure();
        p_state["compressor_enabled"] = get_compressor_enabled();
        p_state["compressor_allowed"] = get_compressor_allowed();
        p_state["brake_controller_position"] = get_controller_position();
        p_state["brake_controller_position_normalized"] = get_controller_position_normalized();
        p_state["brake_local_position_normalized"] = get_local_position_normalized();
        p_state["brake_manual_position"] = get_manual_position();
        p_state["brake_unit_force"] = get_unit_force();
        p_state["brake_force_ratio"] = get_force_ratio();
        p_state["brake_emergency_valve_flow"] = get_emergency_valve_flow();
        p_state["brake_main_valve_flow"] = get_main_valve_flow();
        p_state["brake_local_valve_flow"] = get_local_valve_flow();
        p_state["brake_handle_braking_flow"] = get_handle_braking_flow();
        p_state["brake_handle_release_flow"] = get_handle_release_flow();
        p_state["brake_handle_emergency_flow"] = get_handle_emergency_flow();
        p_state["brake_handle_control_chamber_flow"] = get_handle_control_chamber_flow();
        p_state["brake_handle_timing_reservoir_flow"] = get_handle_timing_reservoir_flow();
        p_state["brake_control_pressure"] = get_control_pressure();
        p_state["brake_handle_control_pressure"] = get_handle_control_pressure();
        p_state["brake_local_aeim_position"] = get_local_aeim_position();
        p_state["brake_edb_cylinder_pressure"] = get_edb_cylinder_pressure();
        p_state["brake_releaser_active"] = get_releaser_active();
        p_state["brake_operation_mode"] = get_operation_mode();
        // what brakeopmode_sw shows: the mode's bit (Train.cpp:2457)
        p_state["brake_operation_mode_position"] =
                get_operation_mode() > 0 ? static_cast<int>(std::log2(get_operation_mode())) : 0;
        p_state["main_pipe_locked"] = get_main_pipe_locked();
        p_state["brake_force"] = get_force();
        p_state["brake_is_braking"] = is_braking();
        p_state["brake_is_holding"] = is_holding();
        p_state["brake_is_cut_off"] = is_cut_off();
        // the delay setting in use, a BrakeDelaySetting (BrakeDelayFlag)
        p_state["brake_delay_setting"] = get_delay_setting();
        // the distributor's control reservoir (GetCRP())
        p_state["brake_control_reservoir_pressure"] = get_control_reservoir_pressure();
        // the control pipe (CntrlPipePress) and the distributor's brake reservoir (GetBRP()),
        // Train.cpp:8677-8678
        p_state["brake_control_pipe_pressure"] = mover->CntrlPipePress;
        p_state["brake_reservoir_pressure"] = mover->Hamulec ? mover->Hamulec->GetBRP() : 0.0;
    }

    void MoverRailVehicleBrake::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        /* logika z Mover::LoadFiz_Brake */
        p_mover->BrakeSystem = brake_system_type_map.at(get_cntrl_brake_system()); // BrakeSystem
        p_mover->BrakeCtrlPosNo = get_cntrl_brake_ctrl_position_count();           // BCPN
        p_mover->BrakeDelay[0] = get_cntrl_brake_delay_1();                        // BDelay1
        p_mover->BrakeDelay[1] = get_cntrl_brake_delay_2();                        // BDelay2
        p_mover->BrakeDelay[2] = get_cntrl_brake_delay_3();                        // BDelay3
        p_mover->BrakeDelay[3] = get_cntrl_brake_delay_4();                        // BDelay4
        p_mover->BrakeDelays = get_cntrl_brake_delays();                           // BrakeDelays
        // MaxBPMass, in tonnes in the data and only then in kilograms (Mover.cpp:10771-10775)
        if (get_cntrl_max_brake_pressure_mass() > 0.0) {
            p_mover->MBPM =
                    get_cntrl_max_brake_pressure_mass() * LibMaszynaUnits::KILOGRAMS_PER_TONNE; // Mover.cpp:10774
        }
        p_mover->BrakeOpModes = get_cntrl_brake_op_modes();                                      // BrakeOpModes
        p_mover->BrakeHandle = brake_handle_type_map.at(get_cntrl_brake_handle_type());          // BrakeHandle
        p_mover->BrakeLocHandle = brake_handle_type_map.at(get_cntrl_local_brake_handle_type()); // LocBrakeHandle
        // ASB - a vehicle without a train brake handle takes only "yes" (Mover.cpp:10794-10815)
        p_mover->ASBType =
                get_cntrl_brake_ctrl_position_count() > 0 || get_cntrl_anti_skid_brake_type() == ANTI_SKID_BRAKE_YES
                        ? anti_skid_brake_type_map.at(get_cntrl_anti_skid_brake_type())
                        : 0;
        p_mover->LocalBrake = local_brake_type_map.at(get_cntrl_local_brake_type()); // LocalBrake
        p_mover->MBrake = get_cntrl_manual_brake_present();                          // ManualBrake
        p_mover->LocHandleTimeTraxx = get_cntrl_local_brake_traxx();                 // LocalBrakeTraxx
        p_mover->DynamicBrakeType = get_cntrl_dynamic_brake_type();                  // DynamicBrake
        p_mover->ReleaseParkingBySpringBrake = get_cntrl_release_parking_by_spring_brake();
        p_mover->ReleaseParkingBySpringBrakeWhenDoorIsOpen = get_cntrl_release_parking_by_spring_brake_when_door_open();
        p_mover->SpringBrakeCutsOffDrive = get_cntrl_spring_brake_cuts_off_drive();
        p_mover->SpringBrakeDriveEmergencyVel = get_cntrl_spring_brake_drive_emergency_velocity();

        /* FIXME: BrakeValve nie jest tylko enumem, jesli w FIZ wpisze sie nieznany symbol zawierający ESt, to EXE
         * ustawi BrakeValve=ESt3. Powinien to ogarnąć importer FIZ
         *
         * Whoever thought making BrakeValve half-enum, half-parser-voodoo was a good idea
         * condemned everyone else to cargo-cult their bugs. Thanks a lot, dear original MaSzyna code authors.
         */

        // assuming same int values between our TrainBrakeValve and mover's TBrakeValve
        p_mover->BrakeValve = static_cast<TBrakeValve>(static_cast<int>(get_valve_type()));
        p_mover->BrakeValveParams = get_valve_parameters().utf8().get_data();

        const std::unordered_map<TBrakeValve, TBrakeSubSystem>::const_iterator it =
                brake_valve_to_subsystem_map.find(p_mover->BrakeValve);
        p_mover->BrakeSubsystem = it != brake_valve_to_subsystem_map.end() ? it->second : TBrakeSubSystem::ss_None;

        p_mover->NBpA = get_friction_elements_per_axle();
        p_mover->MaxBrakeForce = get_brake_force_max();
        p_mover->BrakeValveSize = get_est_valve_size();
        p_mover->TrackBrakeForce =
                get_brake_force_traction() * LibMaszynaUnits::NEWTONS_PER_KILONEWTON; // Mover.cpp:10405
        p_mover->MaxBrakePress[3] = get_max_cylinder_pressure();
        if (get_max_cylinder_pressure() > 0.0) {
            p_mover->BrakeCylNo = get_cylinder_count();

            if (get_cylinder_count() > 0) {
                p_mover->MaxBrakePress[0] = get_max_aux_pressure() < MIN_BRAKE_PRESSURE ? get_max_cylinder_pressure()
                                                                                        : get_max_aux_pressure();
                p_mover->MaxBrakePress[1] = get_max_tare_pressure();
                p_mover->MaxBrakePress[2] = get_max_medium_pressure();
                p_mover->MaxBrakePress[4] =
                        get_max_antislip_pressure() < MIN_BRAKE_PRESSURE ? 0.0 : get_max_antislip_pressure();

                p_mover->BrakeCylRadius = get_cylinder_radius();
                p_mover->BrakeCylDist = get_cylinder_distance();
                p_mover->BrakeCylSpring = get_cylinder_spring_force();
                p_mover->BrakeSlckAdj = get_piston_stroke_adjuster_resistance();
                p_mover->BrakeRigEff = get_rig_effectiveness();

                p_mover->BrakeCylMult[0] = get_cylinder_gear_ratio();
                p_mover->BrakeCylMult[1] = get_cylinder_gear_ratio_low();
                p_mover->BrakeCylMult[2] = get_cylinder_gear_ratio_high();

                // Mover.cpp:10438 - kN/bar
                p_mover->P2FTrans = LibMaszynaUnits::KILOPASCALS_PER_BAR * M_PI * std::pow(get_cylinder_radius(), 2);

                p_mover->LoadFlag = (get_cylinder_gear_ratio_low() > 0.0 || get_max_tare_pressure() > 0.0) ? 1 : 0;

                p_mover->BrakeVolume =
                        M_PI * std::pow(get_cylinder_radius(), 2) * get_cylinder_distance() * get_cylinder_count();
                p_mover->BrakeVVolume = get_tank_volume_aux();

                p_mover->BrakeMethod = brake_method_map.at(get_brake_method()); // BM (Mover.cpp:10469)
                p_mover->RapidMult = get_rapid_transfer();
                p_mover->RapidVel = get_rapid_switching_speed();
            }
        } else {
            p_mover->P2FTrans = 0;
        }

        // Mover.cpp:10469-10471 - the control pipe starts slightly off its nominal pressure, unless
        // the FIZ gives HiPP (pipe_pressure_max 0 means it does not), and the high pressure is that
        p_mover->CntrlPipePress =
                get_pipe_pressure_max() > 0.0
                        ? get_pipe_pressure_max()
                        : DEFAULT_CONTROL_PIPE_PRESSURE +
                                  (CONTROL_PIPE_PRESSURE_JITTER *
                                   (UtilityFunctions::randf_range(0.0, CONTROL_PIPE_PRESSURE_JITTER_RANGE) -
                                    UtilityFunctions::randf_range(0.0, CONTROL_PIPE_PRESSURE_JITTER_RANGE)));
        p_mover->HighPipePress = p_mover->CntrlPipePress;
        p_mover->LowPipePress = get_pipe_pressure_min();
        // Mover.cpp:10474 - LoadFIZ derives it; the time-controlled handles of the AI steer by it
        p_mover->DeltaPipePress = p_mover->HighPipePress - p_mover->LowPipePress;
        p_mover->VeselVolume = get_tank_volume_main();
        p_mover->MinCompressor = get_compressor_cab_a_min_pressure();
        p_mover->MaxCompressor = get_compressor_cab_a_max_pressure();
        p_mover->MinCompressor_cabB = get_compressor_cab_b_min_pressure();
        p_mover->MaxCompressor_cabB = get_compressor_cab_b_max_pressure();

        p_mover->CompressorTankValve = get_compressor_tank_valve_active();
        p_mover->EmergencyValveOff = get_compressor_lower_emergency_closing_pressure();
        p_mover->EmergencyValveOn = get_compressor_higher_emergency_closing_pressure();

        p_mover->EmergencyValveArea = get_compressor_emergency_valve_area();
        p_mover->UniversalBrakeButtonFlag[0] = get_universal_brake_button_1();
        p_mover->UniversalBrakeButtonFlag[1] = get_universal_brake_button_2();
        p_mover->UniversalBrakeButtonFlag[2] = get_universal_brake_button_3();

        p_mover->LockPipeOn = get_main_pipe_blocking_pressure();
        p_mover->LockPipeOff = get_main_pipe_unblocking_pressure();
        p_mover->HandleUnlock = get_main_pipe_minimum_unblocking_handle_position();
        p_mover->EmergencyCutsOffHandle = main_pipe_emergency_cuts_off_handle;

        p_mover->CompressorSpeed = get_compressor_speed();
        p_mover->CompressorPower = get_compressor_power();

        // According to the original code - the parameter is provided in the form of a multiplier, where 1.0 means the
        // default rate of 0.01
        p_mover->AirLeakRate = get_air_leak_multiplier() * 0.01;

        // By default, this should be set to true if an engine type is diesel or diesel-electric and false, otherwise
        //  this action should be performed by FIZ parser
        p_mover->ReleaserEnabledOnlyAtNoPowerPos = get_releaser_enabled_only_at_no_power_pos();
        if (p_mover->MinCompressor_cabB > 0.0) {
            p_mover->MinCompressor_cabA = p_mover->MinCompressor;
            p_mover->CabDependentCompressor = true;
        } else {
            p_mover->MinCompressor_cabB = p_mover->MinCompressor;
        }
        if (p_mover->MaxCompressor_cabB > 0.0) {
            p_mover->MaxCompressor_cabA = p_mover->MaxCompressor;
            p_mover->CabDependentCompressor = true;
        } else {
            p_mover->MaxCompressor_cabB = p_mover->MaxCompressor;
        }

        /* BPT: tabelka hamulcowa, wyszczegolnienie cisnien w rurze wg pozycji krana */
        p_mover->BrakePressureTable.clear();
        for (int i = 0; i < get_brake_pressure_table().size(); i++) {
            const Ref<RailVehicleBrakePressureTableItem> &row = get_brake_pressure_table()[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[RailVehicleBrake]: get_brake_pressure_table() property is null at index " + String::num(i));
                continue;
            }
            Maszyna::TBrakePressure entry;
            entry.PipePressureVal = row->get_pipe_pressure();
            entry.BrakePressureVal = row->get_brake_cylinder_pressure();
            entry.FlowSpeedVal = row->get_fill_speed();
            entry.BrakeType = brake_pressure_table_type_map.at(row->get_brake_type());
            p_mover->BrakePressureTable[row->get_handle_position()] = entry;
        }

        /* CompressorList: programator sprezarek */
        constexpr int MAX_COMPRESSOR_LIST = 8;
        const int compressor_list_size = static_cast<int>(get_compressor_list().size());
        if (compressor_list_size > MAX_COMPRESSOR_LIST) {
            UtilityFunctions::push_warning(
                    "[RailVehicleBrake]: get_compressor_list() has " + String::num(compressor_list_size) +
                    " entries, exceeding the mover's limit of " + String::num(MAX_COMPRESSOR_LIST) + "; truncating.");
        }
        for (int i = 0; i < std::min(MAX_COMPRESSOR_LIST, compressor_list_size); i++) {
            const Ref<RailVehicleCompressorListItem> &row = get_compressor_list()[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[RailVehicleBrake]: get_compressor_list() property is null at index " + String::num(i));
                continue;
            }
            p_mover->CompressorList[Maszyna::TCompressorList::cl_Allow][i + 1] = row->get_allow();
            p_mover->CompressorList[Maszyna::TCompressorList::cl_SpeedFactor][i + 1] = row->get_speed_factor();
            p_mover->CompressorList[Maszyna::TCompressorList::cl_MinFactor][i + 1] = row->get_min_pressure_factor();
            p_mover->CompressorList[Maszyna::TCompressorList::cl_MaxFactor][i + 1] = row->get_max_pressure_factor();
        }
    }
} // namespace godot
