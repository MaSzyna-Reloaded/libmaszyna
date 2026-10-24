#include "MaszynaLuaMemoryModule.hpp"
#include "lauxlib.h"
#include "maszyna/scenario/MaszynaScenarioEventServer.hpp"
#include "scripting/lua/LuaHandle.hpp"
#include "scripting/lua/LuaModules.hpp"
#include "scripting/lua/LuaVariant.hpp"

namespace godot {
    /// find(name) - the scenery's memory cell of the name, nil when there is none
    static int memory_find(lua_State *p_state) {
        const StringName name = String::utf8(luaL_checkstring(p_state, 1));
        LuaHandle::push(
                p_state, LuaModules::server<MaszynaScenarioEventServer>(p_state)->memory_get_rid_by_name(name),
                MaszynaLuaMemoryModule::handle_kind);
        return 1;
    }

    /// read(m) - text, value1, value2
    static int memory_read(lua_State *p_state) {
        const RID memory = LuaHandle::check(p_state, 1, MaszynaLuaMemoryModule::handle_kind);
        const MaszynaScenarioEventServer *events = LuaModules::server<MaszynaScenarioEventServer>(p_state);
        LuaVariant::push(p_state, events->memory_get_text(memory));
        lua_pushnumber(p_state, events->memory_get_value1(memory));
        lua_pushnumber(p_state, events->memory_get_value2(memory));
        return 3;
    }

    /// write(m, text, value1, value2)
    static int memory_write(lua_State *p_state) {
        const RID memory = LuaHandle::check(p_state, 1, MaszynaLuaMemoryModule::handle_kind);
        const String text = String::utf8(luaL_checkstring(p_state, 2));
        const double value1 = luaL_checknumber(p_state, 3);
        const double value2 = luaL_checknumber(p_state, 4);
        LuaModules::server<MaszynaScenarioEventServer>(p_state)->memory_set_values(memory, text, value1, value2);
        return 0;
    }

    /// on_values_changed(m, fn) - fn() whenever the memory's values change
    static int memory_on_values_changed(lua_State *p_state) {
        return LuaModules::subscribe(
                p_state, MaszynaLuaMemoryModule::values_changed_signal, MaszynaLuaMemoryModule::handle_kind);
    }

    ScriptHandleKind MaszynaLuaMemoryModule::handle_kind = ScriptHandleKind::NONE;
    ScenarioScriptServer::ScriptSignal MaszynaLuaMemoryModule::values_changed_signal =
            ScenarioScriptServer::SIGNAL_FIRST_REGISTERED;

    static const luaL_Reg FUNCTIONS[] = {
            {"find", memory_find},   {"read", memory_read},
            {"write", memory_write}, {"on_values_changed", memory_on_values_changed},
            {nullptr, nullptr},
    };

    void MaszynaLuaMemoryModule::register_module() {
        handle_kind = ScenarioScriptServer::handle_kind_register(HANDLE_KIND_NAME);
        values_changed_signal = ScenarioScriptServer::signal_register(VALUES_CHANGED_SIGNAL_NAME);
        LuaModules::opener_register(&MaszynaLuaMemoryModule::open);
    }

    void MaszynaLuaMemoryModule::open(lua_State *p_state) {
        luaL_getsubtable(p_state, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
        lua_getglobal(p_state, LuaModules::ROOT_NAME);
        lua_newtable(p_state);
        luaL_setfuncs(p_state, FUNCTIONS, 0);
        lua_pushvalue(p_state, -1);
        lua_setfield(p_state, -4, (String(LuaModules::ROOT_NAME) + "." + MODULE_NAME).utf8().get_data());
        lua_setfield(p_state, -2, MODULE_NAME);
        lua_pop(p_state, 2);
    }

    void MaszynaLuaMemoryModule::deliver_values_changed(const RID &p_memory) {
        if (ScenarioScriptServer *scripts = ScenarioScriptServer::get_instance(); scripts != nullptr) {
            scripts->script_deliver(values_changed_signal, p_memory, Vector<ScriptArgument>(), RID());
        }
    }
} // namespace godot
