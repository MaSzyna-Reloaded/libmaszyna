#pragma once
#include "../maszyna/McZapkie/MOVER.h"
#include "TrainElectricEngine.hpp"
#include "macros.hpp"
#include "resources/engines/RelayListItem.hpp"

namespace godot {
    class TrainController;

    class TrainElectricSeriesEngine : public TrainElectricEngine {
            GDCLASS(TrainElectricSeriesEngine, TrainElectricEngine)
        public:
            /* RVent= (Automatic / Yes / No): resistor cooling fan drive mode */
            enum FanType {
                FAN_TYPE_NONE,
                FAN_TYPE_YES,
                FAN_TYPE_AUTOMATIC,
            };

            static void _bind_methods();

        protected:
            EngineType get_engine_type() override;
            void _do_update_internal_mover(TMoverParameters *p_mover) override;

        public:
            MAKE_MEMBER_GS(double, nominal_voltage, 0.0);
            MAKE_MEMBER_GS(double, winding_resistance, 0.0);
            MAKE_MEMBER_GS(double, max_rpm, 0.0);
            MAKE_MEMBER_GS_NR(FanType, fan_type, FAN_TYPE_NONE);
            MAKE_MEMBER_GS(double, fan_max_rpm, 1.0);
            MAKE_MEMBER_GS(double, fan_cutoff_resistance, 0.0);
            MAKE_MEMBER_GS(double, fan_min_current, 50.0);
            MAKE_MEMBER_GS(double, fan_speed, 0.5);
            MAKE_MEMBER_GS(double, dynamic_brake_resistance, 5.8);
            MAKE_MEMBER_GS(double, dynamic_brake_resistance_1, 5.8);
            MAKE_MEMBER_GS(double, dynamic_brake_resistance_2, 5.8);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<RelayListItem>, relay_list)
    };
} // namespace godot
VARIANT_ENUM_CAST(TrainElectricSeriesEngine::FanType)
