#pragma once
#include "../core/TrainPart.hpp"
#include "../engines/TrainEngine.hpp"
#include "macros.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    class TrainController;
    class TrainHeating : public TrainPart {
            GDCLASS(TrainHeating, TrainPart);

        private:
            static void _bind_methods();

        protected:
            void _do_update_internal_mover(TMoverParameters *p_mover) override;
            void _do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) override;

        public:
            MAKE_MEMBER_GS_NR(
                    TrainController::TrainPowerSource, heating_source,
                    TrainController::TrainPowerSource::POWER_SOURCE_GENERATOR);
            MAKE_MEMBER_GS_NR(TrainEngine::EngineType, generator_engine, TrainEngine::EngineType::MAIN);
            MAKE_MEMBER_GS(double, generator_min_rpm, 0.0);
            MAKE_MEMBER_GS(double, generator_min_voltage, 0.0);
            MAKE_MEMBER_GS(double, generator_max_rpm, 0.0);
            MAKE_MEMBER_GS(double, generator_max_voltage, 0.0);
            MAKE_MEMBER_GS_NR(
                    TrainController::TrainPowerType, power_cable_power_type,
                    TrainController::TrainPowerType::POWER_TYPE_ELECTRIC);
            MAKE_MEMBER_GS(double, max_voltage, 0.0);
    };
} // namespace godot
