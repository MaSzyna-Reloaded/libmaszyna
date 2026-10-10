#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "LuaVariant.hpp"
#include "scenario/ScenarioEventServer.hpp"

namespace godot {
    /// find(name) - the scenery's memory cell of the name, nil when there is none
    static int memory_find(lua_State *p_state) {
        const StringName name = String::utf8(luaL_checkstring(p_state, 1));
        LuaHandle::push(
                p_state, LuaModules::server<ScenarioEventServer>(p_state)->memory_get_rid_by_name(name),
                ScriptHandleKind::MEMORY);
        return 1;
    }

    /// read(m) - text, value1, value2
    static int memory_read(lua_State *p_state) {
        const RID memory = LuaHandle::check(p_state, 1, ScriptHandleKind::MEMORY);
        const ScenarioEventServer *events = LuaModules::server<ScenarioEventServer>(p_state);
        LuaVariant::push(p_state, events->memory_get_text(memory));
        lua_pushnumber(p_state, events->memory_get_value1(memory));
        lua_pushnumber(p_state, events->memory_get_value2(memory));
        return 3;
    }

    /// write(m, text, value1, value2)
    static int memory_write(lua_State *p_state) {
        const RID memory = LuaHandle::check(p_state, 1, ScriptHandleKind::MEMORY);
        const String text = String::utf8(luaL_checkstring(p_state, 2));
        const double value1 = luaL_checknumber(p_state, 3);
        const double value2 = luaL_checknumber(p_state, 4);
        LuaModules::server<ScenarioEventServer>(p_state)->memory_set_values(memory, text, value1, value2);
        return 0;
    }

    /// on_values_changed(m, fn) - fn() whenever the memory's values change
    static int memory_on_values_changed(lua_State *p_state) {
        return LuaModules::subscribe(
                p_state, ScenarioScriptServer::SIGNAL_MEMORY_VALUES_CHANGED, ScriptHandleKind::MEMORY);
    }

    const luaL_Reg LuaModules::MEMORY[] = {
            {"find", memory_find},   {"read", memory_read},
            {"write", memory_write}, {"on_values_changed", memory_on_values_changed},
            {nullptr, nullptr},
    };
} // namespace godot
