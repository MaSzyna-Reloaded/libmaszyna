#include "MoverRailVehicleEnginePowerSource.hpp"
#include "legacy/vehicles/MaszynaMoverVehicleServer.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include "utils/LibMaszynaUnits.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace godot {
    namespace {
        // Train.cpp:3695 (df5a8a8) - the pantograph compressor starts only below this pressure
        constexpr double PANTOGRAPH_COMPRESSOR_START_PRESSURE = 4.8;
    } // namespace

    static const std::unordered_map<RailVehicleEnginePowerSource::ValveOperation, Maszyna::operation_t> &
    valve_operations() {
        static const std::unordered_map<RailVehicleEnginePowerSource::ValveOperation, Maszyna::operation_t> operations =
                {
                        {RailVehicleEnginePowerSource::VALVE_OPERATION_NONE, Maszyna::operation_t::none},
                        {RailVehicleEnginePowerSource::VALVE_OPERATION_ENABLE, Maszyna::operation_t::enable},
                        {RailVehicleEnginePowerSource::VALVE_OPERATION_DISABLE, Maszyna::operation_t::disable},
                        {RailVehicleEnginePowerSource::VALVE_OPERATION_ENABLE_ON, Maszyna::operation_t::enable_on},
                        {RailVehicleEnginePowerSource::VALVE_OPERATION_ENABLE_OFF, Maszyna::operation_t::enable_off},
                        {RailVehicleEnginePowerSource::VALVE_OPERATION_DISABLE_ON, Maszyna::operation_t::disable_on},
                        {RailVehicleEnginePowerSource::VALVE_OPERATION_DISABLE_OFF, Maszyna::operation_t::disable_off},
                };
        return operations;
    }

    /* The collector's parameters share a union with the other sources' (MOVER.h:599), so they are
     * read only from a vehicle fed by a current collector */
    static const TCurrentCollector *collector_parameters(const TMoverParameters *p_mover) {
        return p_mover != nullptr && p_mover->EnginePowerSource.SourceType == Maszyna::TPowerSource::CurrentCollector
                       ? &p_mover->EnginePowerSource.CollectorParameters
                       : nullptr;
    }

    static Maszyna::end pantograph_end(const RailVehicleEnginePowerSource::PantographSelector p_selector) {
        return p_selector == RailVehicleEnginePowerSource::PANTOGRAPH_FIRST ? Maszyna::end::front : Maszyna::end::rear;
    }

    void MoverRailVehicleEnginePowerSource::_bind_methods() {}

    // LoadFIZ_Power / LoadFIZ_PowerParamsDecode (Mover.cpp:11071) and the Cntrl. pantograph keys
    // (LoadFIZ_Cntrl, Mover.cpp:10909-10946). Pantographs[*].voltage, PantFront/RearVolt and
    // PantographVoltage are not set here: they change every frame as the vehicle moves - see
    // set_pantograph_wire_voltage() and set_collector_voltage(), which write them straight.
    void MoverRailVehicleEnginePowerSource::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        VehicleComponent::_apply_configuration();
        TPowerParameters &source = p_mover->EnginePowerSource;
        source.SourceType = MaszynaMoverVehicleServer::power_source_to_mover(get_source_type());
        switch (get_source_type()) {
            case RailVehicleController::POWER_SOURCE_INTERNAL: {
                source.PowerType = MaszynaMoverVehicleServer::power_type_to_mover(get_power_cable_source());
                break;
            }
            case RailVehicleController::POWER_SOURCE_TRANSDUCER: {
                source.Transducer.InputVoltage = get_transducer_input_voltage();
                break;
            }
            case RailVehicleController::POWER_SOURCE_GENERATOR: {
                // engine_revolutions is an uninitialized raw pointer on a fresh TMoverParameters
                // (MOVER.h:551); the original points a Main engine's at enrot (Mover.cpp:11185)
                source.EngineGenerator.engine_revolutions = &p_mover->enrot;
                break;
            }
            case RailVehicleController::POWER_SOURCE_ACCUMULATOR: {
                source.RAccumulator.RechargeSource =
                        MaszynaMoverVehicleServer::power_source_to_mover(get_accumulator_recharge_source());
                break;
            }
            case RailVehicleController::POWER_SOURCE_CURRENTCOLLECTOR: {
                // the original starts every field of the collector at zero before reading it
                // (Mover.cpp:11224) - a Mover allocated here is not zeroed, and a field left out
                // (FakePower) would keep whatever the memory held
                TCurrentCollector &collector = source.CollectorParameters;
                collector = TCurrentCollector{};
                collector.CollectorsNo = get_current_collector_number_of_collectors();
                collector.MinH = get_current_collector_min_collector_lifting();
                collector.MaxH = get_current_collector_max_collector_lifting();
                collector.CSW = get_current_collector_sliding_width();
                // Mover.cpp:11622 - MaxVoltage is also the collector's own limit; left at 0, an
                // induction motor opens the line breaker above MaxV + 200 V (Mover.cpp:5706)
                collector.MaxV = get_current_collector_max_voltage();
                collector.MinV = get_current_collector_min_main_switch_voltage();
                collector.InsetV = get_current_collector_required_main_switch_voltage();
                collector.MinPress = get_current_collector_min_pantograph_tank_pressure();
                collector.MaxPress = get_current_collector_max_pantograph_tank_pressure();
                collector.OVP = get_current_collector_overvoltage_relay();
                collector.FakePower = get_current_collector_fake_power();
                collector.PhysicalLayout = get_current_collector_physical_layout();
                source.MaxVoltage = get_current_collector_max_voltage();
                source.MaxCurrent = get_current_collector_max_current();
                break;
            }
            case RailVehicleController::POWER_SOURCE_POWERCABLE: {
                source.RPowerCable.PowerTrans =
                        MaszynaMoverVehicleServer::power_type_to_mover(get_power_cable_source());
                if (source.RPowerCable.PowerTrans == TPowerType::SteamPower) {
                    source.RPowerCable.SteamPressure = get_power_cable_steam_pressure();
                }
                break;
            }
            case RailVehicleController::POWER_SOURCE_HEATER:; // Not finished on MaSzyna's side
            case RailVehicleController::POWER_SOURCE_NOT_DEFINED:;
            default:;
        }

        p_mover->PantographCompressorStart =
                MaszynaMoverVehicleServer::start_mode_to_mover(get_cntrl_pantograph_compressor_start_mode());
        p_mover->PantAutoValve = get_cntrl_pantograph_auto_valve();
        p_mover->PantsValve.start_type =
                MaszynaMoverVehicleServer::start_mode_to_mover(get_cntrl_pantographs_valve_start_mode());
        p_mover->PantsValve.spring = get_cntrl_pantographs_valve_spring();
        for (auto &pantograph_parameters: p_mover->Pantographs) {
            pantograph_parameters.valve.start_type =
                    MaszynaMoverVehicleServer::start_mode_to_mover(get_cntrl_pantograph_valve_start_mode());
            pantograph_parameters.valve.spring = get_cntrl_pantograph_valve_spring();
            pantograph_parameters.valve.solenoid = get_cntrl_pantograph_valve_solenoid();
        }
    }

    // Original engine: DynObj.cpp:3798-3832 (the current through each pantograph on the wire) and
    // 3897, 3942 (EnergyMeter). Each pantograph is counted with its own voltage and its own
    // is_active; the original swaps the front and rear voltages and asks the front pantograph's
    // is_active for both, which for two raised pantographs comes to the same sum.
    void MoverRailVehicleEnginePowerSource::_do_process_component(const double p_delta) {
        RailVehicleEnginePowerSource::_do_process_component(p_delta);
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (mover->EnginePowerSource.SourceType != Maszyna::TPowerSource::CurrentCollector) {
            return;
        }
        const double current = ((mover->DynamicBrakeFlag && mover->ResistorsFlag)
                                        ? 0.0
                                        : std::abs(mover->Itot) * mover->IsVehicleEIMBrakingFactor()) +
                               mover->TotalCurrent;
        // PantFrontVolt/PantRearVolt are zero for a pantograph that is not active or not on a wire
        // (set_pantograph_wire_voltage())
        const int active_pantographs = (mover->PantFrontVolt > 0.0 ? 1 : 0) + (mover->PantRearVolt > 0.0 ? 1 : 0);
        const double pantograph_current = current / std::max(1, active_pantographs);
        const double energy = (mover->PantFrontVolt + mover->PantRearVolt) * pantograph_current * p_delta /
                              LibMaszynaUnits::JOULES_PER_KILOWATT_HOUR; // DynObj.cpp:3897 - the meter counts kWh
        (pantograph_current > 0.0 ? mover->EnergyMeter.first : mover->EnergyMeter.second) += energy;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_max_voltage() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->EnginePowerSource.MaxVoltage : 0.0;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_max_current() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->EnginePowerSource.MaxCurrent : 0.0;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_max_lifting() const {
        const TCurrentCollector *collector = collector_parameters(get_mover());
        return collector != nullptr ? collector->MaxH : 0.0;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_min_lifting() const {
        const TCurrentCollector *collector = collector_parameters(get_mover());
        return collector != nullptr ? collector->MinH : 0.0;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_sliding_width() const {
        const TCurrentCollector *collector = collector_parameters(get_mover());
        return collector != nullptr ? collector->CSW : 0.0;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_min_main_switch_voltage() const {
        const TCurrentCollector *collector = collector_parameters(get_mover());
        return collector != nullptr ? collector->MinV : 0.0;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_min_pantograph_tank_pressure() const {
        const TCurrentCollector *collector = collector_parameters(get_mover());
        return collector != nullptr ? collector->MinPress : 0.0;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_max_pantograph_tank_pressure() const {
        const TCurrentCollector *collector = collector_parameters(get_mover());
        return collector != nullptr ? collector->MaxPress : 0.0;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_pantograph_tank_pressure() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->PantPress : 0.0;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_pantograph_pressure_lock_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->PantPressLockActive;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_pantograph_pressure_switch_armed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->PantPressSwitchActive;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_pantograph_compressor_valve() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && !mover->bPantKurek3;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_pantograph_compressor_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->PantCompFlag;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_overvoltage_relay() const {
        const TCurrentCollector *collector = collector_parameters(get_mover());
        return collector != nullptr && collector->OVP;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_required_main_switch_voltage() const {
        const TCurrentCollector *collector = collector_parameters(get_mover());
        return collector != nullptr ? collector->InsetV : 0.0;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_valve_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->PantsValve.is_active;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_valve_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->PantsValve.is_enabled;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_pantographs_dropped() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->PantAllDown;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_pantograph_first_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Pantographs[Maszyna::end::front].is_active;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_pantograph_second_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Pantographs[Maszyna::end::rear].is_active;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_pantograph_first_valve_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Pantographs[Maszyna::end::front].valve.is_enabled;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_pantograph_second_valve_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Pantographs[Maszyna::end::rear].valve.is_enabled;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_pantograph_first_valve_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Pantographs[Maszyna::end::front].valve.is_active;
    }

    bool MoverRailVehicleEnginePowerSource::get_collector_pantograph_second_valve_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Pantographs[Maszyna::end::rear].valve.is_active;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_pantograph_first_voltage() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Pantographs[Maszyna::end::front].voltage : 0.0;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_pantograph_second_voltage() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Pantographs[Maszyna::end::rear].voltage : 0.0;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_voltage() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->PantographVoltage : 0.0;
    }

    double MoverRailVehicleEnginePowerSource::get_collector_trainset_high_voltage() const {
        TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->GetTrainsetHighVoltage() : 0.0;
    }

    double MoverRailVehicleEnginePowerSource::get_energy_drawn() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->EnergyMeter.first : 0.0;
    }

    double MoverRailVehicleEnginePowerSource::get_energy_returned() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->EnergyMeter.second : 0.0;
    }

    void MoverRailVehicleEnginePowerSource::pantographs_valve(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->OperatePantographsValve(p_enabled ? Maszyna::operation_t::enable : Maszyna::operation_t::disable);
    }

    // Train.cpp:3456-3510 OnCommand_pantographraiseselected/lowerselected - the selected
    // pantographs' master valve
    void MoverRailVehicleEnginePowerSource::pantographs_valve_operate(const ValveOperation p_operation) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->OperatePantographsValve(valve_operations().at(p_operation));
    }

    // Train.cpp:3336 OnCommand_pantographlowerall
    void MoverRailVehicleEnginePowerSource::pantographs_drop_all(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->DropAllPantographs(p_enabled);
    }

    // Original engine: OnCommand_pantographcompressoractivate (Train.cpp:2912) - runs while held,
    // starting only with low enough pressure and live 24V power
    void MoverRailVehicleEnginePowerSource::pantograph_compressor(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (!p_enabled) {
            mover->PantCompFlag = false;
            return;
        }
        if (mover->PantPress < PANTOGRAPH_COMPRESSOR_START_PRESSURE && mover->Power24vIsAvailable) {
            mover->PantCompFlag = true;
        }
    }

    // Original engine: OnCommand_pantographcompressorvalveenable/disable (Train.cpp:2869-2909)
    void MoverRailVehicleEnginePowerSource::pantograph_compressor_valve(const bool p_to_compressor) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->bPantKurek3 = !p_to_compressor;
    }

    void MoverRailVehicleEnginePowerSource::pantograph(const PantographSelector p_selector, const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->OperatePantographValve(
                pantograph_end(p_selector), p_enabled ? Maszyna::operation_t::enable : Maszyna::operation_t::disable);
    }

    // Train.cpp:3218-3300 OnCommand_pantographraisefront/lowerfront and their rear twins
    void MoverRailVehicleEnginePowerSource::pantograph_valve_operate(
            const PantographSelector p_selector, const ValveOperation p_operation) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->OperatePantographValve(pantograph_end(p_selector), valve_operations().at(p_operation));
    }

    // Written straight to the Mover every frame - _apply_configuration() runs only when the
    // configuration changes, while the wire's voltage changes as the vehicle moves
    void MoverRailVehicleEnginePowerSource::set_pantograph_wire_voltage(
            const PantographSelector p_selector, const float p_voltage) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (p_selector == PANTOGRAPH_FIRST) {
            mover->Pantographs[0].voltage = p_voltage;
            mover->PantFrontVolt = mover->Pantographs[0].is_active ? p_voltage : 0.0;
        } else {
            mover->Pantographs[1].voltage = p_voltage;
            mover->PantRearVolt = mover->Pantographs[1].is_active ? p_voltage : 0.0;
        }
    }

    void MoverRailVehicleEnginePowerSource::set_collector_voltage(const float p_voltage) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->PantographVoltage = p_voltage;
    }
} // namespace godot
