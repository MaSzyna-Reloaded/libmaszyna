#pragma once
#include "TrainDieselEngine.hpp"
#include "macros.hpp"
#include "resources/engines/WWListItem.hpp"

namespace godot {
    class TrainDieselElectricEngine : public TrainDieselEngine {
            GDCLASS(TrainDieselElectricEngine, TrainDieselEngine)

        private:
            static void _bind_methods();
            TypedArray<WWListItem> wwlist;

            /* Engine: (Kont.), przekladnia elektryczna */
            MAKE_MEMBER_GS(bool, generator_voltage_flat, false);
            MAKE_MEMBER_GS(double, hyperbolic_speed, 1.0);
            MAKE_MEMBER_GS(double, additional_speed, 1.0);
            MAKE_MEMBER_GS(double, rpm_change_rate, 2.0);
            MAKE_MEMBER_GS(double, power_correction_ratio, 1.0);
            MAKE_MEMBER_GS(int, shunt_relay_type, 0);
            MAKE_MEMBER_GS(bool, shunt_mode_allowed, false);
            MAKE_MEMBER_GS(double, heating_rpm, 0.0);

        protected:
            TrainEngine::EngineType get_engine_type() override;
            void _do_update_internal_mover(TMoverParameters *p_mover) override;

        public:
            TypedArray<WWListItem> get_wwlist() {
                return wwlist;
            }

            void set_wwlist(const TypedArray<WWListItem> &p_wwlist) {
                wwlist.clear();
                wwlist.append_array(p_wwlist);
            }
    };
} // namespace godot
