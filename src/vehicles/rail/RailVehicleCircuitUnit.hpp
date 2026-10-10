#pragma once
#include "RailVehicleElectricEngine.hpp"

namespace godot {
    /* The traction circuit of an electric engine (FIZ "Circuit:", "elektryczny obwod napedowy"):
     * contactors, relays, starting resistors, the camshaft, the converter and the line breaker.
     *
     * A unit the catenary-fed engines are composed of, with an implementation per simulation.
     * Not a Godot class - Godot sees only the engine. */
    class RailVehicleCircuitUnit {
        public:
            virtual ~RailVehicleCircuitUnit() = default;

            virtual bool get_contactors_active() const = 0;
            virtual bool get_diff_relay_active() const = 0;
            virtual bool get_resistors_active() const = 0;
            virtual bool get_vent_overload_active() const = 0;
            virtual bool get_highcurrent_active() const = 0;
            virtual bool get_mainbreaker_active() const = 0;
            virtual bool get_camshaft_available() const = 0;
            virtual bool get_converter_overload() const = 0;
            virtual double get_line_breaker_delay() const = 0;
            virtual double get_line_breaker_initial_delay() const = 0;
            virtual bool get_line_breaker_closes_at_no_power() const = 0;
            virtual void apply_configuration(const RailVehicleElectricEngine *p_engine) const = 0;
            virtual void converter_fuse_reset() const = 0;
    };
} // namespace godot
