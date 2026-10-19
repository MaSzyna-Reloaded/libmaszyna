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
