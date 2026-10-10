#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "hud/HUDServer.hpp"

namespace godot {
    /// The panel's name, the first argument
    static StringName check_panel(lua_State *p_state) {
        return String::utf8(luaL_checkstring(p_state, 1));
    }

    /// show(panel) - the panel open ("timetable", "driving_aid", ...)
    static int hud_show(lua_State *p_state) {
        LuaModules::server<HUDServer>(p_state)->panel_set_visible(check_panel(p_state), true);
        return 0;
    }

    /// hide(panel)
    static int hud_hide(lua_State *p_state) {
        LuaModules::server<HUDServer>(p_state)->panel_set_visible(check_panel(p_state), false);
        return 0;
    }

    /// toggle(panel)
    static int hud_toggle(lua_State *p_state) {
        LuaModules::server<HUDServer>(p_state)->panel_toggle(check_panel(p_state));
        return 0;
    }

    /// is_visible(panel) - whether the panel is open
    static int hud_is_visible(lua_State *p_state) {
        lua_pushboolean(
                p_state,
                static_cast<int>(LuaModules::server<HUDServer>(p_state)->panel_is_visible(check_panel(p_state))));
        return 1;
    }

    /// open_card(v) - the vehicle's card
    static int hud_open_card(lua_State *p_state) {
        const RID vehicle = LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
        LuaModules::server<HUDServer>(p_state)->card_open(vehicle);
        return 0;
    }

    /// close_card()
    static int hud_close_card(lua_State *p_state) {
        LuaModules::server<HUDServer>(p_state)->card_close();
        return 0;
    }

    /// card() - the vehicle whose card is open, nil for none
    static int hud_card(lua_State *p_state) {
        LuaHandle::push(p_state, LuaModules::server<HUDServer>(p_state)->card_get_vehicle(), ScriptHandleKind::VEHICLE);
        return 1;
    }

    const luaL_Reg LuaModules::HUD[] = {
            {"show", hud_show},           {"hide", hud_hide},
            {"toggle", hud_toggle},       {"is_visible", hud_is_visible},
            {"open_card", hud_open_card}, {"close_card", hud_close_card},
            {"card", hud_card},           {nullptr, nullptr},
    };
} // namespace godot
