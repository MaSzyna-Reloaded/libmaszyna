#include "SimulationServer.hpp"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>

namespace godot {

    const char *SimulationServer::simulation_paused_signal = "simulation_paused";
    const char *SimulationServer::simulation_unpaused_signal = "simulation_unpaused";
    const char *SimulationServer::simulation_speed_changed_signal = "simulation_speed_changed";
    const char *SimulationServer::simulation_current_speed_changed_signal = "simulation_current_speed_changed";
    const char *SimulationServer::time_of_day_changed_signal = "time_of_day_changed";
    const char *SimulationServer::simulation_advanced_signal = "simulation_advanced";

    SimulationServer::SimulationServer() {
        _on_project_settings_changed();
        ProjectSettings::get_singleton()->connect(
                "settings_changed", callable_mp(this, &SimulationServer::_on_project_settings_changed));
    }

    void SimulationServer::_on_project_settings_changed() {
        speed_change_time =
                ProjectSettings::get_singleton()->get_setting(SPEED_CHANGE_TIME_SETTING, SPEED_CHANGE_TIME_DEFAULT);
    }

    void SimulationServer::_bind_methods() {

        ClassDB::bind_method(D_METHOD("set_time_of_day", "hours"), &SimulationServer::set_time_of_day);
        ClassDB::bind_method(D_METHOD("get_time_of_day"), &SimulationServer::get_time_of_day);
        ClassDB::bind_method(D_METHOD("set_simulation_speed", "speed"), &SimulationServer::set_simulation_speed);
        ClassDB::bind_method(D_METHOD("get_simulation_speed"), &SimulationServer::get_simulation_speed);
        ClassDB::bind_method(D_METHOD("simulation_get_current_speed"), &SimulationServer::simulation_get_current_speed);
        ClassDB::bind_method(D_METHOD("simulation_reset_speed"), &SimulationServer::simulation_reset_speed);
        ClassDB::bind_method(D_METHOD("set_light_level", "level"), &SimulationServer::set_light_level);
        ClassDB::bind_method(D_METHOD("get_light_level"), &SimulationServer::get_light_level);
        ClassDB::bind_method(D_METHOD("set_air_temperature", "temperature"), &SimulationServer::set_air_temperature);
        ClassDB::bind_method(D_METHOD("get_air_temperature"), &SimulationServer::get_air_temperature);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "time_of_day"), "set_time_of_day", "get_time_of_day");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "simulation_speed"), "set_simulation_speed", "get_simulation_speed");
        ADD_SIGNAL(MethodInfo(simulation_speed_changed_signal));
        ADD_SIGNAL(MethodInfo(simulation_current_speed_changed_signal));
        ADD_SIGNAL(MethodInfo(time_of_day_changed_signal));
        ClassDB::bind_method(D_METHOD("simulation_get_time"), &SimulationServer::simulation_get_time);
        ClassDB::bind_method(D_METHOD("clock_hold"), &SimulationServer::clock_hold);
        ClassDB::bind_method(D_METHOD("clock_release"), &SimulationServer::clock_release);
        ClassDB::bind_method(D_METHOD("simulation_advance", "frame_delta"), &SimulationServer::simulation_advance);
        ADD_SIGNAL(MethodInfo(simulation_advanced_signal, PropertyInfo(Variant::FLOAT, "seconds")));
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "light_level"), "set_light_level", "get_light_level");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "air_temperature"), "set_air_temperature", "get_air_temperature");
        ClassDB::bind_method(D_METHOD("simulation_pause"), &SimulationServer::simulation_pause);
        ClassDB::bind_method(D_METHOD("simulation_unpause"), &SimulationServer::simulation_unpause);
        ClassDB::bind_method(D_METHOD("simulation_is_paused"), &SimulationServer::simulation_is_paused);
        ADD_SIGNAL(MethodInfo(simulation_paused_signal));
        ADD_SIGNAL(MethodInfo(simulation_unpaused_signal));
    }

    void SimulationServer::set_time_of_day(const double p_hours) {
        if (time_of_day == p_hours) {
            return;
        }
        time_of_day = p_hours;
        emit_signal(time_of_day_changed_signal);
    }

    double SimulationServer::get_time_of_day() const {
        return time_of_day;
    }

    double SimulationServer::simulation_get_time() const {
        return simulation_time;
    }

    void SimulationServer::clock_hold() {
        ++clock_holders;
        _refresh_clock();
    }

    void SimulationServer::clock_release() {
        ERR_FAIL_COND(clock_holders <= 0);
        --clock_holders;
        _refresh_clock();
    }

    void SimulationServer::runtime_attach() {
        ++runtimes;
        _refresh_clock();
    }

    void SimulationServer::runtime_detach() {
        ERR_FAIL_COND(runtimes <= 0);
        --runtimes;
        _refresh_clock();
    }

    void SimulationServer::clock_subscribe(const Callable &p_on_advanced) {
        clock_hold();
        connect(simulation_advanced_signal, p_on_advanced);
    }

    void SimulationServer::clock_unsubscribe(const Callable &p_on_advanced) {
        disconnect(simulation_advanced_signal, p_on_advanced);
        clock_release();
    }

    /// The clock runs while it is held, a SimulationRuntime is attached and the runtime is not
    /// paused. It ticks on SceneTree's `process_frame`, before any node's `_process` - not in a node
    /// of its own: one created on the first hold was added to the root while the root was adding
    /// the main scene, and never ticked (FINDINGS.md 2026-09-30)
    void SimulationServer::_refresh_clock() {
        const bool running = clock_holders > 0 && runtimes > 0 && !paused;
        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (running == clock_running || tree == nullptr) {
            return;
        }
        clock_running = running;
        if (clock_running) {
            tree->connect("process_frame", callable_mp(this, &SimulationServer::_on_process_frame));
            return;
        }
        tree->disconnect("process_frame", callable_mp(this, &SimulationServer::_on_process_frame));
    }

    void SimulationServer::_on_process_frame() {
        const SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        simulation_advance(tree->get_root()->get_process_delta_time());
    }

    void SimulationServer::simulation_advance(const double p_frame_delta) {
        const double frame_delta = MIN(p_frame_delta, MAX_FRAME_DELTA);
        if (!(current_simulation_speed == simulation_speed)) {
            // a tape's motor: the running speed closes on the one set, most of the way in
            // speed_change_time
            const double closing = speed_change_time > 0.0 ? 1.0 - Math::exp(-frame_delta / speed_change_time) : 1.0;
            current_simulation_speed += (simulation_speed - current_simulation_speed) * closing;
            if (Math::abs(simulation_speed - current_simulation_speed) < SPEED_SETTLED) {
                current_simulation_speed = simulation_speed;
            }
            emit_signal(simulation_current_speed_changed_signal);
        }
        const double seconds = frame_delta * current_simulation_speed;
        if (seconds <= 0.0) {
            return;
        }
        // equal slices, so a frame of 0.25 s is 3 x 0.083 s and not 0.1 + 0.1 + 0.05
        const int slices = static_cast<int>(Math::ceil(seconds / MAX_SLICE_TIME));
        const double slice = seconds / slices;
        for (int index = 0; index < slices; ++index) {
            simulation_time += slice;
            // the launchers look at whole minutes (EvLaunch.cpp:197-211): the clock says so when
            // one passes, not every slice
            const double previous = time_of_day;
            time_of_day = Math::fposmod(time_of_day + (slice / SECONDS_PER_HOUR), HOURS_PER_DAY);
            if (!(Math::floor(previous * MINUTES_PER_HOUR) == Math::floor(time_of_day * MINUTES_PER_HOUR))) {
                emit_signal(time_of_day_changed_signal);
            }
            emit_signal(simulation_advanced_signal, slice);
        }
    }

    void SimulationServer::set_simulation_speed(const double p_speed) {
        if (simulation_speed == p_speed) {
            return;
        }
        simulation_speed = p_speed;
        emit_signal(simulation_speed_changed_signal);
    }

    double SimulationServer::get_simulation_speed() const {
        return simulation_speed;
    }

    double SimulationServer::simulation_get_current_speed() const {
        return current_simulation_speed;
    }

    void SimulationServer::simulation_reset_speed() {
        set_simulation_speed(1.0);
        if (current_simulation_speed == simulation_speed) {
            return;
        }
        current_simulation_speed = simulation_speed;
        emit_signal(simulation_current_speed_changed_signal);
    }

    void SimulationServer::set_light_level(const double p_level) {
        light_level = p_level;
    }

    double SimulationServer::get_light_level() const {
        return light_level;
    }

    void SimulationServer::set_air_temperature(const double p_temperature) {
        air_temperature = p_temperature;
    }

    double SimulationServer::get_air_temperature() const {
        return air_temperature;
    }

    void SimulationServer::simulation_pause() {
        if (paused) {
            return;
        }
        paused = true;
        _refresh_clock();
        emit_signal(simulation_paused_signal);
    }

    void SimulationServer::simulation_unpause() {
        if (!paused) {
            return;
        }
        paused = false;
        _refresh_clock();
        emit_signal(simulation_unpaused_signal);
    }

    bool SimulationServer::simulation_is_paused() const {
        return paused;
    }
} // namespace godot
