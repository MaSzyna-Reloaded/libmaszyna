#pragma once
#include "RailVehicleDieselEngine.hpp"
#include "macros.hpp"
#include "vehicles/rail/RailVehicleTractionMotorsUnit.hpp"
#include "vehicles/rail/RailVehicleWWListItem.hpp"

namespace godot {
    class RailVehicleDieselElectricEngine : public RailVehicleDieselEngine {
            GDCLASS(RailVehicleDieselElectricEngine, RailVehicleDieselEngine)
        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            /* The traction motors (RailVehicleTractionMotorsUnit) */
            double get_motor_current() const;
            /* The generator's voltage on the motors [V] (EngineVoltage) */
            double get_engine_voltage() const;
            double get_circuit_imax() const;
            bool get_dynamic_brake_active() const;
            bool get_fuse_active() const;
            bool get_motor_connectors_open() const;
            bool is_line_contactor_closed() const;
            bool is_pressure_switch_tripped() const;
            /* "zbij nadmiarowy" and the line contactors */
            void fuse_reset();
            void set_motor_connectors_open(bool p_open);

        private:
            static void _bind_methods();
            TypedArray<RailVehicleWWListItem> wwlist;

            /* Engine: (Kont.), przekladnia elektryczna */
            bool generator_voltage_flat = false;
            double hyperbolic_speed = 1.0;
            double additional_speed = 1.0;
            double power_correction_ratio = 1.0;
            int shunt_relay_type = 0;
            bool shunt_mode_allowed = false;
            double heating_rpm = 0.0;

        public:
            MAKE_GS(bool, generator_voltage_flat);
            MAKE_GS(double, hyperbolic_speed);
            MAKE_GS(double, additional_speed);
            MAKE_GS(double, power_correction_ratio);
            MAKE_GS(int, shunt_relay_type);
            MAKE_GS(bool, shunt_mode_allowed);
            MAKE_GS(double, heating_rpm);

        protected:
            /* The traction motors driven by the generator, installed by the implementation that owns them */
            const RailVehicleTractionMotorsUnit *traction_motors_unit = nullptr;

            RailVehicleEngine::EngineType get_type() const override;
            void _register_commands() override;
            void _unregister_commands() override;

        public:
            TypedArray<RailVehicleWWListItem> get_wwlist() {
                return wwlist;
            }

            void set_wwlist(const TypedArray<RailVehicleWWListItem> &p_wwlist) {
                wwlist.clear();
                wwlist.append_array(p_wwlist);
            }
    };
} // namespace godot
