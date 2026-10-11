#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "LuaScriptContext.hpp"
#include "LuaVariant.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"

namespace godot {
    /// The cab layer of this script's context; raises an error when none was attached
    static Ref<ScenarioScriptCabinImplementation> cabin(lua_State *p_state) {
        const Ref<ScenarioScriptCabinImplementation> implementation =
                LuaModules::server<ScenarioScriptServer>(p_state)->context_get_cabin_implementation(
                        LuaScriptContext::from_state(p_state)->get_rid());
        if (implementation.is_null()) {
            // luaL_error is the Lua C API's vararg error call
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            luaL_error(p_state, "no cabs are available to scripts here");
        }
        return implementation;
    }

    /// act(cabin, control_id, action, value) - manipulates a control of the cabin as the
    /// driver's hand does; action is "increase", "decrease", "hold", "release", "toggle" or "set".
    /// Returns what the control answered.
    static int cabin_act(lua_State *p_state) {
        const RID cabin_rid = LuaHandle::check(p_state, 1, ScriptHandleKind::CABIN);
        const StringName control_id = String::utf8(luaL_checkstring(p_state, 2));
        const StringName action = String::utf8(luaL_checkstring(p_state, 3));
        const Variant value = LuaVariant::to_variant(p_state, 4);
        LuaVariant::push(p_state, cabin(p_state)->act(cabin_rid, control_id, action, value));
        return 1;
    }

    /// control(cabin, control_id) - where the control stands
    static int cabin_control(lua_State *p_state) {
        const RID cabin_rid = LuaHandle::check(p_state, 1, ScriptHandleKind::CABIN);
        const StringName control_id = String::utf8(luaL_checkstring(p_state, 2));
        LuaVariant::push(p_state, cabin(p_state)->get_control(cabin_rid, control_id));
        return 1;
    }

    /// controls(cabin) - the ids of the cabin's controls
    static int cabin_controls(lua_State *p_state) {
        const RID cabin_rid = LuaHandle::check(p_state, 1, ScriptHandleKind::CABIN);
        LuaVariant::push(p_state, cabin(p_state)->get_controls(cabin_rid));
        return 1;
    }

    /// The cabin as a handle, nil for none
    static int push_cabin(lua_State *p_state, const RID &p_cabin) {
        if (!p_cabin.is_valid()) {
            lua_pushnil(p_state);
            return 1;
        }
        LuaHandle::push(p_state, p_cabin, ScriptHandleKind::CABIN);
        return 1;
    }

    /// driver_cabin(v) - the cabin whose driver the vehicle answers to, nil when nobody drives it
    static int cabin_driver_cabin(lua_State *p_state) {
        const RID vehicle = LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
        return push_cabin(p_state, LuaModules::server<RailVehicleServer>(p_state)->vehicle_get_driver_cabin(vehicle));
    }

    /// front_cabin(v), rear_cabin(v), machine_room(v) - the vehicle's cabin of that kind, nil when
    /// it has none
    static int cabin_front_cabin(lua_State *p_state) {
        const RID vehicle = LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
        return push_cabin(p_state, LuaModules::server<RailVehicleServer>(p_state)->vehicle_get_front_cabin(vehicle));
    }

    static int cabin_rear_cabin(lua_State *p_state) {
        const RID vehicle = LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
        return push_cabin(p_state, LuaModules::server<RailVehicleServer>(p_state)->vehicle_get_rear_cabin(vehicle));
    }

    static int cabin_machine_room(lua_State *p_state) {
        const RID vehicle = LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
        return push_cabin(p_state, LuaModules::server<RailVehicleServer>(p_state)->vehicle_get_machine_room(vehicle));
    }

    /// on_control_changed(v, fn) - fn(cabin, control_id, value) whenever a control of the
    /// vehicle's cabins changes
    static int cabin_on_control_changed(lua_State *p_state) {
        return LuaModules::subscribe(
                p_state, ScenarioScriptServer::SIGNAL_CABIN_CONTROL_CHANGED, ScriptHandleKind::VEHICLE);
    }

    const luaL_Reg LuaModules::CABIN[] = {
            {"act", cabin_act},
            {"control", cabin_control},
            {"controls", cabin_controls},
            {"driver_cabin", cabin_driver_cabin},
            {"front_cabin", cabin_front_cabin},
            {"rear_cabin", cabin_rear_cabin},
            {"machine_room", cabin_machine_room},
            {"on_control_changed", cabin_on_control_changed},
            {nullptr, nullptr},
    };
} // namespace godot
