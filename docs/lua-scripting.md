---
layout: page
title: "Lua: MaSzyna Modules"
---

This page describes what libmaszyna adds to Lua scripting for MaSzyna's sceneries. The scripting
itself is part of the core: `ScenarioScriptServer`, the `reloaded.*` modules, how scripts work, the
sandbox and the API reference. These are described in libreloaded's
[Lua scenario scripts](https://maszyna-reloaded.github.io/libreloaded/lua-scripting.html).

* [Running a script](#running-a-script)
* [`reloaded.memory` - memory cells](#reloadedmemory---memory-cells)
* [The original `eu07.events`](#the-original-eu07events)
* [Examples](#examples)

## Running a script

A `lua` line in a `.scn` file names a script. The path is relative to the `scenery` directory, as
in the original engine:

```
lua edobre_2/switches.lua
```

The script runs after the scenery is built, when its tracks, events, vehicles and drivers already
exist. A scenery can have several `lua` lines. They run in the order they appear in the file.

libmaszyna's Lua modules are compiled only when the core is built with Lua (`LIBRELOADED_LUA`, on
by default). Without Lua, every script fails with "This build has no Lua runtime: scenario scripts
do not run."

## `reloaded.memory` - memory cells

This module gives access to the scenery's memory cells by name. It is available in every script as
`reloaded.memory` and through `require`. It is implemented by `MaszynaLuaMemoryModule`.

| Function | Returns / does |
|---|---|
| `find(name)` | the memory cell of the name |
| `read(m)` | `text, value1, value2` |
| `write(m, text, value1, value2)` | |
| `on_values_changed(m, fn)` | `fn()` whenever its values change |

## The original `eu07.events`

Scripts written for the original engine run without changes. The module `eu07.events` (also
available as `require("eu07.events")`) has the original's 22 functions:

`event_create`, `event_find`, `event_exists`, `event_getname`, `event_dispatch`,
`event_dispatch_n`, `track_find`, `track_isoccupied`, `track_isoccupied_n`, `isolated_find`,
`isolated_isoccupied`, `isolated_isoccupied_n`, `train_getname`, `dynobj_putvalues`,
`memcell_find`, `memcell_read`, `memcell_read_n`, `memcell_update`, `memcell_update_n`, `random`,
`writelog`, `writeerrorlog`.

A memory cell's values are a table `{str =, num1 =, num2 =}`, as in the original. An event whose
name contains `onstart` is queued as soon as it is created.

One difference: `dynobj_putvalues` does nothing on a vehicle that nobody drives. The original passes
the command to the vehicle itself.

The module is implemented by `MaszynaLuaEventsModule` on top of the core's modules and
`reloaded.memory`.

## Examples

### Elektrociepłownia Dobre 2 - switches that throw themselves

In `elektrocieplownia_dobre_2.scn` the SM42-982 starts on track `tor_53`. The nearest switches are
`test_zwr12` and `test_zwr07`. This script throws both switches every second of simulation time:

```lua
local track, sim, log = reloaded.track, reloaded.sim, reloaded.log

-- the switches nearest to the SM42's start track (tor_53)
local switches = {}
for _, name in ipairs({ "test_zwr12", "test_zwr07" }) do
    local found = track.find_switch(name)
    if found then
        table.insert(switches, found)
    else
        log.warning("no switch " .. name)
    end
end

sim.every(1.0, function()
    for _, switch in ipairs(switches) do
        local target = track.switch_get(switch) == "common" and "diverging" or "common"
        track.switch_set(switch, target)
    end
end)
```

### td.scn - a dispatcher at the level crossings

In `td.scn` the two level crossings are on the isolated sections `t1` and `t2`. The scenery's own
event `keyctrl00` (Shift+0) sets the signals of station 1. This script logs every train at the
crossings: its speed at the first crossing and the time it needed to clear the second. After the
train has passed, it sets the station's signals to stop:

```lua
local event, track, vehicle, sim, log =
    reloaded.event, reloaded.track, reloaded.vehicle, reloaded.sim, reloaded.log

local crossing_1 = track.isolated_find("t1")
local crossing_2 = track.isolated_find("t2")
-- the scenery's own event: stacja1_a_s1, stacja1_j-s1, stacja1_k-s1
local signals_to_stop = event.find("keyctrl00")

-- keyed by vehicle id, because a handle cannot be a table key
local entered_at = {}

track.on_isolated_occupied(crossing_1, function(v)
    entered_at[vehicle.id(v)] = sim.time()
    log.info(string.format("%s enters crossing 1 at %.0f km/h", vehicle.name(v), vehicle.speed(v)))
end)

track.on_isolated_freed(crossing_2, function(v)
    local started = entered_at[vehicle.id(v)]
    if started then
        log.info(string.format("%s cleared both crossings in %.0f s", vehicle.name(v), sim.time() - started))
        entered_at[vehicle.id(v)] = nil
    end
    event.queue(signals_to_stop)
end)
```

Both scripts are run from a `lua` line of the scenery.
