#include "MoverDriveUnit.hpp"
#include "legacy/vehicles/MaszynaMoverVehicleServer.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include "utils/LibMaszynaUnits.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleEngine.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include <algorithm>

namespace godot {
    bool MoverDriveUnit::get_main_switch_enabled() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->Mains : false;
    }

    bool MoverDriveUnit::get_main_switch_closable() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->MainSwitchCheck() : false;
    }

    double MoverDriveUnit::get_motor_torque() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->Mm : 0.0;
    }

    double MoverDriveUnit::get_wheel_torque() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->Mw : 0.0;
    }

    double MoverDriveUnit::get_wheel_force() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->Fw : 0.0;
    }

    double MoverDriveUnit::get_tractive_force() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->Ft : 0.0;
    }


    double MoverDriveUnit::get_power() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->EnginePower : 0.0;
    }

    double MoverDriveUnit::get_rpm_count() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->enrot : 0.0;
    }

    double MoverDriveUnit::get_angle() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->eAngle : 0.0;
    }

    double MoverDriveUnit::get_rpm_ratio() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->EngineRPMRatio() : 0.0;
    }

    double MoverDriveUnit::get_circuit_nmax_rpm() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->nmax * LibMaszynaUnits::SECONDS_PER_MINUTE : 0.0;
    }

    int MoverDriveUnit::get_damage() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->EngDmgFlag : 0;
    }

    double MoverDriveUnit::get_main_switch_time() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->MainsInitTimeCountdown : 0.0;
    }

    bool MoverDriveUnit::get_main_no_power_pos() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->IsMainCtrlNoPowerPos() : false;
    }

    bool MoverDriveUnit::get_motor_overload_relay_high_threshold() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->MotorOverloadRelayHighThreshold : false;
    }

    double MoverDriveUnit::get_eimic_real() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->eimic_real : 0.0;
    }

    bool MoverDriveUnit::get_relay_novolt() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->NoVoltRelay : false;
    }

    bool MoverDriveUnit::get_relay_overvoltage() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->OvervoltageRelay : false;
    }

    bool MoverDriveUnit::get_relay_ground() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->GroundRelay : false;
    }

    int MoverDriveUnit::get_circuit_rlist_size() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->RlistSize : 0;
    }

    double MoverDriveUnit::get_current(const int p_ammeter) const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->ShowCurrent(p_ammeter) : 0.0;
    }

    void MoverDriveUnit::apply_configuration(const RailVehicleEngine *p_engine) const {
        TMoverParameters *p_mover = owner.get_mover();
        p_mover->EngineType = MaszynaMoverVehicleServer::engine_type_to_mover(p_engine->get_type());

        p_mover->Transmision.NToothM = p_engine->get_transmission_gear_teeth_motor();
        p_mover->Transmision.NToothW = p_engine->get_transmission_gear_teeth_wheel();
        // Original engine: LoadFIZ_Engine (Mover.cpp) derives Ratio from the teeth counts
        // itself right after parsing "Trans=" - NToothM/NToothW alone are never read anywhere
        // else in Mover.cpp. Without this, Transmision.Ratio stays at its compiled default
        // (1.0), silently dropping the real gear ratio out of Mw/Fw/Ft (ElectricSeriesMotor
        // case, Mover.cpp ~line 5791) and undertractioning every geared vehicle.
        p_mover->Transmision.Ratio = p_engine->get_transmission_ratio();
        p_mover->Transmision.Efficiency = p_engine->get_transmission_efficiency();
        p_mover->Ftmax = p_engine->get_maximum_traction_force();
        p_mover->HasControlPressureSwitch = p_engine->get_pressure_switch_present();
        p_mover->MainsInitTime = p_engine->get_main_init_time();
        p_mover->InvertersNo = p_engine->get_inverters_count();
        for (auto &fan: p_mover->MotorBlowers) {
            fan.speed = static_cast<float>(p_engine->get_motor_blowers_speed());
            fan.sustain_time = static_cast<float>(p_engine->get_motor_blowers_sustain_time());
            fan.min_start_velocity = static_cast<float>(p_engine->get_motor_blowers_start_velocity());
            fan.start_type = MaszynaMoverVehicleServer::start_mode_to_mover(p_engine->get_motor_blowers_start_mode());
        }

        p_mover->EIMCtrlAdditionalZeros = p_engine->get_cntrl_eim_control_additional_zeros();
        p_mover->EIMCtrlEmergency = p_engine->get_cntrl_eim_control_emergency();
        p_mover->EIMCtrlType = p_engine->get_cntrl_eim_control_type();
        p_mover->AutoRelayType = p_engine->get_cntrl_auto_relay_mode();
        p_mover->HasCamshaft = p_engine->get_cntrl_has_camshaft();
        p_mover->ScndS = p_engine->get_cntrl_series_shunt_on_series_position();
        p_mover->FastSerialCircuit = static_cast<int>(p_engine->get_cntrl_fast_series_circuit());

        // Original engine: GroundRelay/NoVoltRelay/OvervoltageRelay/DamageFlag/EngDmgFlag/
        // ConvOvldFlag are all live, self-computed Mover state (relay checks recomputed every
        // Update() tick from real voltage/current, e.g. the ElectricSeriesMotor NoVoltRelay/
        // OvervoltageRelay block, Mover.cpp ~5627) and already default to their "healthy" values
        // in TMoverParameters's own constructor (MOVER.h:1590/1600/1601/401/1517/1518/1589).
        // This method reruns on every dirty-flag config reapply (not just once at startup - see
        // [[p_mover-parity-check]]), so force-resetting them here on every rerun was silently
        // wiping real relay trips/damage the simulation had legitimately produced since the last
        // reapply - matching the "traction voltage drops and the engine cuts out, needs the main
        // switch re-engaged" symptom (NoVoltRelay/OvervoltageRelay flip Mains off for real, then
        // a later config reapply cosmetically closes the relay again without also restoring
        // Mains, so the panel looks fine but the loco is still dead).

        /* motor param table */
        constexpr int MAX = Maszyna::MotorParametersArraySize;
        for (int i = 0; i < std::min(MAX, static_cast<int>(p_engine->get_motor_param_table().size())); i++) {
            const Ref<RailVehicleMotorParameter> &row = p_engine->get_motor_param_table()[i];
            if (row == nullptr || !row.is_valid() || row.is_null()) {
                UtilityFunctions::push_warning(
                        "[RailVehicleEngine]: p_engine->get_motor_param_table() property is null at index " +
                        String::num(i));
                return;
            }

            p_mover->MotorParam[i].mIsat = row->get_saturation_current_multiplier();
            p_mover->MotorParam[i].fi = row->get_voltage_constant();
            p_mover->MotorParam[i].mfi = row->get_voltage_constant_multiplier();
            p_mover->MotorParam[i].Isat = row->get_saturation_current();
            // a gear or a field shunt the controller passes by itself (readMPT0/readMPTDieselEngine)
            p_mover->MotorParam[i].AutoSwitch = row->get_auto_switch();
            // readMPT0's default case (Mover.cpp:8948, what "MotorParamTable0:" rows actually go
            // through) reads these two as real columns, unlike readMPTElectricSeries - see
            // FizTrainEngineCommon.parse_motor_param_row's doc comment for the full story. fi0 in
            // particular feeds Current()'s back-EMF term (Mover.cpp:389, "U1 = U + Mn*n*fi0*fi"),
            // so leaving it at TMotorParameters's compiled-zero default here (matching the
            // never-set case for the OTHER reader) would silently kill that back-EMF term.
            p_mover->MotorParam[i].mfi0 = row->get_initial_voltage_constant_multiplier();
            p_mover->MotorParam[i].fi0 = row->get_initial_voltage_constant();
            p_mover->MPTRelay[i].Iup = row->get_shunting_up();     // bocznikowanie
            p_mover->MPTRelay[i].Idown = row->get_shunting_down(); // bocznikowanie;
        }
    }

    void MoverDriveUnit::fill_config(Dictionary &p_config) const {
        TMoverParameters *p_mover = owner.get_mover();
        if (p_mover == nullptr) {
            return;
        }
        p_config["transmission_ratio"] = p_mover->Transmision.Ratio;
    }

    bool MoverDriveUnit::main_switch(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->MainSwitch(p_enabled) : false;
    }

    bool MoverDriveUnit::get_motor_blowers_enabled(const RailVehicleController::CouplerEnd p_end) const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr && p_mover->MotorBlowers[p_end].is_enabled;
    }

    bool MoverDriveUnit::get_motor_blowers_disabled(const RailVehicleController::CouplerEnd p_end) const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr && p_mover->MotorBlowers[p_end].is_disabled;
    }

    bool MoverDriveUnit::get_motor_blowers_active(const RailVehicleController::CouplerEnd p_end) const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr && p_mover->MotorBlowers[p_end].is_active;
    }

    // the coupler's end is the Mover's end (end::front 0, end::rear 1)
    void MoverDriveUnit::motor_blowers(const bool p_enabled, const RailVehicleController::CouplerEnd p_end) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->MotorBlowersSwitch(p_enabled, static_cast<Maszyna::end>(p_end));
    }

    void MoverDriveUnit::motor_blowers_switch_off(
            const bool p_enabled, const RailVehicleController::CouplerEnd p_end) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->MotorBlowersSwitchOff(p_enabled, static_cast<Maszyna::end>(p_end));
    }

    bool MoverDriveUnit::motor_overload_relay_threshold(const bool p_high) const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->CurrentSwitch(p_high) : false;
    }


    void MoverDriveUnit::process(const RailVehicleEngine *p_engine, const double p_delta) const {
        TMoverParameters *p_mover = owner.get_mover();
        const Ref<VehicleController> controller = p_engine->get_controller();
        const VehicleServer *vehicles = VehicleServer::get_instance();
        // only a vehicle somebody drives (Mechanik, DynObj.cpp:3246)
        if (p_mover == nullptr || controller == nullptr || vehicles == nullptr ||
            !vehicles->vehicle_has_person_role(controller->get_rid(), VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER)) {
            return;
        }
        // Original engine: DynObj.cpp:3246-3283 - the driven vehicle turns the position of its
        // integrated controller into the power setpoint every step and passes it along the
        // trainset; without it eimic_real stays 0 and an induction motor never pulls. The
        // train-wide ED/PN brake force split that follows it there is not ported (TODO.md).
        const bool diesel = p_mover->EngineType == Maszyna::TEngineType::DieselEngine ||
                            p_mover->EngineType == Maszyna::TEngineType::DieselElectric;
        const bool induction = p_mover->EngineType == Maszyna::TEngineType::ElectricInductionMotor;
        if (induction || (diesel && p_mover->EIMCtrlType > 0)) {
            // DynObj.cpp:3196-3201 - a driven car without power of its own (a control car) sets its
            // controller as the vehicle it controls stands; the cab's controller moves that one
            if (induction && p_mover->Power < 1.0) {
                const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
                const MaszynaMoverVehicleServer *movers = MaszynaMoverVehicleServer::get_instance();
                const TMoverParameters *controlling =
                        rail_vehicles != nullptr && movers != nullptr
                                ? movers->mover_get(rail_vehicles->vehicle_find_powered(controller->get_rid()))
                                : nullptr;
                if (controlling != nullptr && controlling != p_mover) {
                    p_mover->MainCtrlPos =
                            controlling->MainCtrlPos * p_mover->MainCtrlPosNo / std::max(1, controlling->MainCtrlPosNo);
                    p_mover->SpeedCtrlValue = controlling->SpeedCtrlValue;
                    p_mover->SpeedCtrlUnit.IsActive = controlling->SpeedCtrlUnit.IsActive;
                }
            }
            p_mover->CheckEIMIC(p_delta);
            if (induction || p_mover->SpeedCtrl) {
                p_mover->CheckSpeedCtrl(p_delta);
            }
            p_mover->eimic_real = std::min(p_mover->eimic, p_mover->eimicSpeedCtrl);
            // the trainset gets traction only; braking is the ED/PN split's business
            p_mover->SendCtrlToNext("EIMIC", std::max(0.0, p_mover->eimic_real), p_mover->CabActive);
        }
    }
} // namespace godot
