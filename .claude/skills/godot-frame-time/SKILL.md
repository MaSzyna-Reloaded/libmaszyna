---
name: godot-frame-time
description: Diagnose and fix frame-rate, stutter, hitching and long-load problems in this Godot MaSzyna wrapper - low fps in a scenery, juddering vehicles, a frame time that no profiler seems to account for, a loading screen that takes minutes, or the world popping in while you drive. Use it whenever the question is "why is this slow" or "where does the frame time go" in demo/ or addons/libmaszyna, before proposing any optimization, because the instruments in this project report narrower things than their labels suggest and guessing from node counts reliably points at the wrong subsystem.
---

# Where the frame time goes

A real scenery here is enormous - `baltyk_skm1.scn` with every include expanded
places 90 228 `triangles` nodes, 8 487 models, 2 340 tracks, 1 163 traction
spans and 376 vehicles - so almost any subsystem *looks* like a plausible
culprit. It usually isn't the one you'd guess. Measure, form **one** hypothesis
per measurement, and check it before forming the next. Three wrong hypotheses in
a row cost a real operator three game restarts and a lot of patience.

## Profile an optimized build

Before reading a single number, check what the project is running. `make compile-debug` builds
the extension - including the vendored `Mover.cpp` physics - at `-O0`, and that is the library the
editor loads. Physics code there ran several times slower than in a shipped build; on one scenery
simply rebuilding with optimizations took it from 31 to 44 fps, more than every code change of a
long session put together, and until then the profile pointed at the simulation as if the code
were at fault.

`make compile-profiling` builds the same `template_debug` library with `RelWithDebInfo`, so the
editor still loads it. Go back with `make compile-debug`. Comparing against the original engine is
only meaningful this way - that one is a release build.

## Measure a real scenery without a window

No probe is needed: the game takes the original's command line, so
`godot-double --path demo --audio-driver Dummy -- -s <scenery>.scn` loads the scenery from the
game directory and puts the player in its first trainset (`-v <train_id>` for another one).

* Run it under `gamescope --backend headless -W 1920 -H 1080 -- ...` with `WAYLAND_DISPLAY`
  unset. That gives a real GPU and no window. Plain `--headless` crashes a big scenery on the dummy
  renderer, and `xvfb-run` opens a window under Wayland (`FINDINGS.md`, 2026-10-03).
* Run it outside the sandbox, and in the background.
* Split the CPU with `perf record -F 499 --call-graph dwarf,16384 -p <godot pid> -- sleep 20` on
  the `compile-profiling` build. Go back to `make compile-debug` afterwards, because the profiling
  build replaces the debug `.so` the editor loads.
* **The script fails fast. Never wait out a fixed delay blind.** Check the log and the process
  every second, and stop at once on `Invalid game directory`, a crash, a `SCRIPT ERROR` or a dead
  process. Only a clean load reaches `perf`. A 150 s `sleep` before the first look once spent the
  whole wait on a scenery that had refused to load in the first second.
* **The user directory is custom**: `demo/project.godot` sets `custom_user_dir_name`, so `user://`
  is `~/.local/share/MaSzyna-Reloaded/`, not `~/.local/share/godot/app_userdata/MaSzyna Reloaded/`.
  A test that points the game directory at the fixtures (`save_maszyna_game_dir(FIXTURES_GAME_DIR)`)
  and is killed before its `after_all` leaves `game_dir="res://tests/fixtures"` in that
  `settings.cfg`. Every later `-s` start then quits with "Invalid game directory". Check
  `game_dir` there before the first run.

## Read the instruments correctly

This is where most of the time gets lost, so start here.

**The F3 debug menu (`demo/addons/debug_menu`) does not show what it seems to.**
`debug_menu.gd:383` computes its "CPU" as

```gdscript
RenderingServer.viewport_get_measured_render_time_cpu(viewport_rid)
    + RenderingServer.get_frame_setup_time_cpu()
```

so **"CPU" is the renderer's CPU time only** and "GPU" is the viewport's GPU
time. Neither includes `_process`, `_physics_process`, signals or any game
script. "Total" is wall-clock between frames. A frame that is slow in game code
therefore shows up as a large unexplained gap between Total and CPU+GPU. Seeing
`Total 176 ms, CPU 2.4, GPU 10.2` does **not** mean the engine is stalling - it
means ~164 ms is in script or physics and this tool cannot see it.

What actually splits that gap:

