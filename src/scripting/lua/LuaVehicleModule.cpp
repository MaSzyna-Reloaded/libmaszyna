#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "LuaVariant.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"

namespace godot {
    /// The couplings, as a script names them, and the flag each name stands for
    static constexpr const char *COUPLING_NAMES[] = {
            "coupler", "brake_hose", "main_hose", "control", "gangway", "heating", "permanent", nullptr,
    };
    static constexpr RailVehicleController::CouplingFlags COUPLING_FLAGS[] = {
            RailVehicleController::COUPLING_FLAG_COUPLER,   RailVehicleController::COUPLING_FLAG_BRAKEHOSE,
            RailVehicleController::COUPLING_FLAG_MAINHOSE,  RailVehicleController::COUPLING_FLAG_CONTROL,
            RailVehicleController::COUPLING_FLAG_GANGWAY,   RailVehicleController::COUPLING_FLAG_HEATING,
            RailVehicleController::COUPLING_FLAG_PERMANENT,
    };

    static void push_vehicles(lua_State *p_state, const TypedArray<RID> &p_vehicles) {
        lua_createtable(p_state, static_cast<int>(p_vehicles.size()), 0);
        for (int64_t i = 0; i < p_vehicles.size(); i++) {
            LuaHandle::push(p_state, p_vehicles[i], ScriptHandleKind::VEHICLE);
            lua_rawseti(p_state, -2, i + 1);
        }
    }

    static RID check_vehicle(lua_State *p_state) {
        return LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
    }

    /// find(name) - the vehicle of the name, nil when there is none; of several, one
    static int vehicle_find(lua_State *p_state) {
        const String name = String::utf8(luaL_checkstring(p_state, 1));
        LuaHandle::push(
                p_state, LuaModules::server<VehicleServer>(p_state)->vehicle_get_rid_by_name(name),
                ScriptHandleKind::VEHICLE);
        return 1;
    }

    /// find_all(name) - every vehicle of the name: a scenery may give one name to several
    static int vehicle_find_all(lua_State *p_state) {
        const String name = String::utf8(luaL_checkstring(p_state, 1));
        const VehicleServer *vehicles = LuaModules::server<VehicleServer>(p_state);
        const TypedArray<RID> all = vehicles->vehicle_get_rids();
        TypedArray<RID> named;
        for (int64_t i = 0; i < all.size(); i++) {
            if (vehicles->vehicle_get_name(all[i]) == name) {
                named.push_back(all[i]);
            }
        }
        push_vehicles(p_state, named);
        return 1;
    }

    static int vehicle_all(lua_State *p_state) {
        push_vehicles(p_state, LuaModules::server<VehicleServer>(p_state)->vehicle_get_rids());
        return 1;
    }

    static int vehicle_name(lua_State *p_state) {
        const RID vehicle = check_vehicle(p_state);
        LuaVariant::push(p_state, LuaModules::server<VehicleServer>(p_state)->vehicle_get_name(vehicle));
        return 1;
    }

    /// Speed in km/h, never negative
    static int vehicle_speed(lua_State *p_state) {
        const RID vehicle = check_vehicle(p_state);
        lua_pushnumber(p_state, LuaModules::server<VehicleServer>(p_state)->vehicle_get_speed(vehicle));
        return 1;
    }

    /// Velocity in km/h, negative when moving backwards
    static int vehicle_velocity(lua_State *p_state) {
        const RID vehicle = check_vehicle(p_state);
        lua_pushnumber(p_state, LuaModules::server<VehicleServer>(p_state)->vehicle_get_velocity(vehicle));
        return 1;
    }

    static int vehicle_commands(lua_State *p_state) {
        const RID vehicle = check_vehicle(p_state);
        LuaVariant::push(p_state, LuaModules::server<VehicleServer>(p_state)->vehicle_get_commands(vehicle));
        return 1;
    }

