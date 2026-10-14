#include "MoverRailVehiclePowerSupply.hpp"
#include "legacy/vehicles/MaszynaMoverVehicleServer.hpp"
#include "legacy/vehicles/MoverBackend.hpp"

namespace godot {
    void MoverRailVehiclePowerSupply::_bind_methods() {}

    void MoverRailVehiclePowerSupply::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        VehicleComponent::_apply_configuration();

        // a battery without a nominal voltage cannot be started (CheckLocomotiveParameters(),
        // Mover.cpp:8681) - applied here too, as this runs again after that check
        p_mover->BatteryStart =
                get_battery_voltage() == 0.0
                        ? Maszyna::start_t::disabled
                        : MaszynaMoverVehicleServer::start_mode_to_mover(get_cntrl_battery_start_mode());
        // a Cntrl. key of every vehicle, not of an electric engine (Mover.cpp:10909 LoadFIZ_Cntrl) -
        // a diesel-electric's compressor runs off the converter too (CompressorPower=Converter)
        p_mover->ConverterStart = MaszynaMoverVehicleServer::start_mode_to_mover(get_cntrl_converter_start_mode());
        p_mover->ConverterStartDelay = static_cast<float>(get_cntrl_converter_start_delay());
        p_mover->BatteryVoltage = get_battery_voltage();
        // the nominal voltage, which BatteryVoltage then drains from (Mover.cpp:946)
        p_mover->NominalBatteryVoltage = static_cast<float>(get_battery_voltage()); // LoadFIZ_Light
    }

    double MoverRailVehiclePowerSupply::get_live_battery_voltage() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->BatteryVoltage : 0.0;
    }

    bool MoverRailVehiclePowerSupply::get_battery_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Battery : false;
    }

    bool MoverRailVehiclePowerSupply::get_converter_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->ConverterFlag : false;
    }

    bool MoverRailVehiclePowerSupply::get_converter_allowed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->ConverterAllow : false;
    }

    double MoverRailVehiclePowerSupply::get_converter_time_to_start() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->ConverterStartDelayTimer : 0.0;
    }

    double MoverRailVehiclePowerSupply::get_power24_voltage() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Power24vVoltage : 0.0;
    }

    bool MoverRailVehiclePowerSupply::get_power24_available() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Power24vIsAvailable : false;
    }

    bool MoverRailVehiclePowerSupply::get_power110_available() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Power110vIsAvailable : false;
    }

    void MoverRailVehiclePowerSupply::battery(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->BatterySwitch(p_enabled);
    }

    void MoverRailVehiclePowerSupply::converter(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->ConverterSwitch(p_enabled);
    }
} // namespace godot
