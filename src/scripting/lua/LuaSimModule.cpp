#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "LuaScriptContext.hpp"
#include "scripting/ScenarioScriptServer.hpp"
#include "simulation/SimulationServer.hpp"
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    /// Seconds of simulation time since the scenario started
    static int sim_time(lua_State *p_state) {
        lua_pushnumber(p_state, LuaModules::server<SimulationServer>(p_state)->simulation_get_time());
        return 1;
    }

    /// Hours since midnight, e.g. 13.5 at half past one
    static int sim_time_of_day(lua_State *p_state) {
        lua_pushnumber(p_state, LuaModules::server<SimulationServer>(p_state)->get_time_of_day());
        return 1;
    }

    static int sim_is_paused(lua_State *p_state) {
        lua_pushboolean(
                p_state, static_cast<int>(LuaModules::server<SimulationServer>(p_state)->simulation_is_paused()));
        return 1;
    }

    /// A number between a and b (the original's Random(a, b), lua.cpp:352)
    static int sim_random(lua_State *p_state) {
        const double from = luaL_checknumber(p_state, 1);
        const double to = luaL_checknumber(p_state, 2);
        lua_pushnumber(p_state, UtilityFunctions::randf_range(from, to));
        return 1;
    }

    static int sim_create_timer(lua_State *p_state, const ScenarioScriptServer::Timer p_timer) {
        const double seconds = luaL_checknumber(p_state, 1);
        LuaScriptContext *context = LuaScriptContext::from_state(p_state);
        const int64_t function = context->keep_function(p_state, 2);
        ScenarioScriptServer *scripts = LuaModules::server<ScenarioScriptServer>(p_state);
        LuaHandle::push(
                p_state, scripts->script_timer_create(context->get_rid(), seconds, function, p_timer),
                ScriptHandleKind::SUBSCRIPTION);
        return 1;
    }

    /// after(seconds, fn) - fn() once, after the seconds of simulation time
    static int sim_after(lua_State *p_state) {
        return sim_create_timer(p_state, ScenarioScriptServer::TIMER_ONCE);
    }

    /// every(seconds, fn) - fn() every so many seconds of simulation time
    static int sim_every(lua_State *p_state) {
        return sim_create_timer(p_state, ScenarioScriptServer::TIMER_REPEATING);
    }

    const luaL_Reg LuaModules::SIM[] = {
            {"time", sim_time},           {"time_of_day", sim_time_of_day},
            {"is_paused", sim_is_paused}, {"random", sim_random},
            {"after", sim_after},         {"every", sim_every},
            {nullptr, nullptr},
    };
} // namespace godot
