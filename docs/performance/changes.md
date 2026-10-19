---
layout: page
title: "Performance - changes"
---

Changes made to libmaszyna for performance, each measured on the game's Wrzosy
(`wrzosy_eie8310.scn`, about 300 vehicles) before and after. None changes behaviour or the
picture. The baseline is in [baseline](baseline.md).

## Servers hold their singleton (23 servers)

`X::get_instance()` looked the singleton up by name in the engine on every call. Every server now
keeps a static pointer, set in its constructor and cleared in its destructor - the shape
`SimulationServer`, `GameDataServer`, `ResourceLazyLoader` and `RailVehicleRenderingServer` already
had. `RadioServer` and `PersonServer` got a constructor for it; servers without a destructor got
one.

* **Before:** `Engine::is_singleton_editor_only` 5.6% of the main thread, `StringName` from a C
  string 0.8%.
* **After:** 0% and 0.17%.
* **Frame:** no measurable change on its own (68 -> 70 ms, within noise): the frame is held by the
  simulation's sub-steps and by the GPU (baseline).

## RID-keyed maps and sets hash and compare without the engine

`src/utils/RIDHash.hpp`: `RIDMap<V>` and `RIDSet` - `HashMap`/`HashSet` keyed by `RID` with a
hasher and a comparator that read the RID's 64-bit id from its bytes (`rid_get_id()`), as the
engine's `RID` holds nothing else. Every `HashMap<RID, ...>` and `HashSet<RID>` in `src/` moved
onto them.

* **Before:** 8.8% of the main thread in RID compares and `get_id()` through the engine.
* **After:** 0.05%. The Mover step fell from 53.6% to 46.0% of the main thread
  (`vehicle_process_movement` 17.7% -> 13.5%, `_update_neighbours` 16.2% -> 11.5%).
* **Frame:** 68 -> 68-69 ms, no measurable change: the shares of drawing (15.3% -> 18.4%) and of
  GDScript (28.4% -> 32.5%) grew by what the step lost. Why the frame does not follow the main
  thread's work is under investigation (baseline).

## `track_find_next()` reads the node's endpoints in place

It walked `track_get_endpoint_connections()`, which makes a `Ref<TrackEndpointRef>` per
connection, on every axle crossing a track's end. It now walks the node's `endpoint_refs`. A
repeated endpoint, which the connections list left out, falls into the existing "same candidate"
branch, so the result is the same.

* **Before:** `track_find_next` 5.8% of the main thread (after the two changes above).
* **After:** 2.4%. The track topology tests (`test_track_server_topology.gd`,
  `test_track_server_double_slip_topology.gd`, 32 tests) pass.

## Where the three stand together (Wrzosy, 1920x1080, on AC)

k is the simulation's CPU seconds per simulated second (main thread samples under
`SimulationServer::simulation_advance`); the frame settles at about R / (1 - k).

| Build | Mover step per simulated s | k (whole simulation) | R (rest of frame) | FPS |
|---|---|---|---|---|
| before | 0.530 s | 0.694 | 19.6 ms | 15.0 |
| + singletons | 0.475 s | 0.655 | 21.8 ms | 15.1 |
| + RIDMap | 0.453 s | 0.646 | 22.2 ms | 15.2 |
| + track_find_next | 0.439 s | 0.628 | 22.3 ms | 15.3 |

The Mover step is 17% cheaper. At 1920x1080 the GPU needs 52 ms of a ~66 ms frame, and the drawing
phase (R) grew as the main thread got lighter: it waits for the GPU there. The FPS gain shows where
the GPU has room (720p).

## The material types' variants are files; only the cut-out one has `discard`

Every lit material type held `discard` behind the `transparency` uniform, so every surface - the
opaque ones too - drew with its own shader in the depth prepass and in each of the sun's four
shadow cascades instead of Godot's shared depth material. `MaszynaMaterialFactory._get_shader_variant()`
also built its variants (blended, both faces, specgloss) with `code.replace()`, against
`CODE_STYLE.md`.

* `types/<type>.gdshader` is now the opaque variant with the back faces culled. Its code is in
  `types/<type>.gdshaderinc`, with `discard` under `#ifdef MASZYNA_ALPHA_SCISSOR`.
* `types/variants/` holds the other 69 combinations as files. `SHADER_VARIANTS` lists what each
  type has, and the factory picks the file once it knows the material's transparency.
* The `discard` is dropped only where it never ran: a variant without it is used only when
  `transparency` is not `AlphaScissor` (and, for parallax, `use_alpha_scissor` is off). The picture
  is the same by construction; no screenshot A/B was taken.
