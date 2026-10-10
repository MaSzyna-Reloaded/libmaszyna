#include "MoverCircuitUnit.hpp"
#include "legacy/vehicles/MaszynaMoverVehicleServer.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include "vehicles/rail/RailVehicleController.hpp"
#include "vehicles/rail/RailVehicleElectricEngine.hpp"

namespace godot {
    namespace {
        /* The cab lamps' own thresholds (Train.cpp:9054-9058, 9076) */
        constexpr double LAMP_BRAKE_PRESS_RELEASED = 1.0;
        constexpr double LAMP_VENT_OVERLOAD_MIN_ROT = 5.0;
    } // namespace

    bool MoverCircuitUnit::get_contactors_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        if (p_mover == nullptr || p_mover->StLinFlag || p_mover->ControlPressureSwitch) {
            return false;
        }
        return p_mover->BrakePress < LAMP_BRAKE_PRESS_RELEASED;
    }

    bool MoverCircuitUnit::get_diff_relay_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        if (p_mover == nullptr || p_mover->GroundRelay || p_mover->ControlPressureSwitch) {
            return false;
        }
        return p_mover->BrakePress < LAMP_BRAKE_PRESS_RELEASED;
    }

    bool MoverCircuitUnit::get_resistors_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        if (p_mover == nullptr || !p_mover->StLinFlag) {
            return false;
        }
        return p_mover->ResistorsFlagCheck();
    }

    bool MoverCircuitUnit::get_vent_overload_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? (p_mover->RventRot < LAMP_VENT_OVERLOAD_MIN_ROT) && p_mover->ResistorsFlagCheck()
                                  : false;
    }

    bool MoverCircuitUnit::get_highcurrent_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? !(p_mover->Imax < p_mover->ImaxHi) : false;
    }

    bool MoverCircuitUnit::get_mainbreaker_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->Mains : false;
    }

    bool MoverCircuitUnit::get_camshaft_available() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->HasCamshaft : false;
    }

    bool MoverCircuitUnit::get_converter_overload() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->ConvOvldFlag : false;
    }

    double MoverCircuitUnit::get_line_breaker_delay() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->CtrlDelay : 0.0;
    }

    double MoverCircuitUnit::get_line_breaker_initial_delay() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->InitialCtrlDelay : 0.0;
    }

    bool MoverCircuitUnit::get_line_breaker_closes_at_no_power() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->LineBreakerClosesOnlyAtNoPowerPos : false;
    }

    void MoverCircuitUnit::converter_fuse_reset() const {
        TMoverParameters *mover = owner.get_mover();
        ASSERT_MOVER(mover);
        // Original engine: OnCommand_converteroverloadrelayreset (Train.cpp:3567-3585) ->
        // RelayReset(relay_t::primaryconverteroverload), "converterfuse_bt:"/ggConverterFuseButton
        // (Train.cpp:10053) - the converter-specific counterpart to fuse_reset()/FuseOn() above.
        mover->RelayReset(Maszyna::primaryconverteroverload);
    }

    void MoverCircuitUnit::apply_configuration(const RailVehicleElectricEngine *p_engine) const {
        TMoverParameters *p_mover = owner.get_mover();
        /* Circuit: (elektryczny obwod napedowy), tylko pojazdy elektryczne i spalinowo-elektryczne */
        p_mover->CircuitRes = p_engine->get_circuit_resistance();
        p_mover->ImaxLo = p_engine->get_circuit_imax_low();
        p_mover->ImaxHi = p_engine->get_circuit_imax_high();
        p_mover->IminLo = p_engine->get_circuit_imin_low();
        p_mover->IminHi = p_engine->get_circuit_imin_high();
        // LoadFIZ_Circuit (Mover.cpp:11424-11425): the thresholds in use start at the low ones -
        // Imax is moved by the relay only where ImaxHi > ImaxLo, Imin only by its switch
        p_mover->Imin = p_mover->IminLo;
        p_mover->Imax = p_mover->ImaxLo;
        p_mover->TUHEX_Sum = p_engine->get_circuit_tuhex_sum();
        p_mover->TUHEX_Diff = p_engine->get_circuit_tuhex_diff();
        p_mover->TUHEX_MinIw = p_engine->get_circuit_tuhex_min_current();
        p_mover->TUHEX_MaxIw = p_engine->get_circuit_tuhex_max_current();
        p_mover->TUHEX_Stages = p_engine->get_circuit_tuhex_stages();
        p_mover->TUHEX_Sum1 = p_engine->get_circuit_tuhex_sum_1();
        p_mover->TUHEX_Sum2 = p_engine->get_circuit_tuhex_sum_2();
        p_mover->TUHEX_Sum3 = p_engine->get_circuit_tuhex_sum_3();

        p_mover->ConverterOverloadRelayStart = MaszynaMoverVehicleServer::start_mode_to_mover(
                p_engine->get_cntrl_converter_overload_relay_start_mode());
        p_mover->ConverterOverloadRelayOffWhenMainIsOff =
                p_engine->get_cntrl_converter_overload_relay_off_when_main_is_off();
        p_mover->MainsStart =
                MaszynaMoverVehicleServer::start_mode_to_mover(p_engine->get_cntrl_main_switch_start_mode());
    }
} // namespace godot
