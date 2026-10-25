#pragma once
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "vehicles/rail/RailVehicleHeating.hpp"

namespace godot {
    /* RailVehicleHeating on the vendored Mover - the only class here that knows TMoverParameters. */
    class MaszynaMoverRailVehicleHeating : public RailVehicleHeating, public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleHeating, RailVehicleHeating);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();

        protected:
            void _apply_configuration() override;

        public:
            bool get_active() const override;
            bool get_allowed() const override;
            double get_power() const override;
            void heating(bool p_enabled) override;
            void _fill_state_dictionary(Dictionary &p_state) const override;

        private:
            RailVehicleEngine::EngineType heating_generator_engine = RailVehicleEngine::EngineType::MAIN;
            double heating_generator_min_rpm = 0.0;
            double heating_generator_min_voltage = 0.0;
            double heating_generator_max_rpm = 0.0;
            double heating_generator_max_voltage = 0.0;
            RailVehicleController::TrainPowerType heating_power_cable_type =
                    RailVehicleController::TrainPowerType::POWER_TYPE_ELECTRIC;

        public:
            MAKE_GS_NR(RailVehicleEngine::EngineType, heating_generator_engine);
            MAKE_GS(double, heating_generator_min_rpm);
            MAKE_GS(double, heating_generator_min_voltage);
            MAKE_GS(double, heating_generator_max_rpm);
            MAKE_GS(double, heating_generator_max_voltage);
            MAKE_GS_NR(RailVehicleController::TrainPowerType, heating_power_cable_type);
    };
} // namespace godot
