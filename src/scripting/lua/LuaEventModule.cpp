#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "LuaScriptContext.hpp"
#include "LuaVariant.hpp"
#include "scenario/ScenarioEventServer.hpp"

namespace godot {
    /// A function field of the table at p_index, kept for later; NO_FUNCTION when there is none
    static int64_t keep_field_function(lua_State *p_state, const int p_index, const char *p_field) {
        lua_getfield(p_state, p_index, p_field);
        const int64_t function = lua_isnil(p_state, -1)
                                         ? ScenarioScriptServer::NO_FUNCTION
                                         : LuaScriptContext::from_state(p_state)->keep_function(p_state, -1);
        lua_pop(p_state, 1);
        return function;
    }

    /// create{name =, delay =, random_delay =, run = fn(event, activator), run_else = fn} -
    /// an event of the scenario, which the scenery's events may queue by its name too; run
    /// when it runs, run_else when its condition failed. The script owns it.
    static int event_create(lua_State *p_state) {
        luaL_checktype(p_state, 1, LUA_TTABLE);
        lua_getfield(p_state, 1, "name");
        const StringName name = String::utf8(luaL_optstring(p_state, -1, ""));
        lua_getfield(p_state, 1, "delay");
        const double delay = luaL_optnumber(p_state, -1, 0.0);
        lua_getfield(p_state, 1, "random_delay");
        const double random_delay = luaL_optnumber(p_state, -1, 0.0);
        lua_pop(p_state, 3);
        const int64_t function = keep_field_function(p_state, 1, "run");
        const int64_t else_function = keep_field_function(p_state, 1, "run_else");
        ScenarioScriptServer *scripts = LuaModules::server<ScenarioScriptServer>(p_state);
        ScenarioEventServer *events = LuaModules::server<ScenarioEventServer>(p_state);
        const RID event =
                scripts->script_event_create(LuaScriptContext::from_state(p_state)->get_rid(), function, else_function);
        events->event_set_name(event, name);
        events->event_set_delay(event, delay);
        events->event_set_random_delay(event, random_delay);
        LuaHandle::push(p_state, event, ScriptHandleKind::EVENT);
        return 1;
    }

    /// find(name) - the event of the name, the scenery's or a script's; nil when none
    static int event_find(lua_State *p_state) {
        const StringName name = String::utf8(luaL_checkstring(p_state, 1));
        LuaHandle::push(
                p_state, LuaModules::server<ScenarioEventServer>(p_state)->event_get_rid_by_name(name),
                ScriptHandleKind::EVENT);
        return 1;
    }

    static int event_exists(lua_State *p_state) {
        const StringName name = String::utf8(luaL_checkstring(p_state, 1));
        lua_pushboolean(
                p_state,
                static_cast<int>(
                        LuaModules::server<ScenarioEventServer>(p_state)->event_get_rid_by_name(name).is_valid()));
        return 1;
    }

    static int event_name(lua_State *p_state) {
        const RID event = LuaHandle::check(p_state, 1, ScriptHandleKind::EVENT);
        LuaVariant::push(p_state, LuaModules::server<ScenarioEventServer>(p_state)->event_get_name(event));
        return 1;
    }

    static int event_is_queued(lua_State *p_state) {
        const RID event = LuaHandle::check(p_state, 1, ScriptHandleKind::EVENT);
        lua_pushboolean(
                p_state, static_cast<int>(LuaModules::server<ScenarioEventServer>(p_state)->event_is_queued(event)));
        return 1;
    }

    /// queue(e, activator, extra_delay) - runs the event after its delay plus extra_delay;
    /// false when it is queued already
    static int event_queue(lua_State *p_state) {
        const RID event = LuaHandle::check(p_state, 1, ScriptHandleKind::EVENT);
        const RID activator = LuaHandle::optional(p_state, 2, ScriptHandleKind::VEHICLE);
        const double extra_delay = luaL_optnumber(p_state, 3, 0.0);
        lua_pushboolean(
                p_state, static_cast<int>(LuaModules::server<ScenarioEventServer>(p_state)->event_queue(
                                 event, activator, extra_delay)));
        return 1;
    }

    /// free(e) - frees an event this script created
    static int event_free(lua_State *p_state) {
        const RID event = LuaHandle::check(p_state, 1, ScriptHandleKind::EVENT);
        ScenarioScriptServer *scripts = LuaModules::server<ScenarioScriptServer>(p_state);
        if (!scripts->script_event_free(LuaScriptContext::from_state(p_state)->get_rid(), event)) {
            return luaL_argerror(p_state, 1, "not an event this script created");
        }
        return 0;
    }

    /// on_launched(e, fn) - fn(activator) whenever the event runs
    static int event_on_launched(lua_State *p_state) {
        return LuaModules::subscribe(p_state, ScenarioScriptServer::SIGNAL_EVENT_LAUNCHED, ScriptHandleKind::EVENT);
    }

    const luaL_Reg LuaModules::EVENT[] = {
            {"create", event_create},
            {"find", event_find},
            {"exists", event_exists},
            {"name", event_name},
            {"is_queued", event_is_queued},
            {"queue", event_queue},
            {"free", event_free},
            {"on_launched", event_on_launched},
            {nullptr, nullptr},
    };
} // namespace godot