    /// state(v, key) - one value of the vehicle's state, by its key ("brake/cylinder_pressure")
    static int vehicle_state(lua_State *p_state) {
        const RID vehicle = check_vehicle(p_state);
        const String key = String::utf8(luaL_checkstring(p_state, 2));
        LuaVariant::push(
                p_state, LuaModules::server<VehicleServer>(p_state)->vehicle_dump_state(vehicle).get(key, Variant()));
        return 1;
    }

    static int vehicle_config(lua_State *p_state) {
        const RID vehicle = check_vehicle(p_state);
        const String key = String::utf8(luaL_checkstring(p_state, 2));
        LuaVariant::push(
                p_state, LuaModules::server<VehicleServer>(p_state)->vehicle_dump_config(vehicle).get(key, Variant()));
        return 1;
    }

    /// send_command(v, command, p1, p2) - returns what the vehicle answered
    static int vehicle_send_command(lua_State *p_state) {
        const RID vehicle = check_vehicle(p_state);
        const StringName command = String::utf8(luaL_checkstring(p_state, 2));
        const Variant p1 = LuaVariant::to_variant(p_state, 3);
        const Variant p2 = LuaVariant::to_variant(p_state, 4);
        LuaVariant::push(
                p_state, LuaModules::server<VehicleServer>(p_state)->vehicle_send_command(vehicle, command, p1, p2));
        return 1;
    }

    /// coupled(v, end, coupling) - the vehicles joined to this one by the coupling ("coupler",
    /// "brake_hose", ...), from the last one beyond the end (0 front, 1 rear) back through this one
    static int vehicle_coupled(lua_State *p_state) {
        const RID vehicle = check_vehicle(p_state);
        // a script's number, checked before it becomes an end
        const lua_Integer end = luaL_checkinteger(p_state, 2);
        luaL_argcheck(
                p_state,
                end == RailVehicleController::COUPLER_END_FRONT || end == RailVehicleController::COUPLER_END_REAR, 2,
                "end must be 0 (front) or 1 (rear)");
        const RailVehicleController::CouplingFlags flag =
                COUPLING_FLAGS[luaL_checkoption(p_state, 3, nullptr, COUPLING_NAMES)];
        push_vehicles(
                p_state, LuaModules::server<RailVehicleServer>(p_state)->vehicle_get_coupled(
                                 vehicle, static_cast<RailVehicleController::CouplerEnd>(end), flag));
        return 1;
    }

    /// track_position(v) - {track = the track it stands on, along = metres along it}
    static int vehicle_track_position(lua_State *p_state) {
        const RID vehicle = check_vehicle(p_state);
        const Dictionary position = LuaModules::server<RailVehicleServer>(p_state)->vehicle_get_track_position(vehicle);
        lua_createtable(p_state, 0, 2);
        LuaHandle::push(p_state, position["track_rid"], ScriptHandleKind::TRACK);
        lua_setfield(p_state, -2, "track");
        lua_pushnumber(p_state, position["along"]);
        lua_setfield(p_state, -2, "along");
        return 1;
    }

    /// on_command_received(v, fn) - fn(command, p1, p2) for every command the vehicle gets
    static int vehicle_on_command_received(lua_State *p_state) {
        return LuaModules::subscribe(
                p_state, ScenarioScriptServer::SIGNAL_VEHICLE_COMMAND_RECEIVED, ScriptHandleKind::VEHICLE);
    }

    const luaL_Reg LuaModules::VEHICLE[] = {
            {"find", vehicle_find},
            {"find_all", vehicle_find_all},
            {"all", vehicle_all},
            {"name", vehicle_name},
            {"speed", vehicle_speed},
            {"velocity", vehicle_velocity},
            {"commands", vehicle_commands},
            {"state", vehicle_state},
            {"config", vehicle_config},
            {"send_command", vehicle_send_command},
            {"coupled", vehicle_coupled},
            {"track_position", vehicle_track_position},
            {"on_command_received", vehicle_on_command_received},
            {nullptr, nullptr},
    };
} // namespace godot
