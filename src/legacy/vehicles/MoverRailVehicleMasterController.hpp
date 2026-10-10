#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleMasterController.hpp"

namespace godot {
    /* RailVehicleMasterController on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleMasterController : public RailVehicleMasterController, public MoverComponent {
            GDCLASS(MoverRailVehicleMasterController, RailVehicleMasterController);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();

            /* Hasler speed recorder: TTrain::Update() (Train.cpp:8580-8611) and its sound gate
             * (Train.cpp:10091-10103) */
            static constexpr double MAX_TACHOMETER_COUNT = 3.0;            // Train.cpp:8581 maxtacho
            static constexpr double TACHOMETER_WHEEL_SPEED_FACTOR = 11.31; // Train.cpp:8588
            static constexpr double TACHOMETER_MAX_SPEED_FACTOR = 1.05;    // Train.cpp:8583
            static constexpr double TACHOMETER_MIN_VELOCITY = 1.0;         // Train.cpp:8602
            static constexpr double TACHOMETER_MOVING_VELOCITY = 5.0;      // Train.cpp:8594
            static constexpr double TACHOMETER_SWING_RANGE = 4.0;          // Train.cpp:8597
            static constexpr double TACHOMETER_JUMP_OFFSET = 2.0;          // Train.cpp:8595
            static constexpr double TACHOMETER_JUMP_RANDOM_RANGE = 3.0;    // Train.cpp:8595
            static constexpr double TACHOMETER_JUMP_SCALE = 0.5;           // Train.cpp:8595
            static constexpr double TACHOMETER_COUNT_RISE_RATE = 3.0;      // Train.cpp:8605
            static constexpr double TACHOMETER_COUNT_FALL_RATE = 0.66;     // Train.cpp:8610
            static constexpr double TACHOMETER_CLOCK_STOP_COUNT = 1.0;     // Train.cpp:10100
            double tachometer_velocity = 0.0;
            double tachometer_velocity_jump = 0.0;
            double tachometer_count = 0.0;
            double tachometer_time = 0.0;
            bool tachometer_clock_active = false;
            /* TTrain::m_distancecounter (Train.h:904) - metres since activation, -1 while off */
            static constexpr double DISTANCE_COUNTER_OFF = -1.0;
            double distance_counter = DISTANCE_COUNTER_OFF;

        protected:
            void _apply_configuration() override;
            void _do_process_component(double p_delta) override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
            void _fill_state_dictionary(Dictionary &p_state) const override;

        public:
            int get_main_position() const override;
            int get_second_position() const override;
            int get_joint_position() const override;
            int get_main_actual_position() const override;
            int get_second_actual_position() const override;
            bool get_main_delayed() const override;
            int get_main_no_power_position() const override;
            int get_cabin() const override;
            bool get_cabin_controleable() const override;
            double get_tachometer_speed() const override;
            double get_tachometer_speed_jump() const override;
            double get_tachometer_clock_speed() const override;
            double get_distance_counter() const override;
            void distance_counter_activate(bool p_pressed) override;
    };
} // namespace godot
