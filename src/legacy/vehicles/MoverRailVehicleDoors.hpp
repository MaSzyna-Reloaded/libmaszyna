#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleDoors.hpp"

namespace godot {
    /* RailVehicleDoors on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleDoors : public RailVehicleDoors, public MoverComponent {
            GDCLASS(MoverRailVehicleDoors, RailVehicleDoors);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();

            /// What each side's doors were on the last step, to report the change once
            bool left_open = false;
            bool left_closed = true;
            bool right_open = false;
            bool right_closed = true;

        protected:
            void _apply_configuration() override;
            void _do_process_component(double p_delta) override;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
            int get_permit_preset() const override;
            bool get_locked() const override;
            bool get_lock_enabled() const override;
            bool get_step_enabled() const override;
            int get_open_control() const override;
            bool get_left_open() const override;
            bool get_left_closed() const override;
            bool get_left_door_closed() const override;
            bool get_left_open_permit() const override;
            bool get_left_local_open() const override;
            bool get_left_remote_open() const override;
            double get_left_position() const override;
            double get_left_position_normalized() const override;
            bool get_left_operating() const override;
            double get_left_step_position() const override;
            bool get_left_step_operating() const override;
            bool get_right_open() const override;
            bool get_right_closed() const override;
            bool get_right_door_closed() const override;
            bool get_right_open_permit() const override;
            bool get_right_local_open() const override;
            bool get_right_remote_open() const override;
            double get_right_position() const override;
            double get_right_position_normalized() const override;
            bool get_right_operating() const override;
            double get_right_step_position() const override;
            bool get_right_step_operating() const override;
            void permit_step(bool p_state) override;
            void permit_doors(bool p_state, Side p_side) override;
            void operate_doors(bool p_state, Side p_side) override;
            void operate_doors_locally(bool p_state, Side p_side) override;
            void door_lock(bool p_state) override;
            void door_remote_control(bool p_state) override;
            void signal_departure(bool p_state) override;
            bool get_departure_signal() const override;
            bool get_departure_signal_sounding() const override;
            bool get_remote_only() const override;
            void next_permit_preset() override;
            void previous_permit_preset() override;
            double get_mirror_left_position() const override;
            double get_mirror_right_position() const override;
            bool get_mirrors_forbidden() const override;
            void forbid_mirrors(bool p_state) override;

        private:
            /* dMirrorMoveL/dMirrorMoveR of the original (DynObj.h), kept by the vehicle, not the Mover */
            double mirror_left_position = 0.0;
            double mirror_right_position = 0.0;
            const std::map<Voltage, float> voltage_map = {
                    {VOLTAGE_0, 0.0f}, {VOLTAGE_12, 12.0f}, {VOLTAGE_24, 24.0f}, {VOLTAGE_110, 110.0f}};
            const std::map<Type, int> door_type_map = {
                    {TYPE_SHIFT, 1}, {TYPE_ROTATE, 2}, {TYPE_FOLD, 3}, {TYPE_PLUG, 4}};
            const std::map<PlatformType, int> door_platform_type_map = {
                    {PLATFORM_TYPE_SHIFT, 1}, {PLATFORM_TYPE_ROTATE, 2}};
            const std::map<PermitLight, int> door_permit_light_map = {
                    {PERMIT_LIGHT_CONTINUOUS, 0},
                    {PERMIT_LIGHT_FLASHING_ON_PERMISSION_WITH_STEP, 1},
                    {PERMIT_LIGHT_FLASHING_ON_PERMISSION, 2},
                    {PERMIT_LIGHT_FLASHING_ALWAYS, 3}};
            const std::unordered_map<Controls, Maszyna::control_t> door_controls_map = {
                    {Controls::CONTROLS_PASSENGER, Maszyna::control_t::passenger},
                    {Controls::CONTROLS_AUTOMATIC, Maszyna::control_t::autonomous},
                    {Controls::CONTROLS_DRIVER, Maszyna::control_t::driver},
                    {Controls::CONTROLS_CONDUCTOR, Maszyna::control_t::conductor},
                    {Controls::CONTROLS_MIXED, Maszyna::control_t::mixed},
            };
    };
} // namespace godot
