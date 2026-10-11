#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "LuaVariant.hpp"
#include "driver/DriverServer.hpp"

namespace godot {
    /// send_command(v, command, value1, value2) - an order to the vehicle's driver, as a
    /// `putvalues` event gives it ("SetVelocity", 40, 40); false when nobody drives the vehicle
    static int driver_send_command(lua_State *p_state) {
        const RID vehicle = LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
        const String command = String::utf8(luaL_checkstring(p_state, 2));
        const double value1 = luaL_optnumber(p_state, 3, 0.0);
        const double value2 = luaL_optnumber(p_state, 4, 0.0);
        DriverServer *drivers = LuaModules::server<DriverServer>(p_state);
        const RID driver = drivers->vehicle_get_driver(vehicle);
        if (driver.is_valid()) {
            drivers->driver_send_command(driver, command, value1, value2);
        }
        lua_pushboolean(p_state, static_cast<int>(driver.is_valid()));
        return 1;
    }

    /// timetable(v) - the driver's timetable and how far it got, nil when nobody drives
    static int driver_timetable(lua_State *p_state) {
        const RID vehicle = LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
        const DriverServer *drivers = LuaModules::server<DriverServer>(p_state);
        const RID driver = drivers->vehicle_get_driver(vehicle);
        if (!driver.is_valid()) {
            lua_pushnil(p_state);
            return 1;
        }
        LuaVariant::push(p_state, drivers->driver_get_timetable_state(driver));
        return 1;
    }

    const luaL_Reg LuaModules::DRIVER[] = {
            {"send_command", driver_send_command},
            {"timetable", driver_timetable},
            {nullptr, nullptr},
    };
} // namespace godot