* Tests: `test_material_manager_variants.gd` (30, with the cut-out material's expected shader path
  moved to its `_scissor` variant, by the operator's consent) and `test_e3d_lights_state.gd` pass.
  All 82 type and variant shaders compile on Vulkan with no error.

**Measured** (Wrzosy, 1920x1080, on AC, the ablation run, one run each, the old files put back for
the "before" run):

| | before | after |
|---|---|---|
| GPU, start of the run | 53.0 ms | 52.2 ms |
| GPU, end of the run | 53.1 ms | 53.0 ms |
| renderer CPU | 12.5 ms | 11.8 ms |
| sun shadows' cost (GPU with them minus without) | 13.3 ms | 12.5 ms |
| frame | 66.8 ms | 66.2 ms |

Under 1 ms of the GPU, within the noise of one run, far from the 2.2 ms of the upper bound
(findings): what the shadow passes draw on Wrzosy is most likely mostly cut out (trees, fences),
and those keep their `discard` - not counted. The frame is held by
the simulation (66 ms), so the FPS does not change.

## A directional light without energy casts no shadow

The sky backend (gnd-skydome, `Skydome.gd:1190-1204`) drives one world light: the sun by day, the
moon by night, and zero energy while neither is up. Its shadow stayed on, so at night its four
cascades drew the scenery again for a light that shines on nothing. `MaszynaSkyEnvironment`
keeps what the settings want in `_shadow_wanted`, and `sync_shadow_casting()` turns the shadow on
every frame only while the light's energy is above zero; the cab light follows its world light's
energy.

**Measured** (calkowo_v2 at night, `calkowo_v2_osoba_wil_noc.scn`, 1920x1080, on AC, the player's
cab, one run each):

| | before | after |
|---|---|---|
| FPS | 17.3 | **18.9** |
| frame (median, p99) | 57.6, 81.7 ms | **52.8, 68.1 ms** |
| render GPU | 54.4 ms | **42.9 ms** |
| render CPU | 18.6 ms | 17.6 ms |

* The picture is the same: the screenshots of the two runs differ in 38 pixels of 2 M beyond a 2%
  tolerance (PSNR 50 dB), the frame taken at a slightly different moment.
* Night is now held by the CPU (the CPU model gives about 49 ms) rather than the GPU.
* The sky and environment tests pass (the game's `test_gnd_skydome_environment.gd`, 8;
  `test_maszyna_environment_node.gd`, 9).

## The second round, on the Fedora desktop

Measured on the game's Wrzosy with its rendering on a thread of its own (the game's
`docs/performance/changes.md`), 1920x1080, a symbol build of Godot 4.7.2 for the shares.

### The controller of a placed vehicle is held

`RailVehicleServer::_get_controller()` turned the placement's `ObjectID` into the controller on
every call: `ObjectDB::get_instance()` (an engine call that takes a mutex) and a cast across the
extension boundary. The Mover step calls it per vehicle several times a sub-step.

* **Change:** `VehiclePlacement` holds `Ref<RailVehicleController>`, taken where the id was taken -
  `vehicle_attach()` and `vehicle_controller_changed`, which VehicleServer emits on every binding
  and unbinding, so the placement never holds a controller the vehicle no longer has.
* **Before:** 4.7% of the main thread (`ObjectDB::get_instance` 3.5%, the mutex 1.4%,
  `is_class` 1.5%). **After:** `_get_controller` gone from the profile, the mutex 0.14%.

### Components indexed by type

`VehicleController::_get_component_of_type()` walked the vehicle's components and asked each its
type, for every `get_component()` - per axle and per sub-step in the Mover step.

* **Change:** an index of the first component of each type, made again whenever the list changes
  (`add_component()`, `remove_component()`, `set_components()`); the first one of a type still
  answers, as the walk did.
* **Before:** 2.8% of the main thread. **After:** 2.2% - most of the cost is the `Ref` handed back
  and the call, not the walk.

### Material types read their shader defaults once

`MaszynaMaterialFactory.create()` duplicated the type's base material and `_apply()` read every
shader parameter of it again. A parameter the material does not set is read from the
RenderingServer (`shader_get_parameter_default`), and with the rendering thread on its own every
such read waits for it: about 1 600 waits in a run while Wrzosy streamed its materials (Godot's
warning "causing RenderingServer synchronizations").

* **Change:** `MaszynaShaderMeta` keeps a copy of the type's base material with every shader
  parameter set from its default, once; materials are made from the copy. The values, and so the
  picture, are the same: `duplicate()` already set every default explicitly on the material made.
* **A copy, not the type's material:** the first version set the defaults on the loaded `.tres`,
  and an editor open on the project wrote all 13 type files back.
* **After:** no synchronization warning from the factory in a run.

### Cab logic of unseen cabs

Every driven vehicle registers its cab logic for the cab its driver sits in - on Wrzosy 264 AI
drivers, each running the cab lights, the main switch and the door permits on every simulation
step: 18.5% of the main thread (a probe: lights 8.5%, main switch 7.8%, door permits 2.3%).

* **Change:** `CabinSystem.register_display_process()` - a callable that only works out what a cab
  shows. The cab lights (`LegacyCabinCabLights`) are one: they run with the simulation while their
  vehicle's interior is shown, else just before it is shown (`cabin_show()`) or when a getter
  reads them (`cabin_get_light_level()`, ...). Their inputs - the switches and the vehicle's power
  after the step - are the same at the read as at the step, so the values read are the same; the
  elements that follow the change signals exist only in a shown cab.
* The main switch and the door permits send commands, so they run on every step as before - with
  their controls read in place and `CabinState.vehicle_component()` asking VehicleServer directly.
  The process lists are replaced when they change, not copied on every step; `act()` makes the
  vehicle's id only for an error message.
* Tests: `test_cab_lights.gd` (7, one of them reading the lights of a cab never shown),
  `test_cabin_system_state.gd`, `test_legacy_cabin_doors.gd`, `test_legacy_cabin_cab_activation.gd`,
  `test_cabin_switch.gd`, `test_train_cab_change.gd` and others, 14 scripts, pass.

### Where the round stands

| Build (symbol build of Godot) | FPS | Main thread busy | Main thread a frame | Rendering thread busy |
|---|---|---|---|---|
| the controller held | 50.2 | 90% | 17.9 ms | 80% |
| + components, materials, cab logic | **53.6** | 77% | **14.4 ms** | 83% |

The main thread's time a frame fell by a fifth; the rendering thread now limits the frame.

* Failing before the round, not by it: `test_material_manager_variants.gd` (7/27),
  `test_material_preload.gd` (1/3), `test_mmd_cabin_instancer.gd` (37/39) and
  `test_zzz_ep07_cabin_main_switch.gd` (0/5) fail the same with this round's scripts taken out.
