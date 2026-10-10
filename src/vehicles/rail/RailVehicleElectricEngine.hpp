#pragma once
#include "RailVehicleEngine.hpp"
#include "macros.hpp"
#include "vehicles/rail/RailVehicleController.hpp"
#include "vehicles/rail/RailVehicleTractionMotorsUnit.hpp"

namespace godot {
    class VehicleController;
    class RailVehicleCircuitUnit;

    class RailVehicleElectricEngine : public RailVehicleEngine {
            GDCLASS(RailVehicleElectricEngine, RailVehicleEngine)


        protected:
            /* The units the engine is composed of, installed by the implementation that owns them */
            const RailVehicleCircuitUnit *circuit_unit = nullptr;
            const RailVehicleTractionMotorsUnit *traction_motors_unit = nullptr;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;

            /* Fed from the catenary - a diesel has none of these */
            bool get_camshaft_available() const;
            bool get_converter_overload() const;
            double get_line_breaker_delay() const;
            double get_line_breaker_initial_delay() const;
            bool get_line_breaker_closes_at_no_power() const;

            /* The traction motors (RailVehicleTractionMotorsUnit) */
            double get_motor_current() const;
            /* The voltage on the motors [V] (EngineVoltage) */
            double get_engine_voltage() const;
            /* The current drawn from the high voltage line [A] (Itot) */
            double get_total_current() const;
            double get_circuit_imax() const;
            bool get_dynamic_brake_active() const;
            bool get_fuse_active() const;
            bool get_motor_connectors_open() const;
            bool is_line_contactor_closed() const;
            bool is_pressure_switch_tripped() const;
            /* "zbij nadmiarowy" and the line contactors */
            void fuse_reset();
            void set_motor_connectors_open(bool p_open);

            bool get_contactors_active() const;
            bool get_diff_relay_active() const;
            bool get_resistors_active() const;
            bool get_vent_overload_active() const;
            bool get_highcurrent_active() const;
            bool get_mainbreaker_active() const;

            static void _bind_methods();
            /* Circuit: (elektryczny obwod napedowy) - defaults of MOVER.h:1740-1744, 1762-1769 */
            MAKE_MEMBER_GS(double, circuit_resistance, 0.0);
            MAKE_MEMBER_GS(int, circuit_imax_low, 0);
            MAKE_MEMBER_GS(int, circuit_imax_high, 0);
            MAKE_MEMBER_GS(int, circuit_imin_low, 0);
            MAKE_MEMBER_GS(int, circuit_imin_high, 0);
            MAKE_MEMBER_GS(double, circuit_tuhex_sum, 750.0);
            MAKE_MEMBER_GS(double, circuit_tuhex_diff, 10.0);
            MAKE_MEMBER_GS(double, circuit_tuhex_min_current, 60.0);
            MAKE_MEMBER_GS(double, circuit_tuhex_max_current, 400.0);
            MAKE_MEMBER_GS(int, circuit_tuhex_stages, 0);
            MAKE_MEMBER_GS(double, circuit_tuhex_sum_1, 750.0);
            MAKE_MEMBER_GS(double, circuit_tuhex_sum_2, 750.0);
            MAKE_MEMBER_GS(double, circuit_tuhex_sum_3, 750.0);

            /* Cntrl. (elektryczne) */
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::StartMode, cntrl_converter_overload_relay_start_mode,
                    RailVehicleController::START_MODE_MANUAL);
            MAKE_MEMBER_GS(bool, cntrl_converter_overload_relay_off_when_main_is_off, false);
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::StartMode, cntrl_main_switch_start_mode,
                    RailVehicleController::START_MODE_MANUAL);

            void converter_fuse_reset();
            void _register_commands() override;
            void _unregister_commands() override;

        protected:
            void _apply_configuration() override;
    };
} // namespace godot
