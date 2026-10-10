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

        protected:
            void _register_commands() override;
            void _unregister_commands() override;

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
            virtual double get_tachometer_speed() const = 0;
            virtual double get_tachometer_speed_jump() const = 0;
            virtual double get_tachometer_clock_speed() const = 0;
            /* Metres since the distance counter was started, or -1 while it is off
             * (TTrain::m_distancecounter, Train.h:904) */
            virtual double get_distance_counter() const = 0;

            /* distancecounter_sw: pressed starts the distance counter anew (Train.cpp:1552) */
            virtual void distance_counter_activate(bool p_pressed) = 0;

            void _fill_state_dictionary(Dictionary &p_state) const override;
    };
} // namespace godot
