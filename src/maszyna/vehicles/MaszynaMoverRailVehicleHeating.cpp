#include "MaszynaMoverRailVehicleHeating.hpp"
#include "maszyna/vehicles/MaszynaMoverBackend.hpp"
#include "maszyna/vehicles/MaszynaMoverVehicleServer.hpp"
#include "utils/Units.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/rail/RailVehicleController.hpp"

namespace godot {
    void MaszynaMoverRailVehicleHeating::_bind_methods() {
        BIND_PROPERTY_W_HINT(
                MaszynaMoverRailVehicleHeating, Variant::INT, heating_power_cable_type, "heating/power_cable",
                PROPERTY_HINT_ENUM, "NoPower,BioPower,MechPower,ElectricPower,SteamPower");
        BIND_PROPERTY_W_HINT(
                MaszynaMoverRailVehicleHeating, Variant::INT, heating_generator_engine, "heating/generator",
                PROPERTY_HINT_ENUM,
                "None,Dumb,WheelsDriven,ElectricSeriesMotor,ElectricInductionMotor,DieselEngine,SteamEngine,"
                "DieselElectric,Main");
        BIND_PROPERTY(MaszynaMoverRailVehicleHeating, Variant::FLOAT, heating_generator_min_rpm, "heating/generator");
        BIND_PROPERTY(
                MaszynaMoverRailVehicleHeating, Variant::FLOAT, heating_generator_min_voltage, "heating/generator");
        BIND_PROPERTY(MaszynaMoverRailVehicleHeating, Variant::FLOAT, heating_generator_max_rpm, "heating/generator");
        BIND_PROPERTY(
                MaszynaMoverRailVehicleHeating, Variant::FLOAT, heating_generator_max_voltage, "heating/generator");
    }

    void MaszynaMoverRailVehicleHeating::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        VehicleComponent::_apply_configuration();

        p_mover->HeatingPowerSource.SourceType = MaszynaMoverVehicleServer::power_source_to_mover(get_heating_source());
        p_mover->HeatingPowerSource.MaxVoltage = get_heating_max_voltage();

        switch (get_heating_source()) {
            case RailVehicleController::POWER_SOURCE_GENERATOR: {
                // engine_revolutions is an uninitialized raw pointer on a fresh TMoverParameters
                // (MOVER.h:551); HeatingCheck() dereferences it unconditionally whenever
                // SourceType == Generator, so it must be pointed at a real double before that can
                // run safely. enrot is the vehicle's own engine revolutions counter.
                p_mover->HeatingPowerSource.EngineGenerator.engine_revolutions = &p_mover->enrot;
                p_mover->HeatingPowerSource.EngineGenerator.revolutions_min =
                        get_heating_generator_min_rpm() / Units::SECONDS_PER_MINUTE;
                p_mover->HeatingPowerSource.EngineGenerator.revolutions_max =
                        get_heating_generator_max_rpm() / Units::SECONDS_PER_MINUTE;
                p_mover->HeatingPowerSource.EngineGenerator.voltage_min = get_heating_generator_min_voltage();
                p_mover->HeatingPowerSource.EngineGenerator.voltage_max = get_heating_generator_max_voltage();
                break;
            }
            case RailVehicleController::POWER_SOURCE_POWERCABLE: {
                p_mover->HeatingPowerSource.RPowerCable.PowerTrans =
                        MaszynaMoverVehicleServer::power_type_to_mover(get_heating_power_cable_type());
                break;
            }
            default:
                break;
        }
    }

    bool MaszynaMoverRailVehicleHeating::get_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Heating : false;
    }

    bool MaszynaMoverRailVehicleHeating::get_allowed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->HeatingAllow : false;
    }

    double MaszynaMoverRailVehicleHeating::get_power() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->HeatingPower : 0.0;
    }

    void MaszynaMoverRailVehicleHeating::heating(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->HeatingSwitch(p_enabled);
    }

    void MaszynaMoverRailVehicleHeating::_fill_state_dictionary(Dictionary &p_state) const {
        // a component without a backend publishes nothing at all, rather than zeroes
        if (get_mover() == nullptr) {
            return;
        }
        p_state["heating_enabled"] = get_active();
        p_state["heating_allowed"] = get_allowed();
        p_state["heating_power"] = get_power();
    }
} // namespace godot
