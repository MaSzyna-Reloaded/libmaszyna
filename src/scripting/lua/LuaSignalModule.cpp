#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "LuaVariant.hpp"
#include "signalling/SignallingServer.hpp"

namespace godot {
    /// find_head(name) - the signal head of the name, nil when there is none
    static int signal_find_head(lua_State *p_state) {
        const StringName name = String::utf8(luaL_checkstring(p_state, 1));
        LuaHandle::push(
                p_state, LuaModules::server<SignallingServer>(p_state)->signal_head_get_rid_by_name(name),
                ScriptHandleKind::SIGNAL_HEAD);
        return 1;
    }

    /// aspect(h) - the name of the aspect the head shows
    static int signal_aspect(lua_State *p_state) {
        const RID head = LuaHandle::check(p_state, 1, ScriptHandleKind::SIGNAL_HEAD);
        LuaVariant::push(p_state, LuaModules::server<SignallingServer>(p_state)->signal_head_get_aspect(head));
        return 1;
    }

    /// aspects(h) - the names of the aspects the head can show
    static int signal_aspects(lua_State *p_state) {
        const RID head = LuaHandle::check(p_state, 1, ScriptHandleKind::SIGNAL_HEAD);
        LuaVariant::push(p_state, LuaModules::server<SignallingServer>(p_state)->signal_head_get_aspects(head));
        return 1;
    }

    /// find_system(name) - the signalling system of the name, nil when there is none
    static int signal_find_system(lua_State *p_state) {
        const StringName name = String::utf8(luaL_checkstring(p_state, 1));
        LuaHandle::push(
                p_state, LuaModules::server<SignallingServer>(p_state)->system_get_rid_by_name(name),
                ScriptHandleKind::SIGNALLING_SYSTEM);
        return 1;
    }

    /// send_event(system, event, arguments) - the system decides what its heads show, so a
    /// script asks it ("lights", {signal_head = h, aspect = "S1"}) instead of lighting them
    static int signal_send_event(lua_State *p_state) {
        const RID system = LuaHandle::check(p_state, 1, ScriptHandleKind::SIGNALLING_SYSTEM);
        const StringName event = String::utf8(luaL_checkstring(p_state, 2));
        const Variant arguments = LuaVariant::to_variant(p_state, 3);
        luaL_argcheck(
                p_state, arguments.get_type() == Variant::DICTIONARY || arguments.get_type() == Variant::NIL, 3,
                "a table of named arguments expected");
        LuaModules::server<SignallingServer>(p_state)->system_send_event(
                system, event, arguments.get_type() == Variant::NIL ? Dictionary() : Dictionary(arguments));
        return 0;
    }

    /// on_aspect_changed(h, fn) - fn(aspect) whenever the head shows another aspect
    static int signal_on_aspect_changed(lua_State *p_state) {
        return LuaModules::subscribe(
                p_state, ScenarioScriptServer::SIGNAL_SIGNAL_HEAD_ASPECT_CHANGED, ScriptHandleKind::SIGNAL_HEAD);
    }

    const luaL_Reg LuaModules::SIGNAL[] = {
            {"find_head", signal_find_head},
            {"aspect", signal_aspect},
            {"aspects", signal_aspects},
            {"find_system", signal_find_system},
            {"send_event", signal_send_event},
            {"on_aspect_changed", signal_on_aspect_changed},
            {nullptr, nullptr},
    };
} // namespace godot
