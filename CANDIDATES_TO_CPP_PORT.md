# GDScript on the hot path - candidates for a port to C++

GDScript that runs on every simulation step or every frame for many objects at once, measured on
the heaviest scenery we have. Ordered by what it costs. Ported pieces go to C++ in the layer that
owns them (libmaszyna for the original's logic, libreloaded for the core's), as `MaszynaMover*`
and the servers are now.

## How it was measured (2026-10-24, MaSzyna-Reloaded/reports#49)

* Wrzosy (`wrzosy_eie2620.scn`), the player in EP09-028 standing; about 350 vehicles moving
  (AI trains and road cars), about 264 AI drivers.
* `make compile-profiling` (RelWithDebInfo, `template_debug`), the packaged `godot-double`,
  `gamescope --backend headless -W 1920 -H 1080 -r 240`, a laptop on AC in its power-saver
  profile - absolute times are slow; the shares are what counts.
* `perf record -e cycles:u -F 499 --call-graph dwarf` (the game launched under `perf` with
  `-D -1 --control fifo:...`: `-p` cannot attach without ptrace rights) for the native side.
  GDScript shows in `perf` only as the interpreter, so the AI driver was split with a temporary
  probe: `Time.get_ticks_usec()` written into a preallocated `PackedInt64Array` at the boundaries
  of `MaszynaAIDriver._update()`, one probe call per update. A probe that calls an autoload per
  mark costs ~20 us a call in a debug build and measures itself, not the driver.

Main thread after the singleton and RID-container fixes of that day: `SimulationServer` 69% -
the vehicles' step (C++) 33%, the AI drivers (GDScript) 32%, the rest of the step's listeners ~4%.

## 1. The AI driver - `addons/libmaszyna/ai_driver/` (7 587 lines)

`DriverServer` calls `MaszynaAIDriver._update()` about 1 550 times a second (each driver at its
reaction time). 78% of that time is the GDScript interpreter itself; the native calls it makes
(`vehicle_trace_route` 5%, `vehicle_is_control_active` 4%, `vehicle_component_get` 3%) are the
rest. Its sections, per second of the run (445 ms of AI per second, all of `_update()`):

| Section of `_update()` | ms/s | Share | Code |
|---|---|---|---|
| `braking.control()` | 73-79 | 17% | `maszyna_driver_braking.gd` |
| `route.update()` | 70-74 | 16% | `maszyna_driver_route.gd` |
| `traction.control()` | 68-69 | 15% | `maszyna_driver_traction*.gd` |
| the route behind (`check_route_behind()`) | 43-49 | 10% | `maszyna_ai_driver.gd` |
| `trainset.update()` | 35-37 | 8% | `maszyna_driver_trainset.gd` |
| time controllers and the doors loop | 24-31 | 6% | `maszyna_ai_driver.gd` |
| `_handle_engine()` | 21-32 | 5% | `maszyna_ai_driver.gd` |
| `speed.pick()` | 15-17 | 4% | `maszyna_driver_speed.gd` |
| hints, the brakes' read, the situation, `set_time_controllers()`, the cab, the security system, the main pipe, `prepare()`, orders | ~95 | 21% | |

Port order by gain: braking, route (with the route behind), traction, trainset. `maszyna_driver_hints.gd`
(1 390 lines) is called from all of them (`cue()`); it moves with them or stays a thin GDScript
layer only for what a player sees. The driver's per-vehicle state (`AIDriverState`) becomes a C++
structure of `DriverServer`'s implementation.

## 2. The cab logic of every vehicle - `CabinSystem._on_simulation_advanced()`

libreloaded `addons/libreloaded/cabin/cabin_system.gd:401` calls, on every simulation advance and
for every cab of the scenery (hundreds - every AI driver sits in one), its process callables:
`addons/libmaszyna/cabin/main_switch.gd`, `door_permits.gd`, and for the shown cabs
`cab_lights.gd`; `cabin_logic.gd:314` adds its held knobs. `Callable.call()` per process per cab per
step. Measured (a timing wrapper around `_on_simulation_advanced()`, the same run): **~2.3 ms a
frame** - 309 cabs, 618 processes (`main_switch.gd`, `door_permits.gd` of every cab) called on every
advance, and the 309 cabs with display processes looked up to find the few shown. At 27 fps that is
about 6% of the main thread.

Not a port to C++: the cost is the cab logic instantiated per vehicle and stepped for every cab.
The plan is one instance per behaviour, stepped only for the cabs where something runs -
`TODO_OPTIMIZE_VEHICLES_PROCESSING.md`, section 3.

## Not candidates

Per-frame scripts that only act on a dirty flag, run in the editor only, or serve one object:
`environment_node.gd`, `sky_environment.gd`, `player.gd`, `follow_view.gd`, `free_look_view.gd`,
the cab's own nodes (`cabin_*_3d.gd`, `cabin_windscreen_wipers.gd`),
`maszyna_rail_vehicle_3d.gd`, `maszyna_rail_vehicle_physics_node.gd`,
`maszyna_e3d_model_instance.gd`, `scenery_content_node.gd`, `track_3d.gd`, `traction_3d.gd`,
`train_set_3d.gd`, `maszyna_vehicle_system.gd` (its frame hook builds vehicles within a budget and
disconnects when done), the debug HUD and the minimap.
