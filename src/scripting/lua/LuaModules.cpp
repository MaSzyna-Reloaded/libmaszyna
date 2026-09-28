#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "LuaScriptContext.hpp"
#include "scripting/ScenarioScriptServer.hpp"

namespace godot {
    /// maszyna.cancel(subscription) - stops a timer or a subscription of this script
    static int cancel(lua_State *p_state) {
        const RID subscription = LuaHandle::check(p_state, 1, ScriptHandleKind::SUBSCRIPTION);
        ScenarioScriptServer *scripts = LuaModules::server<ScenarioScriptServer>(p_state);
        lua_pushboolean(
                p_state, static_cast<int>(scripts->script_cancel(
                                 LuaScriptContext::from_state(p_state)->get_rid(), subscription)));
        return 1;
    }

    int LuaModules::subscribe(
            lua_State *p_state, const ScenarioScriptServer::ScriptSignal p_signal, const ScriptHandleKind p_target) {
        const RID target = LuaHandle::check(p_state, 1, p_target);
        LuaScriptContext *context = LuaScriptContext::from_state(p_state);
        const int64_t function = context->keep_function(p_state, 2);
        ScenarioScriptServer *scripts = server<ScenarioScriptServer>(p_state);
        LuaHandle::push(
                p_state, scripts->script_subscribe(context->get_rid(), p_signal, target, function),
                ScriptHandleKind::SUBSCRIPTION);
        return 1;
    }

    const luaL_Reg LuaModules::ROOT[] = {
            {"cancel", cancel},
            {nullptr, nullptr},
    };

    // Only the addresses of the module tables are taken, which is a constant initialization
    // NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
    const LuaModules::Module LuaModules::MODULES[] = {
            {"sim", SIM},       {"vehicle", VEHICLE}, {"cabin", CABIN},   {"driver", DRIVER}, {"event", EVENT},
            {"memory", MEMORY}, {"track", TRACK},     {"signal", SIGNAL}, {"log", LOG},       {"player", PLAYER},
            {"camera", CAMERA}, {"hud", HUD},         {nullptr, nullptr},
    };
} // namespace godot
