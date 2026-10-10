#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "player/PlayerCameraServer.hpp"
#include <array>

namespace godot {
    namespace {
        /// The modes and the follow views by their names in scripts, in the order of the enums
        constexpr std::array MODE_NAMES{"cabin", "free", "follow"};
        constexpr std::array FOLLOW_VIEW_NAMES{"trainset_front", "trainset_rear", "bogie", "driveby"};
        static_assert(FOLLOW_VIEW_NAMES.size() == PlayerCameraServer::CAMERA_FOLLOW_VIEW_MAX);
    } // namespace

    template<size_t N>
    static int check_name(lua_State *p_state, const int p_index, const std::array<const char *, N> &p_names) {
        // luaL_checkoption wants a null-terminated list of the names
        std::array<const char *, N + 1> names{};
        std::copy(p_names.begin(), p_names.end(), names.begin());
        return luaL_checkoption(p_state, p_index, nullptr, names.data());
    }

    /// set_mode("cabin" | "free" | "follow")
    static int camera_set_mode(lua_State *p_state) {
        const int mode = check_name(p_state, 1, MODE_NAMES);
        LuaModules::server<PlayerCameraServer>(p_state)->camera_set_mode(
                static_cast<PlayerCameraServer::CameraMode>(mode));
        return 0;
    }

    /// mode() - "cabin", "free" or "follow"
    static int camera_mode(lua_State *p_state) {
        lua_pushstring(p_state, MODE_NAMES.at(LuaModules::server<PlayerCameraServer>(p_state)->camera_get_mode()));
        return 1;
    }

    /// set_target(v) - the vehicle the following camera follows
    static int camera_set_target(lua_State *p_state) {
        const RID vehicle = LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
        LuaModules::server<PlayerCameraServer>(p_state)->camera_set_target(vehicle);
        return 0;
    }

    /// target() - the vehicle followed, nil for none
    static int camera_target(lua_State *p_state) {
        LuaHandle::push(
                p_state, LuaModules::server<PlayerCameraServer>(p_state)->camera_get_target(),
                ScriptHandleKind::VEHICLE);
        return 1;
    }

    /// set_follow_view("trainset_front" | "trainset_rear" | "bogie" | "driveby")
    static int camera_set_follow_view(lua_State *p_state) {
        const int view = check_name(p_state, 1, FOLLOW_VIEW_NAMES);
        LuaModules::server<PlayerCameraServer>(p_state)->camera_set_follow_view(
                static_cast<PlayerCameraServer::CameraFollowView>(view));
        return 0;
    }

    /// follow_view() - the view of the following camera, by name
    static int camera_follow_view(lua_State *p_state) {
        lua_pushstring(
                p_state,
                FOLLOW_VIEW_NAMES.at(LuaModules::server<PlayerCameraServer>(p_state)->camera_get_follow_view()));
        return 1;
    }

    /// cycle_follow_view() - Shift+F4
    static int camera_cycle_follow_view(lua_State *p_state) {
        LuaModules::server<PlayerCameraServer>(p_state)->camera_cycle_follow_view();
        return 0;
    }

    /// toggle_cabin() - F4
    static int camera_toggle_cabin(lua_State *p_state) {
        LuaModules::server<PlayerCameraServer>(p_state)->camera_toggle_cabin();
        return 0;
    }

    /// show_vehicle(v) - the free camera beside the vehicle, looking at it
    static int camera_show_vehicle(lua_State *p_state) {
        const RID vehicle = LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
        LuaModules::server<PlayerCameraServer>(p_state)->camera_show_vehicle(vehicle);
        return 0;
    }

    const luaL_Reg LuaModules::CAMERA[] = {
            {"set_mode", camera_set_mode},
            {"mode", camera_mode},
            {"set_target", camera_set_target},
            {"target", camera_target},
            {"set_follow_view", camera_set_follow_view},
            {"follow_view", camera_follow_view},
            {"cycle_follow_view", camera_cycle_follow_view},
            {"toggle_cabin", camera_toggle_cabin},
            {"show_vehicle", camera_show_vehicle},
            {nullptr, nullptr},
    };
} // namespace godot
