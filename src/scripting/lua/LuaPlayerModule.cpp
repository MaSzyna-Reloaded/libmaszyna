#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "player/PlayerServer.hpp"

namespace godot {
    /// vehicle() - what the player drives, nil for none
    static int player_vehicle(lua_State *p_state) {
        LuaHandle::push(
                p_state, LuaModules::server<PlayerServer>(p_state)->player_get_vehicle(), ScriptHandleKind::VEHICLE);
        return 1;
    }

    /// take_over(v) - the player takes the vehicle over, into its cab
    static int player_take_over(lua_State *p_state) {
        const RID vehicle = LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
        LuaModules::server<PlayerServer>(p_state)->player_take_over_vehicle(vehicle);
        return 0;
    }

    /// enter(v) - the player sits in the vehicle's cab, its driver drives on
    static int player_enter(lua_State *p_state) {
        const RID vehicle = LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
        LuaModules::server<PlayerServer>(p_state)->player_enter_vehicle(vehicle);
        return 0;
    }

    /// leave() - the player lets the trainset go, to its drivers
    static int player_leave(lua_State *p_state) {
        LuaModules::server<PlayerServer>(p_state)->player_leave_vehicle();
        return 0;
    }

    const luaL_Reg LuaModules::PLAYER[] = {
            {"vehicle", player_vehicle}, {"take_over", player_take_over},
            {"enter", player_enter},     {"leave", player_leave},
            {nullptr, nullptr},
    };
} // namespace godot