| instrument | what it covers |
|---|---|
| `Performance.TIME_PROCESS` | all `_process` callbacks |
| `Performance.TIME_PHYSICS_PROCESS` | all `_physics_process` callbacks |
| Editor profiler → Script Functions | per-function time **and call counts** |
| `addons/libmaszyna/debug_hud/frame_time_panel.gd` | the above in-game, plus physics steps per frame |
| `addons/libmaszyna/scenery/scenery_streaming_panel.gd` | streaming backlog, builds/s, budget in use |

**Call counts matter more than times.** The single biggest win in this codebase
was found by noticing `_physics_process` had `Calls: 8` - Godot was running
eight catch-up ticks per rendered frame, multiplying everything inside.

## Cost map of this project

Work outward from the measurement, not from this list - but when you have a
split, this is where each part lives.

- **`TIME_PHYSICS_PROCESS` / `TIME_PROCESS` with vehicles present** →
  `addons/libmaszyna/servers/rail_vehicle_physics_server.gd`. Its step now runs
  on `_process` with the frame delta, mirroring the original's
  `vehicle_table::update(Deltatime, Iterationcount)` (`DynObj.cpp:8181`), with
  `process_priority = -100` so it precedes `RailVehicle3D`'s visual transform.
  Inner cost scales as `iterations = ceil(delta / PHYSICS_STEP)` × controllers;
  `PHYSICS_STEP` is a constant in that file, and *raising it* is the only lever
  that reduces work per frame - changing the physics tick rate does not, because
  the iteration count compensates exactly.
- **Script Functions dominated by `track_*` / `switch_*` calls** → the hot path
  queries `TrackServer` (GDScript) across the autoload boundary hundreds of
  times per frame. Hoisting a repeated query out of a loop is worth more than it
  looks; a query answered by a value already in hand is pure profit.
- **Renderer CPU/GPU genuinely high** → geometry actually in range. The lever is
  `maszyna/rendering/scenery_draw_distance`, and `SceneryStreamingServer`
  decides what exists at all.
- **Long load, or the world filling in while you drive** →
  `SceneryStreamingServer` (`src/scenery/`). Check its panel: `Pending builds`,
  `Builds/s`, `Budget`. A backlog drains at roughly *frame rate × budget*, so at
  low fps an idle budget takes minutes.
- **Vehicles juddering or "kicking"** → a visual transform read at a different
  rate than it is written. `RailVehicle3D::_process_impl()`
  (`src/vehicles/rail/RailVehicle3D.cpp:368`) recomputes the transform every rendered
  frame from the simulation's track offset; if the simulation runs on a
  different clock, every other frame repeats a position. Godot's built-in
  physics interpolation does **not** help here, because the transform is set in
  `_process`, not `_physics_process`.

## Recon on the scenery data itself

When two sceneries behave differently, diff what they actually contain instead
of theorising. Walk the `.scn` include tree counting `node <max> <min> <name>
<type>` occurrences *with include multiplicity* (memoize per file; a file
included 24 000 times counts 24 000 times). The game dir comes from
`UserSettings.get_maszyna_game_dir()`.

This is fast and it settles arguments - but read the result carefully. Finding
that the slow scenery has 12% more vehicles does not explain a 12× slowdown; a
cliff that steep is a feedback loop or a threshold, not scaling. In practice the
vehicle-count correlation was a red herring twice over: the *fastest* Bałtyk
variant had the *most* vehicles.

## Traps that have already cost time here

- **Never put `#` comments in `demo/project.godot`.** Godot's ConfigFile writer
  drops newlines and glues the comment onto the next key, silently turning a
  setting into part of a comment. The editor then shows a bogus settings
  category named after the comment text. Write the key alone, `grep` it back,
  and remember an open editor holds its own copy and will overwrite the file.
- **`MaterialManager.get_material()` keeps only a `weakref`**
  (`material_manager.gd:73`). Whoever wants the material rendered must hold a
  strong reference for as long as the instance lives - otherwise it is freed and
  the `RenderingServer` override dangles, which looks like untextured white
  geometry. `E3DInstanceData.materials` and `ChunkState.material` exist for
  exactly this reason.
- **A settings change needs a restart** - most of these are read once at
  startup.
- **`viewport_get_measured_render_time_*` returns 0** unless measurement was
  enabled on that viewport.

## Working rules

- Build the instrument rather than asking the operator for the same number
  twice. A HUD panel or a headless profiling run costs you minutes; a repeated
  question costs them a full restart and scenery load.
- State plainly when a measurement disproves your own previous claim, and when a
  regression is yours. This project's operator reads the diffs.
- Before claiming a change is active, verify it - a setting that did not apply
  looks exactly like a setting that did not help.
