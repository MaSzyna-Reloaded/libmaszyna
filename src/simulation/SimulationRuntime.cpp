#include "SimulationRuntime.hpp"

#include "simulation/SimulationServer.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/error_macros.hpp>

namespace godot {

    void SimulationRuntime::_bind_methods() {}

    void SimulationRuntime::_notification(const int p_what) {
        if ((p_what != NOTIFICATION_ENTER_TREE && p_what != NOTIFICATION_EXIT_TREE) ||
            Engine::get_singleton()->is_editor_hint()) {
            return;
        }
        SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        if (p_what == NOTIFICATION_ENTER_TREE) {
            runtime->runtime_attach();
            return;
        }
        runtime->runtime_detach();
    }

} // namespace godot
