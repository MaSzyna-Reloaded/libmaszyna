#include "ScenarioScriptAction.hpp"
#include "ScenarioScriptServer.hpp"

namespace godot {
    void ScenarioScriptAction::_bind_methods() {}

    void ScenarioScriptAction::run(const RID &p_event, const RID &p_activator) {
        ScenarioScriptServer *scripts = ScenarioScriptServer::get_instance();
        ERR_FAIL_NULL(scripts);
        scripts->_run_hook(hook, p_event, p_activator, ScenarioScriptServer::BRANCH_RUN);
    }

    void ScenarioScriptAction::run_else(const RID &p_event, const RID &p_activator) {
        ScenarioScriptServer *scripts = ScenarioScriptServer::get_instance();
        ERR_FAIL_NULL(scripts);
        scripts->_run_hook(hook, p_event, p_activator, ScenarioScriptServer::BRANCH_ELSE);
    }

    void ScenarioScriptAction::set_hook(const RID &p_hook) {
        hook = p_hook;
    }
} // namespace godot
