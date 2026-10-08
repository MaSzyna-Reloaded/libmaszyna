#pragma once

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

    /// The state of the world the whole simulation shares - the light level and the air
    /// temperature as the environment publishes them (Global.fLuminance, Global.AirTemperature) -
    /// and runs the one clock of the simulation (Timer::UpdateTimers(), Timer.cpp:70-92): the
    /// physics, the events, the drivers and the time of day all advance by the same seconds, read
    /// here.
    class SimulationServer : public Object {
            GDCLASS(SimulationServer, Object)

        private:
            /// A frame counts at most this much real time [s], then times the simulation speed. The
            /// original caps the simulated time instead, at 1 s (Timer.cpp:84), because its Mover
            /// went wrong on long frames; here the couplers are refreshed every physics step and
            /// the drivers and events run in slices, so a long frame simulates the same - x100 is
            /// x100, whatever it costs in frames. What stays capped is the real time a frame may
            /// count: one hitch (loading, a debugger) is not taken whole and multiplied, so the
            /// simulation cannot spiral into ever longer frames; under 4 fps it runs slower.
            static constexpr double MAX_FRAME_DELTA = 0.25;
            /// The running speed this near the one set is at it
            static constexpr double SPEED_SETTLED = 0.001;
            /// A frame is simulated in slices no longer than this [s], each announced on its own:
            /// what reacts on the clock between frames - a driver reacts every 0.1 s at the
            /// quickest (ReactionTime, Driver.cpp:7501), an event runs in its own pass - then does
            /// the same whether a frame is short or long. The physics steps each slice in its own
            /// 0.01 s steps.
            static constexpr double MAX_SLICE_TIME = 0.1;
            static constexpr double SECONDS_PER_HOUR = 3600.0;
            static constexpr double MINUTES_PER_HOUR = 60.0;
            static constexpr double HOURS_PER_DAY = 24.0;

            double time_of_day = 0.0;
            double simulation_time = 0.0;
            double simulation_speed = 1.0;
            /// The speed the clock runs at, going to simulation_speed over speed_change_time
            double current_simulation_speed = 1.0;
            /// [s], taken from the Project Setting when a speed is set
            double speed_change_time = 0.0;
            /// Who needs the clock running, and whether it runs - on SceneTree's `process_frame`,
            /// which comes before any node's `_process`, so every reader sees this frame's step
            int clock_holders = 0;
            bool clock_running = false;
            /// The SimulationRuntime nodes in the tree: the clock runs under at least one; with
            /// none (the editor, a viewer) it stands, whoever holds it
            int runtimes = 0;
            double light_level = 1.0;
            double air_temperature = 0.0;
            bool paused = false;

            void _refresh_clock();
            void _on_process_frame();

            /// The speed's change time follows its setting
            void _on_project_settings_changed();

        protected:
            static void _bind_methods();

        public:
            SimulationServer();
            static const char *simulation_advanced_signal;
            static const char *simulation_paused_signal;
            static const char *simulation_unpaused_signal;
            static const char *simulation_speed_changed_signal;
            static const char *simulation_current_speed_changed_signal;
            /// The Project Setting of how long the running speed takes to reach one set [s]
            static constexpr const char *SPEED_CHANGE_TIME_SETTING = "maszyna/simulation/speed_change_time";
            /// ... when the project does not set it (libmaszyna.gd registers the same) [s]
            static constexpr double SPEED_CHANGE_TIME_DEFAULT = 0.4;
            static const char *time_of_day_changed_signal;

            static SimulationServer *get_instance() {
                return dynamic_cast<SimulationServer *>(Engine::get_singleton()->get_singleton("SimulationServer"));
            }

            /// Hours since midnight, fractional. Setting it jumps the clock (a scenario's start, the
            /// player's change, the system time); running, it advances with the simulation.
            void set_time_of_day(double p_hours);
            double get_time_of_day() const;
            /// Simulated seconds so far (fSimulationTime, Timer.cpp:87)
            double simulation_get_time() const;
            /// The clock runs while somebody holds it and the runtime is not paused: whoever needs
            /// time to pass holds it while it does, and lets it go
            void clock_hold();
            void clock_release();
            /// A SimulationRuntime node entering and leaving the tree: the clock runs only while
            /// one is attached
            void runtime_attach();
            void runtime_detach();
            /// Holds the clock and receives every `simulation_advanced` slice until unsubscribed
            void clock_subscribe(const Callable &p_on_advanced);
            void clock_unsubscribe(const Callable &p_on_advanced);
            /// One frame of the clock, `p_frame_delta` real seconds (Timer::UpdateTimers(),
            /// Timer.cpp:79-87): at most MAX_FRAME_DELTA of it, times the simulation speed, in
            /// slices of at most MAX_SLICE_TIME - each added to the simulation time and the time
            /// of day, then `simulation_advanced(seconds)`. Called by the clock's node, processed
            /// before every other.
            void simulation_advance(double p_frame_delta);
            /// How many simulated seconds pass in one real second (Global.fTimeSpeed, Timer.cpp:80) -
            /// the speed set; the clock gets to it like a tape's motor, over the Project Setting's
            /// SPEED_CHANGE_TIME_SETTING (simulation_get_current_speed())
            void set_simulation_speed(double p_speed);
            double get_simulation_speed() const;
            /// The speed the clock runs at now, on its way to the one set
            double simulation_get_current_speed() const;
            /// The wall clock's speed, at once rather than over the speed change time - a scenery
            /// starts at it, and the menu is heard at it
            void simulation_reset_speed();
            /// How bright the scene is, 0-1 (Global.fLuminance, simulationenvironment.cpp:184)
            void set_light_level(double p_level);
            double get_light_level() const;
            /// Degrees Celsius
            void set_air_temperature(double p_temperature);
            double get_air_temperature() const;
            /// Stops the world while something covers it (a loading screen, the spinner of "Exit to
            /// menu"): whoever runs a part of the simulation, or its sound, holds it on "simulation_paused"
            /// and lets it go on "simulation_unpaused"
            void simulation_pause();
            void simulation_unpause();
            bool simulation_is_paused() const;
    };

} // namespace godot
