#include "MaszynaMoverTractionMotorsUnit.hpp"
#include "maszyna/vehicles/MaszynaMoverBackend.hpp"

namespace godot {
    double MaszynaMoverTractionMotorsUnit::get_motor_current() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->Im : 0.0;
    }

    double MaszynaMoverTractionMotorsUnit::get_engine_voltage() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->EngineVoltage : 0.0;
    }

    double MaszynaMoverTractionMotorsUnit::get_total_current() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->Itot : 0.0;
    }

    double MaszynaMoverTractionMotorsUnit::get_circuit_imax() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->Imax : 0.0;
    }

    bool MaszynaMoverTractionMotorsUnit::get_dynamic_brake_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr && p_mover->DynamicBrakeFlag;
    }

    bool MaszynaMoverTractionMotorsUnit::get_fuse_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr && p_mover->FuseFlag;
    }

    bool MaszynaMoverTractionMotorsUnit::get_motor_connectors_open() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr && p_mover->StLinSwitchOff;
    }

    bool MaszynaMoverTractionMotorsUnit::is_line_contactor_closed() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr && p_mover->StLinFlag;
    }

    bool MaszynaMoverTractionMotorsUnit::is_pressure_switch_tripped() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr && p_mover->ControlPressureSwitch;
    }

    /* Original engine: OnCommand_motoroverloadrelayreset (Train.cpp:4061) calls this same FuseOn()
     * on press - "zbij nadmiarowy", clearing the overload trip (FuseFlag) that blocks
     * Mains/converter/compressor from re-enabling. */
    void MaszynaMoverTractionMotorsUnit::fuse_reset() const {
        TMoverParameters *p_mover = owner.get_mover();
        if (p_mover == nullptr) {
            return;
        }
        p_mover->FuseOn();
    }

    /* Original engine: OnCommand_motorconnectorsopen/close (Train.cpp:3947-4008) - a plain field
     * flip, no dedicated setter exists on the vendored Mover for this one. */
    void MaszynaMoverTractionMotorsUnit::set_motor_connectors_open(const bool p_open) const {
        TMoverParameters *p_mover = owner.get_mover();
        if (p_mover == nullptr) {
            return;
        }
        p_mover->StLinSwitchOff = p_open;
    }
} // namespace godot
