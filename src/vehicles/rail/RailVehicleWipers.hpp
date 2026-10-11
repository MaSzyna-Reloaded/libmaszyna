#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleWiperListItem.hpp"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <vector>

namespace godot {
    class VehicleController;

    /* Wraps the FIZ WiperList: section and simulates the wipers.
     *
     * The simulation has no wiper switch or wiper state at all (the original keeps them in its
     * vehicle layer, DynObj.h dWiperPos), so both live in this component. The movement is
     * a port of TDynamicObject::update() (DynObj.cpp:4048-4115), the switch of
     * TTrain::OnCommand_wiperswitchincrease/decrease (Train.cpp:2638-2661). */
    class RailVehicleWipers : public RailVehicleComponent {
            GDCLASS(RailVehicleWipers, RailVehicleComponent);


        public:
            int get_component_type() const override {
                return VehicleComponentType::COMPONENT_WIPERS;
            }

        private:
            static void _bind_methods();

            /* The original's wiperOutTimer/wiperParkTimer run on every step (DynObj.cpp:4152-4153);
             * here a wiper keeps when they were last zeroed against one sweep_clock, so a parked
             * one costs nothing and still reads the same timers */
            struct Wiper {
                    double position = 0.0;  // dWiperPos: 0 parked, 1 fully out
                    bool returning = false; // wiperDirection
                    double out_since = 0.0;
                    double park_since = 0.0;
                    int working_switch_position = 0;
            };
            std::vector<Wiper> wipers;
            double sweep_clock = 0.0;
            /* Every wiper stood parked after the last pass */
            bool parked = true;
            int switch_position = 0;
            bool switch_initialized = false;
            void _set_switch_position(int p_position);

        protected:
            void _register_commands() override;
            void _unregister_commands() override;
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
            void _do_process_component(double p_delta) override;
            /* What the step takes from the simulation: whether the battery is on (Battery) */
            virtual bool _get_battery_enabled() const = 0;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            int get_switch_position() const;
            /* 0..1 sweeping out, 1..2 on the way back - wiper_pos of the original scene uniforms
             * (opengl33renderer.cpp:762-766) */
            PackedFloat64Array get_sweep_positions() const;
            void switch_increase();
            void switch_decrease();
            MAKE_MEMBER_GS(double, angle, 0.0);
            MAKE_MEMBER_GS(int, default_position, 0);
            // Number of wipers of the vehicle model (the original counts its animated submodels,
            // DynObj.cpp:5842), set by the vehicle factory. 0: the highest wiper the list switches on.
            MAKE_MEMBER_GS(int, wiper_count, 0);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<RailVehicleWiperListItem>, positions)
    };
} // namespace godot
