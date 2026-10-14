#pragma once
#include "vehicles/rail/RailVehicleComponent.hpp"

namespace godot {
    /* The driver's master controller of a vehicle, whatever it drives: the positions of its main
     * and second controller and how fast they step (LoadFIZ_Cntrl, Mover.cpp:10837-10869). A control
     * car has one without an engine - EN57's ra reverses and steps its controller like its motor car.
     * It is what makes a vehicle one with a cab, so the cab's own instruments are its too: the cab
     * activation, the Hasler speed recorder and the distance counter (TTrain, Train.cpp). */
    class RailVehicleMasterController : public RailVehicleComponent {
            GDCLASS(RailVehicleMasterController, RailVehicleComponent);

        public:
            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_MASTER_CONTROLLER;
            }

        private:
            static void _bind_methods();

            /* Hasler speed recorder - the cab's, not the simulation's: TTrain::Update() (Train.cpp:8580-8611) and its
             * sound gate (Train.cpp:10091-10103) */
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
            void _register_commands() override;
            void _unregister_commands() override;
            /* The speed recorder and the distance counter moved on by a step */
            void _do_process_component(double p_delta) override;

            /* What the step takes from the simulation: the powered wheels' diameter [m] and
             * revolutions [1/s] (WheelDiameter, nrot), the top speed [km/h] (Vmax), whether the low
             * voltage is up (Power24vIsAvailable || Power110vIsAvailable), and the occupied cab - 1
             * front, -1 rear, 0 none (CabOccupied) */
            virtual double _get_powered_wheel_diameter() const = 0;
            virtual double _get_powered_axle_revolutions() const = 0;
            virtual double _get_max_velocity() const = 0;
            virtual bool _get_low_voltage_available() const = 0;
            virtual int _get_occupied_cab() const = 0;

            /* MCPN */
            int main_position_count = 0;
            /* SCPN */
            int second_position_count = 0;
            /* DirChangeMaxPos: the highest main position the reverser may be moved at */
            int direction_change_max_position = 0;
            /* CoupledCtrl: the second controller continues the main one */
            bool coupled_controllers = false;
            /* IniCDelay, SCDelay, SCDDelay [s] */
            double initial_delay = 0.0;
            double step_delay = 0.0;
            double step_down_delay = 0.0;
            /* MaxTachoSpeed: the top of the speed recorder's dial [km/h], 0 for Vmax * 1.05
             * (Mover.cpp:10815, Train.cpp:8583-8587) */
            double tachometer_max_speed = 0.0;

        public:
            void set_main_position_count(int p_value);
            int get_main_position_count() const;
            void set_second_position_count(int p_value);
            int get_second_position_count() const;
            void set_direction_change_max_position(int p_value);
            int get_direction_change_max_position() const;
            void set_coupled_controllers(bool p_value);
            bool get_coupled_controllers() const;
            void set_initial_delay(double p_value);
            double get_initial_delay() const;
            void set_step_delay(double p_value);
            double get_step_delay() const;
            void set_step_down_delay(double p_value);
            double get_step_down_delay() const;
            void set_tachometer_max_speed(double p_value);
            double get_tachometer_max_speed() const;

            /* Live state, read straight from the backend */
            virtual int get_main_position() const = 0;
            virtual int get_second_position() const = 0;
            /* The cab's joint handle: the main position, the shunt counted on with a coupled
             * controller, and the local brake below zero (Train.cpp:9410) */
            virtual int get_joint_position() const = 0;
            virtual int get_main_actual_position() const = 0;
            /* The position the secondary controller has actually reached (ScndCtrlActualPos) */
            virtual int get_second_actual_position() const = 0;
            /* DelayCtrlFlag: the master controller waits on its first position for the line
             * contactors (Mover.cpp) */
            virtual bool get_main_delayed() const = 0;
            /* The last master controller position that gives no power (MainCtrlNoPowerPos(),
             * Mover.cpp:2694): 0, or the EIM controller's own */
            virtual int get_main_no_power_position() const = 0;
            /* The active cab (CabActive) and whether it is the one in command (IsCabMaster()) */
            virtual int get_cabin() const = 0;
            virtual bool get_cabin_controleable() const = 0;
            /* Hasler speed recorder (Train.cpp:8580-8611) */
            double get_tachometer_speed() const;
            double get_tachometer_speed_jump() const;
            double get_tachometer_clock_speed() const;
            /* Metres since the distance counter was started, or -1 while it is off
             * (TTrain::m_distancecounter, Train.h:904) */
            double get_distance_counter() const;

            /* distancecounter_sw: pressed starts the distance counter anew (Train.cpp:1552) */
            void distance_counter_activate(bool p_pressed);

            void _fill_state_dictionary(Dictionary &p_state) const override;
    };
} // namespace godot
