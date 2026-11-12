#include "MaszynaLuaEventsModule.hpp"
#include "lauxlib.h"
#include "maszyna/scenario/MaszynaScenarioEventServer.hpp"
#include "scripting/lua/LuaHandle.hpp"
#include "scripting/lua/LuaModules.hpp"
#include <cstring>

namespace godot {
    namespace {
        /// lua.cpp:125-372, function by function
        constexpr const char *SOURCE = R"lua(
local event, memory, track, vehicle, driver, sim, log =
    reloaded.event, reloaded.memory, reloaded.track, reloaded.vehicle, reloaded.driver, reloaded.sim, reloaded.log
-- the order put to a vehicle nobody drives (MaszynaScenarioEventServer.vehicle_put_command()),
-- handed in by the module's opener
local put_command = ...

local events = {}

-- lua.cpp:87-123: a memory cell's values as a table, each field optional
local function values_of(values)
    return values.str or "", values.num1 or 0, values.num2 or 0
end

local function missing(kind, name)
    log.error("lua: missing " .. kind .. ": " .. name)
end

-- lua.cpp:125-143: fn(event, activator) runs when the event does. The original queues an event
-- whose name holds "onstart" as it is inserted (Event.cpp:2340-2344)
function events.event_create(name, delay, randomdelay, fn)
    local created = event.create{ name = name, delay = delay, random_delay = randomdelay, run = fn }
    if string.find(name, "onstart", 1, true) then
        event.queue(created)
    end
    return created
end

function events.event_find(name)
    local found = event.find(name)
    if not found then
        missing("event", name)
    end
    return found
end

function events.event_exists(name)
    return event.exists(name)
end

function events.event_getname(e)
    return event.name(e)
end

function events.event_dispatch(e, activator, delay)
    event.queue(e, activator, delay)
end

function events.event_dispatch_n(name, activator, delay)
    local found = events.event_find(name)
    if found then
        event.queue(found, activator, delay)
    end
end

function events.track_find(name)
    local found = track.find(name)
    if not found then
        missing("track", name)
    end
    return found
end

function events.track_isoccupied(t)
    return track.is_occupied(t)
end

function events.track_isoccupied_n(name)
    local found = events.track_find(name)
    return found ~= nil and track.is_occupied(found)
end

function events.isolated_find(name)
    local found = track.isolated_find(name)
    if not found then
        missing("isolated", name)
    end
    return found
end

function events.isolated_isoccupied(i)
    return track.isolated_is_occupied(i)
end

function events.isolated_isoccupied_n(name)
    local found = events.isolated_find(name)
    return found ~= nil and track.isolated_is_occupied(found)
end

-- lua.cpp:274-283: the name of the train its driver drives, nothing without a driver
function events.train_getname(dynobj)
    local timetable = driver.timetable(dynobj)
    if timetable and timetable.timetable then
        return timetable.timetable.train_name
    end
end

-- lua.cpp:285-297: what a `putvalues` event does: to the vehicle's driver, and to the vehicle
-- itself when nobody drives it - with no place, as the original's empty TLocation
function events.dynobj_putvalues(dynobj, values)
    local text, value1, value2 = values_of(values)
    if not driver.send_command(dynobj, text, value1, value2) then
        put_command(dynobj, text, value1, value2)
    end
end

function events.memcell_find(name)
    local found = memory.find(name)
    if not found then
        missing("memcell", name)
    end
    return found
end

-- lua.cpp:312-327: nil and zeros for a missing cell
function events.memcell_read(m)
    if not m then
        return { str = nil, num1 = 0, num2 = 0 }
    end
    local text, value1, value2 = memory.read(m)
    return { str = text, num1 = value1, num2 = value2 }
end

function events.memcell_read_n(name)
    return events.memcell_read(events.memcell_find(name))
end

-- lua.cpp:329-350: all three fields are written, missing ones as empty and zero
function events.memcell_update(m, values)
    if m then
        memory.write(m, values_of(values))
    end
end

function events.memcell_update_n(name, values)
    events.memcell_update(events.memcell_find(name), values)
end

function events.random(a, b)
    return sim.random(a, b)
end

function events.writelog(text)
    log.info("lua: log: " .. text)
end

function events.writeerrorlog(text)
    log.error("lua: log: " .. text)
end

eu07 = { events = events }
return events
)lua";

    } // namespace

    /// put_command(v, command, value1, value2) - the order to the vehicle itself, at no place
    /// (TMoverParameters::PutCommand(), lua.cpp:293-294)
    static int put_command(lua_State *p_state) {
        const RID vehicle = LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
        const String command = String::utf8(luaL_checkstring(p_state, 2));
        const double value1 = luaL_optnumber(p_state, 3, 0.0);
        const double value2 = luaL_optnumber(p_state, 4, 0.0);
        LuaModules::server<MaszynaScenarioEventServer>(p_state)->vehicle_put_command(
                vehicle, command, value1, value2, Vector3());
        return 0;
    }

    void MaszynaLuaEventsModule::open(lua_State *p_state) {
        if (!(luaL_loadbufferx(p_state, SOURCE, std::strlen(SOURCE), "=eu07.events", "t") == LUA_OK)) {
            lua_error(p_state);
        }
        lua_pushcfunction(p_state, &put_command);
        lua_call(p_state, 1, 1);
        luaL_getsubtable(p_state, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
        lua_insert(p_state, -2);
        lua_setfield(p_state, -2, MODULE_NAME);
        lua_pop(p_state, 1);
    }
} // namespace godot
