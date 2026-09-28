#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "tracks/TrackServer.hpp"
#include <iterator>

namespace godot {
    /// SWITCH_TRACK_NAMES with the terminator luaL_checkoption wants
    static constexpr const char *SWITCH_TRACK_OPTIONS[] = {
            ScenarioScriptServer::SWITCH_TRACK_NAMES[TrackServer::TRACK_COMMON],
            ScenarioScriptServer::SWITCH_TRACK_NAMES[TrackServer::TRACK_DIVERGING],
            nullptr,
    };

    /// find(name) - the track of the name, nil when there is none
    static int track_find(lua_State *p_state) {
        const String name = String::utf8(luaL_checkstring(p_state, 1));
        LuaHandle::push(
                p_state, LuaModules::server<TrackServer>(p_state)->track_get_rid_by_name(name),
                ScriptHandleKind::TRACK);
        return 1;
    }

    static int track_is_occupied(lua_State *p_state) {
        const RID track = LuaHandle::check(p_state, 1, ScriptHandleKind::TRACK);
        lua_pushboolean(p_state, static_cast<int>(LuaModules::server<TrackServer>(p_state)->track_is_occupied(track)));
        return 1;
    }

    /// vehicles(t) - the vehicles on the track
    static int track_vehicles(lua_State *p_state) {
        const RID track = LuaHandle::check(p_state, 1, ScriptHandleKind::TRACK);
        const TypedArray<RID> vehicles = LuaModules::server<TrackServer>(p_state)->track_get_vehicles(track);
        lua_createtable(p_state, static_cast<int>(vehicles.size()), 0);
        for (int64_t i = 0; i < vehicles.size(); i++) {
            LuaHandle::push(p_state, vehicles[i], ScriptHandleKind::VEHICLE);
            lua_rawseti(p_state, -2, i + 1);
        }
        return 1;
    }

    /// switch_get(t) - "common" or "diverging"
    static int track_switch_get(lua_State *p_state) {
        const RID track = LuaHandle::check(p_state, 1, ScriptHandleKind::TRACK);
        const int active = LuaModules::server<TrackServer>(p_state)->switch_get_active_track(track);
        luaL_argcheck(
                p_state, active >= 0 && active < static_cast<int>(std::size(ScenarioScriptServer::SWITCH_TRACK_NAMES)),
                1, "not a switch");
        lua_pushstring(p_state, ScenarioScriptServer::SWITCH_TRACK_NAMES[active]);
        return 1;
    }

    /// switch_set(t, "common" | "diverging") - throws the switch
    static int track_switch_set(lua_State *p_state) {
        const RID track = LuaHandle::check(p_state, 1, ScriptHandleKind::TRACK);
        const int active = luaL_checkoption(p_state, 2, nullptr, SWITCH_TRACK_OPTIONS);
        LuaModules::server<TrackServer>(p_state)->switch_set_active_track(track, active);
        return 0;
    }

    /// isolated_find(name) - the isolated section of the name, nil when there is none
    static int track_isolated_find(lua_State *p_state) {
        const StringName name = String::utf8(luaL_checkstring(p_state, 1));
        LuaHandle::push(
                p_state, LuaModules::server<TrackServer>(p_state)->isolated_get_rid_by_name(name),
                ScriptHandleKind::ISOLATED);
        return 1;
    }

    static int track_isolated_is_occupied(lua_State *p_state) {
        const RID isolated = LuaHandle::check(p_state, 1, ScriptHandleKind::ISOLATED);
        lua_pushboolean(
                p_state, static_cast<int>(LuaModules::server<TrackServer>(p_state)->isolated_is_occupied(isolated)));
        return 1;
    }

    /// on_vehicle_heading_to_start(t, fn) - fn(vehicle) when a vehicle on the track starts
    /// moving towards the track's start
    static int track_on_vehicle_heading_to_start(lua_State *p_state) {
        return LuaModules::subscribe(
                p_state, ScenarioScriptServer::SIGNAL_VEHICLE_HEADING_TO_TRACK_START, ScriptHandleKind::TRACK);
    }

    static int track_on_vehicle_heading_to_end(lua_State *p_state) {
        return LuaModules::subscribe(
                p_state, ScenarioScriptServer::SIGNAL_VEHICLE_HEADING_TO_TRACK_END, ScriptHandleKind::TRACK);
    }

    /// on_vehicle_stopped(t, fn) - fn(vehicle) when a vehicle stops on the track
    static int track_on_vehicle_stopped(lua_State *p_state) {
        return LuaModules::subscribe(
                p_state, ScenarioScriptServer::SIGNAL_VEHICLE_STOPPED_ON_TRACK, ScriptHandleKind::TRACK);
    }

    /// on_isolated_occupied(i, fn) - fn(vehicle) when the first vehicle comes onto the section
    static int track_on_isolated_occupied(lua_State *p_state) {
        return LuaModules::subscribe(
                p_state, ScenarioScriptServer::SIGNAL_ISOLATED_OCCUPIED, ScriptHandleKind::ISOLATED);
    }

    /// on_isolated_freed(i, fn) - fn(vehicle) when the last vehicle leaves the section
    static int track_on_isolated_freed(lua_State *p_state) {
        return LuaModules::subscribe(p_state, ScenarioScriptServer::SIGNAL_ISOLATED_FREED, ScriptHandleKind::ISOLATED);
    }

    /// on_switch_changed(t, fn) - fn("common" | "diverging") when the switch moves
    static int track_on_switch_changed(lua_State *p_state) {
        return LuaModules::subscribe(p_state, ScenarioScriptServer::SIGNAL_SWITCH_CHANGED, ScriptHandleKind::TRACK);
    }

    const luaL_Reg LuaModules::TRACK[] = {
            {"find", track_find},
            {"is_occupied", track_is_occupied},
            {"vehicles", track_vehicles},
            {"switch_get", track_switch_get},
            {"switch_set", track_switch_set},
            {"isolated_find", track_isolated_find},
            {"isolated_is_occupied", track_isolated_is_occupied},
            {"on_vehicle_heading_to_start", track_on_vehicle_heading_to_start},
            {"on_vehicle_heading_to_end", track_on_vehicle_heading_to_end},
            {"on_vehicle_stopped", track_on_vehicle_stopped},
            {"on_isolated_occupied", track_on_isolated_occupied},
            {"on_isolated_freed", track_on_isolated_freed},
            {"on_switch_changed", track_on_switch_changed},
            {nullptr, nullptr},
    };
} // namespace godot
