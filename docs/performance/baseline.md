---
layout: page
title: "Performance - baseline"
---

Where libmaszyna's share of a frame goes in the game (MaSzyna Reloaded), measured before
optimising. The method, the machine and the traps of the measurement are in the game's
`docs/performance/baseline.md`. Changes are in [changes](changes.md). The rule is that the picture
does not change.

## Wrzosy (`wrzosy_eie8310.scn`), main thread

About 300 vehicles; the player standing at Wolica. The main thread is busy 99% of the time; with the
simulation paused the frame falls from ~66 ms to the GPU's ~52 ms. Shares are of the main thread's
samples (perf, `cycles:u`, 20 s, profiling build, on AC).

| Phase | Share |
|---|---|
| `SimulationServer::simulation_advance` | 70% |
| `MaszynaMoverVehicleServer::stepping_advance` | 54% |
| - `RailVehicleServer::vehicle_process_movement` | 17.7% |
| -- `_move_vehicle` > `_place_body` (axle transforms: `Curve3D::sample_baked`, `track_get_domain_curve`, `orthonormalized`) | 9.6% |
| - `RailVehicleServer::_update_neighbours` | 16.1% |
| -- `_find_vehicle` > `_walk_axle` | 7.3% |
| - `MoverRailVehicleController::_integrate` | 10.1% |
| - `TMoverParameters::ComputeTotalForce` | 4.1% |
| `RailVehicleServer::_walk_axle` (all callers) | 8.9% |
| - `_motion_connection` > `TrackServer::track_find_next` | 6.5% |
| `RenderingServer` drawing (engine) | 15% |

The Mover step runs `ceil(delta / PHYSICS_STEP)` sub-steps of 10 ms per frame for every vehicle,
parked ones included (`MaszynaMoverVehicleServer.cpp:111-137`). A longer frame means more
sub-steps in the next one, so the cost of a sub-step counts twice.

### Calls into the engine on the hot path

Every one of these crosses the extension boundary and does more work than it looks.

* **`X::get_instance()` of 23 servers** looked the singleton up by name:
  `Engine::get_singleton()->get_singleton("TrackServer")` builds a `StringName` from a C string and
  scans the engine's singleton list (`Engine::is_singleton_editor_only`). In the Mover step
  `TrackServer::get_instance()` is called per axle. That was 5.6% of the thread in
  `is_singleton_editor_only` and 0.8% in making `StringName`s.
* **RID compare and hash:** godot-cpp's `RID::operator==` is a variant operator call
  (`OperatorEvaluatorEqual<RID, RID>`) and `RID::get_id()` a builtin method call. A
  `HashMap<RID, ...>` makes one on every hash and every compare. That was 8.8% of the thread: most
  of it inlined in `tracks.has()` (`track_exists`), `vehicles` lookups, and
  `SceneryStreamingServer::_apply_builds`.
* **`TrackServer::track_find_next()`** went through `track_get_endpoint_connections()`, which makes
  a `Ref<TrackEndpointRef>` in a `TypedArray` for every connection, on every axle crossing a track's
  end: 3.3% in making them, plus `Array` reference counting.
* `VehicleController::get_component` > `_get_component_of_type`: 1.4%.
