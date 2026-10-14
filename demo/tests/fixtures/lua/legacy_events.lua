-- the original's API (lua.cpp:17-42), as a scenery script of the original uses it
local events = require("eu07.events")
assert(events == eu07.events)

events.event_create("lua_test_legacy_onstart", 0, 0, function(event, activator)
    local values = events.memcell_read_n("lua_test_output")
    events.memcell_update_n("lua_test_output", { str = "legacy:" .. events.event_getname(event), num1 = values.num1 + 1 })
    events.writelog("legacy event ran")
end)
