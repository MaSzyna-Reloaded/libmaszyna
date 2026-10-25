#pragma once
#include "MaszynaMoverCircuitUnit.hpp"
#include "MaszynaMoverDriveUnit.hpp"
#include "MaszynaMoverTractionMotorsUnit.hpp"
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "maszyna/vehicles/MaszynaMoverElectricEngineConfiguration.hpp"
#include "maszyna/vehicles/MaszynaMoverEngineConfiguration.hpp"
#include "maszyna/vehicles/MaszynaMoverRelayListItem.hpp"
#include "vehicles/rail/RailVehicleElectricSeriesEngine.hpp"

namespace godot {
    /* RailVehicleElectricSeriesEngine on the vendored Mover. It owns the units the engine is
     * composed of, and writes the series motor's own
     * configuration into the Mover, which none of the units covers. */
    class MaszynaMoverRailVehicleElectricSeriesEngine : public RailVehicleElectricSeriesEngine,
                                                        public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleElectricSeriesEngine, RailVehicleElectricSeriesEngine);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();
            /* What the Mover's model of the engine is configured with beyond its facts. The units read it */
            Ref<MaszynaMoverEngineConfiguration> engine_configuration;
            /* What the Mover's model of the electric engine's circuit is configured with. The units read it */
            Ref<MaszynaMoverElectricEngineConfiguration> electric_engine_configuration;
            MaszynaMoverDriveUnit drive_unit_impl{*this, engine_configuration};
            MaszynaMoverCircuitUnit circuit_unit_impl{*this, electric_engine_configuration};
            MaszynaMoverTractionMotorsUnit traction_motors_unit_impl{*this};


        public:
            MaszynaMoverRailVehicleElectricSeriesEngine() {
                engine_configuration.instantiate();
                electric_engine_configuration.instantiate();
                traction_motors_unit = &traction_motors_unit_impl;
                drive_unit = &drive_unit_impl;
                circuit_unit = &circuit_unit_impl;
            }

            double get_resistor_fan_rotation() const override;
            double get_circuit_imin() const override;
            bool get_circuit_imin_high_enabled() const override;
            double get_next_position_velocity(bool p_main_controller) const override;

        protected:
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;

        private:
            double winding_resistance = 0.0;
            double resistor_fan_cutoff_resistance = 0.0;
            double resistor_fan_min_current = 50.0;
            double resistor_fan_speed = 0.5;
            double dynamic_brake_resistance = 5.8;
            double dynamic_brake_resistance_1 = 5.8;
            double dynamic_brake_resistance_2 = 5.8;
            TypedArray<MaszynaMoverRelayListItem> relay_list;

        public:
            MAKE_GS(double, winding_resistance);
            MAKE_GS(double, resistor_fan_cutoff_resistance);
            MAKE_GS(double, resistor_fan_min_current);
            MAKE_GS(double, resistor_fan_speed);
            MAKE_GS(double, dynamic_brake_resistance);
            MAKE_GS(double, dynamic_brake_resistance_1);
            MAKE_GS(double, dynamic_brake_resistance_2);
            MAKE_GS_NR(TypedArray<MaszynaMoverRelayListItem>, relay_list);
            Ref<MaszynaMoverEngineConfiguration> get_engine_configuration() const;
            void set_engine_configuration(const Ref<MaszynaMoverEngineConfiguration> &p_value);
            /* The Mover's model of the engine, read by the sounds and the AI (#316) */
            double get_circuit_nmax_rpm() const;
            double get_eimic_real() const;
            Ref<MaszynaMoverElectricEngineConfiguration> get_electric_engine_configuration() const;
            void set_electric_engine_configuration(const Ref<MaszynaMoverElectricEngineConfiguration> &p_value);
    };
} // namespace godot
