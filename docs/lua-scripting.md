---
layout: page
title: "Lua Scenario Scripts"
---

A scenery can bring its own logic written in Lua: a dispatcher that sets the signals, a switch that
throws itself, a train that is given its orders when it stops. The scripts run in the simulation,
through the same servers the rest of the game uses - they are not a mod of the engine, and they
cannot reach past the scenario.

* [Running a script](#running-a-script)
* [How a script works](#how-a-script-works)
* [API reference](#api-reference)
* [The original's `eu07.events`](#the-originals-eu07events)
* [Examples](#examples)

## Running a script

**From the scenery.** A `lua` line in the `.scn` names a file relative to the `scenery` directory,
as in the original engine:

```
lua edobre_2/switches.lua
```

The script runs once the scenery is built - its tracks, events, vehicles and drivers all exist by
then. A scenery may have several `lua` lines; they run in the order of the file.

**While playing.** *View > Lua scripts* opens an editor for the scenario being played:

* **Check** compiles the code without running it and shows the first error with its line
  (`editor:2: unexpected symbol near <eof>`).
* **Apply** runs the code. Applying again first takes back everything the previous Apply made -
  its events, timers and subscriptions - so the code can be changed and applied at will. Applying
  an empty editor stops whatever it started.

Errors of a running script (a callback that fails later) are shown under the editor and written to
the game log.

**In the build.** Lua 5.4 is the `vendor/lua` submodule, compiled in by the CMake option
`LIBMASZYNA_LUA` (on by default). A build without it refuses every script with
"lua scripts not supported in this build.", as the original does.

## How a script works

**Handles.** Everything a script works with - a vehicle, a track, an event, a memory cell, a signal
head - is a handle of its kind, found by name (`maszyna.vehicle.find("SM42-982")`). A lookup that
finds nothing returns `nil`. A function taking a vehicle refuses a track. Two handles of the same
object are equal (`==`), but they are not the same Lua value: do not use a handle as a table key -
use its name.

**Callbacks.** A function given to `on_*`, `event.create`, `sim.after` or `sim.every` never runs
inside the server that reported something. It runs as a scenario event, on the simulation clock:
right after the report, in the order the reports came, and not at all while the simulation is
paused. Time in the API is simulation time - faster simulation, faster timers.

**What a script owns.** The events, timers and subscriptions a script made are freed with the
scenario, or when the editor applies its code again. A script frees an event of its own with
`maszyna.event.free()` and stops a timer or a subscription with `maszyna.cancel()`.

**The sandbox.** A script has the `base`, `string`, `table`, `math`, `coroutine` and `utf8`
libraries - no `io`, `os`, `debug` or `package`, no `dofile`/`loadfile`, and `load` of text only.
`require("lib.util")` loads `lib/util.lua` of the scenery directory, and nothing outside it.
`print` writes to the game log. A call that runs too long (a loop that never ends) and a script
that takes too much memory fail with an error instead of stopping the game.

## API reference

Every module is a table of `maszyna` (`maszyna.vehicle`, ...), also available through `require`.
The command and state names are the vehicle's own - the ones the console and the cab use.

### `maszyna.sim` - time

| Function | Returns / does |
|---|---|
| `time()` | seconds of simulation time since the scenario started |
| `time_of_day()` | hours since midnight (13.5 at half past one) |
| `is_paused()` | whether the simulation is paused |
| `random(a, b)` | a number between `a` and `b` |
| `after(seconds, fn)` | runs `fn()` once, after the seconds; returns a subscription |
| `every(seconds, fn)` | runs `fn()` every so many seconds; returns a subscription |

`maszyna.cancel(subscription)` stops a timer or a subscription.

### `maszyna.vehicle` - vehicles

| Function | Returns / does |
|---|---|
| `find(name)` | the vehicle of the name; of several, one |
| `find_all(name)` | every vehicle of the name |
| `all()` | every vehicle |
| `name(v)`, `speed(v)`, `velocity(v)` | its name; km/h, never negative; km/h, negative backwards |
| `state(v, key)`, `config(v, key)` | one value of its state or configuration (`"brake/cylinder_pressure"`) |
| `commands(v)` | the commands it takes |
| `send_command(v, command, p1, p2)` | sends a command, returns the answer (`send_command(v, "battery", true)`) |
| `coupled(v, end, element)` | the vehicles joined by `"coupler"`, `"brake_hose"`, `"main_hose"`, `"control"`, `"gangway"`, `"heating"` or `"permanent"` |
| `track_position(v)` | `{track = <track>, along = <metres>}` |
| `on_command_received(v, fn)` | `fn(command, p1, p2)` for every command the vehicle gets |

### `maszyna.cabin` - the cab, as the driver's hand

| Function | Returns / does |
|---|---|
| `act(cabin, control_id, action, value)` | manipulates a control; `action` is `"increase"`, `"decrease"`, `"hold"`, `"release"`, `"toggle"` or `"set"` |
| `control(cabin, control_id)` | where the control stands |
| `controls(cabin)` | the ids of the cabin's controls |
| `driver_cabin(v)` | the cabin the vehicle's driver sits in, `nil` when nobody drives it |
| `front_cabin(v)`, `rear_cabin(v)`, `machine_room(v)` | the vehicle's cabin of that kind, `nil` when it has none |
| `on_control_changed(v, fn)` | `fn(cabin, control_id, value)` when a control of the vehicle's cabins changes |

### `maszyna.driver` - the vehicle's driver

| Function | Returns / does |
|---|---|
| `send_command(v, command, value1, value2)` | an order to the driver, as a `putvalues` event gives it (`"SetVelocity", 40, 40`); `false` when nobody drives |
| `timetable(v)` | the driver's timetable and how far it got; `nil` when nobody drives |

### `maszyna.event` - scenario events

| Function | Returns / does |
|---|---|
| `create{name =, delay =, random_delay =, run =, run_else =}` | a new event; `run(event, activator)` when it runs, `run_else` when its condition failed. The scenery's events may queue it by name |
| `find(name)`, `exists(name)` | the event of the name - the scenery's or a script's |
| `name(e)`, `is_queued(e)` | |
| `queue(e, activator, extra_delay)` | runs it after its delay; `false` when it waits already |
| `free(e)` | frees an event the script created |
| `on_launched(e, fn)` | `fn(activator)` whenever the event runs |

### `maszyna.memory` - memory cells

| Function | Returns / does |
|---|---|
| `find(name)` | the memory cell of the name |
| `read(m)` | `text, value1, value2` |
| `write(m, text, value1, value2)` | |
| `on_values_changed(m, fn)` | `fn()` whenever its values change |

### `maszyna.track` - tracks, switches, isolated sections

| Function | Returns / does |
|---|---|
| `find(name)` | the track (or switch) of the name |
| `is_occupied(t)`, `vehicles(t)` | |
| `switch_get(t)`, `switch_set(t, which)` | `"common"` or `"diverging"` |
| `isolated_find(name)`, `isolated_is_occupied(i)` | |
| `on_vehicle_heading_to_start(t, fn)`, `on_vehicle_heading_to_end(t, fn)` | `fn(vehicle)` when a vehicle on the track moves towards its start / end |
| `on_vehicle_stopped(t, fn)` | `fn(vehicle)` when a vehicle stops on the track |
| `on_isolated_occupied(i, fn)`, `on_isolated_freed(i, fn)` | `fn(vehicle)` when the first vehicle comes onto the section / the last one leaves |
| `on_switch_changed(t, fn)` | `fn(which)` when the switch moves |

### `maszyna.signal` - signals

A signalling system decides what its signal heads show, so a script asks the system rather than
lighting a head.

| Function | Returns / does |
|---|---|
| `find_head(name)`, `aspect(h)`, `aspects(h)` | a signal head, the aspect it shows, the ones it can show |
| `find_system(name)` | a signalling system (the scenery's is named after its file) |
| `send_event(system, event, arguments)` | e.g. `send_event(system, "lights", {signal_head = h, aspect = "S1"})` |
| `on_aspect_changed(h, fn)` | `fn(aspect)` whenever the head shows another aspect |

### `maszyna.player` - what the player drives

| Function | Returns / does |
|---|---|
| `vehicle()` | the vehicle the player drives; `nil` for none |
| `take_over(v)` | the player takes the vehicle over and sits in its cab; its driver only takes orders meanwhile |
| `enter(v)` | the player sits in the vehicle's cab; its driver, if it has one, drives on |
| `leave()` | the player lets the trainset go; its drivers drive it again |

### `maszyna.camera` - where the player looks from

| Function | Returns / does |
|---|---|
| `mode()`, `set_mode(mode)` | `"cabin"` (only with a vehicle driven), `"free"` or `"follow"` (only with a target) |
| `target()`, `set_target(v)` | the vehicle the following camera follows |
| `follow_view()`, `set_follow_view(view)` | `"trainset_front"`, `"trainset_rear"`, `"bogie"` or `"driveby"` |
| `cycle_follow_view()` | Shift+F4: following, the next view; else the player's vehicle followed |
| `toggle_cabin()` | F4: from outside (following or walking) back into the cab of the vehicle driven - following with none, the free camera; in the cab, out of it |
| `show_vehicle(v)` | the free camera beside the vehicle, looking at it |

Looking from outside is only a view: the player keeps driving the vehicle and its controls work.

### `maszyna.hud` - what the HUD shows

A panel is named by the HUD that shows it; the demo's are `transcripts`, `driving_aid`,
`timetable`, `scenario`, `controls` (every control window at once), `scripts`, `trainsets`, and
one per control window (`general`, `engine`, `brakes`, ..., `mini_map`, `weather_and_time`, `help`).

| Function | Returns / does |
|---|---|
| `show(panel)`, `hide(panel)`, `toggle(panel)`, `is_visible(panel)` | |
| `open_card(v)`, `close_card()`, `card()` | the vehicle card; `card()` is `nil` while it is closed |

### `maszyna.log`

`debug(text)`, `info(text)`, `warning(text)`, `error(text)` - to the game log.

## The original's `eu07.events`

Scripts written for the original engine run unchanged: `eu07.events` (also
`require("eu07.events")`) has the original's 22 functions, built on the modules above -
`event_create`, `event_find`, `event_exists`, `event_getname`, `event_dispatch`,
`event_dispatch_n`, `track_find`, `track_isoccupied`, `track_isoccupied_n`, `isolated_find`,
`isolated_isoccupied`, `isolated_isoccupied_n`, `train_getname`, `dynobj_putvalues`,
`memcell_find`, `memcell_read`, `memcell_read_n`, `memcell_update`, `memcell_update_n`, `random`,
`writelog`, `writeerrorlog`. A memory cell's values are the original's table
`{str =, num1 =, num2 =}`, and an event whose name holds `onstart` is queued as it is created.

One difference: `dynobj_putvalues` on a vehicle nobody drives does nothing - the original hands the
command to the vehicle itself.

## Examples

### Elektrociepłownia Dobre 2 - switches that throw themselves

`elektrocieplownia_dobre_2.scn` starts the SM42-982 on `tor_53`; the nearest switches are
`test_zwr12` and `test_zwr07`. Every second of simulation time both go to the other track:

```lua
local track, sim, log = maszyna.track, maszyna.sim, maszyna.log

-- the switches nearest to the SM42's start track (tor_53)
local switches = {}
for _, name in ipairs({ "test_zwr12", "test_zwr07" }) do
    local found = track.find(name)
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

In `td.scn` the two level crossings stand on the isolated sections `t1` and `t2`, and the scenery's
own `keyctrl00` (Shift+0) sets the signals of station 1 to S1. The script reports every train
crossing the road - its speed at the first crossing, how long it took to clear the second - and
sets the station's signals to stop behind it:

```lua
local event, track, vehicle, sim, log =
    maszyna.event, maszyna.track, maszyna.vehicle, maszyna.sim, maszyna.log

local crossing_1 = track.isolated_find("t1")
local crossing_2 = track.isolated_find("t2")
-- the scenery's own event: stacja1_a_s1, stacja1_j-s1, stacja1_k-s1
local signals_to_stop = event.find("keyctrl00")

-- by name: a handle is no table key
local entered_at = {}

track.on_isolated_occupied(crossing_1, function(v)
    entered_at[vehicle.name(v)] = sim.time()
    log.info(string.format("%s enters crossing 1 at %.0f km/h", vehicle.name(v), vehicle.speed(v)))
end)

track.on_isolated_freed(crossing_2, function(v)
    local name = vehicle.name(v)
    local started = entered_at[name]
    if started then
        log.info(string.format("%s cleared both crossings in %.0f s", name, sim.time() - started))
        entered_at[name] = nil
    end
    event.queue(signals_to_stop)
end)
```

Both run as they are from *View > Lua scripts*, or from a file named by a `lua` line of the
scenery.
