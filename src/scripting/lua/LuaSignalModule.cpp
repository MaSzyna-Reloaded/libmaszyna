#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "LuaVariant.hpp"
#include "signalling/SignallingServer.hpp"

namespace godot {
    /// The table of named arguments at p_index; an empty one for nil
    static Dictionary check_arguments(lua_State *p_state, const int p_index) {
        const Variant arguments = LuaVariant::to_variant(p_state, p_index);
        luaL_argcheck(
                p_state, arguments.get_type() == Variant::DICTIONARY || arguments.get_type() == Variant::NIL, p_index,
                "a table of named arguments expected");
        return arguments.get_type() == Variant::NIL ? Dictionary() : Dictionary(arguments);
    }

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
        LuaModules::server<SignallingServer>(p_state)->system_send_event(system, event, check_arguments(p_state, 3));
        return 0;
    }

    /// find_source(name) - the signalling source of the name, nil when there is none
    static int signal_find_source(lua_State *p_state) {
        const StringName name = String::utf8(luaL_checkstring(p_state, 1));
        LuaHandle::push(
                p_state, LuaModules::server<SignallingServer>(p_state)->source_get_rid_by_name(name),
                ScriptHandleKind::SIGNALLING_SOURCE);
        return 1;
    }

    /// source_send_event(s, event, arguments) - a report of the source to its system, e.g. a sensor's
    /// passage ("impulse", {polarity = ...})
    static int signal_source_send_event(lua_State *p_state) {
        const RID source = LuaHandle::check(p_state, 1, ScriptHandleKind::SIGNALLING_SOURCE);
        const StringName event = String::utf8(luaL_checkstring(p_state, 2));
        LuaModules::server<SignallingServer>(p_state)->source_send_event(source, event, check_arguments(p_state, 3));
        return 0;
    }

    /// find_route(name) - the route of the name, nil when there is none
    static int signal_find_route(lua_State *p_state) {
        const StringName name = String::utf8(luaL_checkstring(p_state, 1));
        LuaHandle::push(
                p_state, LuaModules::server<SignallingServer>(p_state)->route_get_rid_by_name(name),
                ScriptHandleKind::SIGNALLING_ROUTE);
        return 1;
    }

    /// route_state(r) - "idle", "setting", "closed", "locked" or "releasing"
    static int signal_route_state(lua_State *p_state) {
        const RID route = LuaHandle::check(p_state, 1, ScriptHandleKind::SIGNALLING_ROUTE);
        lua_pushstring(
                p_state,
                ScenarioScriptServer::ROUTE_STATE_NAMES[LuaModules::server<SignallingServer>(p_state)->route_get_state(
                        route)]);
        return 1;
    }

    /// route_set(r) - asks the route's system to set it
    static int signal_route_set(lua_State *p_state) {
        const RID route = LuaHandle::check(p_state, 1, ScriptHandleKind::SIGNALLING_ROUTE);
        LuaModules::server<SignallingServer>(p_state)->route_request_set(route);
        return 0;
    }

    /// route_cancel(r) - asks the route's system to cancel it
    static int signal_route_cancel(lua_State *p_state) {
        const RID route = LuaHandle::check(p_state, 1, ScriptHandleKind::SIGNALLING_ROUTE);
        LuaModules::server<SignallingServer>(p_state)->route_request_cancel(route);
        return 0;
    }

    /// route_release(r) - asks the route's system to release it by hand
    static int signal_route_release(lua_State *p_state) {
        const RID route = LuaHandle::check(p_state, 1, ScriptHandleKind::SIGNALLING_ROUTE);
        LuaModules::server<SignallingServer>(p_state)->route_request_release(route);
        return 0;
    }

    /// route_emergency_release(r) - asks the route's system to release it at once
    static int signal_route_emergency_release(lua_State *p_state) {
        const RID route = LuaHandle::check(p_state, 1, ScriptHandleKind::SIGNALLING_ROUTE);
        LuaModules::server<SignallingServer>(p_state)->route_request_emergency_release(route);
        return 0;
    }

    /// on_route_state_changed(r, fn) - fn(state) whenever the route goes to another state
    static int signal_on_route_state_changed(lua_State *p_state) {
        return LuaModules::subscribe(
                p_state, ScenarioScriptServer::SIGNAL_ROUTE_STATE_CHANGED, ScriptHandleKind::SIGNALLING_ROUTE);
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
            {"find_source", signal_find_source},
            {"source_send_event", signal_source_send_event},
            {"find_route", signal_find_route},
            {"route_state", signal_route_state},
            {"route_set", signal_route_set},
            {"route_cancel", signal_route_cancel},
            {"route_release", signal_route_release},
            {"route_emergency_release", signal_route_emergency_release},
            {"on_route_state_changed", signal_on_route_state_changed},
            {nullptr, nullptr},
    };
} // namespace godot
