#include "RailVehicleHeating.hpp"
#include "vehicles/base/VehicleController.hpp"

namespace godot {
    void RailVehicleHeating::_bind_methods() {
        BIND_PROPERTY_W_HINT(
                RailVehicleHeating, Variant::INT, heating_source, "heating", PROPERTY_HINT_ENUM,
                "NotDefined,InternalSource,Transducer,Generator,Accumulator,CurrentCollector,PowerCable,Heater,Main");
        BIND_PROPERTY_W_HINT(
                RailVehicleHeating, Variant::INT, heating_generator_engine, "heating/generator", PROPERTY_HINT_ENUM,
                "None,Dumb,WheelsDriven,ElectricSeriesMotor,ElectricInductionMotor,DieselEngine,SteamEngine,"
                "DieselElectric,Main");
        BIND_PROPERTY(RailVehicleHeating, Variant::FLOAT, heating_generator_min_rpm, "heating/generator");
        BIND_PROPERTY(RailVehicleHeating, Variant::FLOAT, heating_generator_min_voltage, "heating/generator");
        BIND_PROPERTY(RailVehicleHeating, Variant::FLOAT, heating_generator_max_rpm, "heating/generator");
        BIND_PROPERTY(RailVehicleHeating, Variant::FLOAT, heating_generator_max_voltage, "heating/generator");
        BIND_PROPERTY_W_HINT(
                RailVehicleHeating, Variant::INT, heating_power_cable_type, "heating/power_cable", PROPERTY_HINT_ENUM,
                "NoPower,BioPower,MechPower,ElectricPower,SteamPower");
        BIND_PROPERTY(RailVehicleHeating, Variant::FLOAT, heating_max_voltage, "heating");

        ClassDB::bind_method(D_METHOD("heating", "enabled"), &RailVehicleHeating::heating);
        ClassDB::bind_method(D_METHOD("get_active"), &RailVehicleHeating::get_active);
        ClassDB::bind_method(D_METHOD("get_allowed"), &RailVehicleHeating::get_allowed);
        ClassDB::bind_method(D_METHOD("get_power"), &RailVehicleHeating::get_power);
    }

    void RailVehicleHeating::_register_commands() {
        VehicleComponent::_register_commands();
        register_command("heating", Callable(this, "heating"));
    }

    void RailVehicleHeating::_unregister_commands() {
        VehicleComponent::_unregister_commands();
        unregister_command("heating");
    }
} // namespace godot
