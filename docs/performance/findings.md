---
layout: page
title: "Performance - findings"
---

What measuring found in libmaszyna that is not a change made: ideas tried and given up because the
numbers did not carry them, and where the frame's work actually is. The game's
`docs/performance/` holds the method.

## Where the draw calls are (Wrzosy, 1920x1080, the player's cab at Wolica)

| | Draw calls | Primitives | GPU |
|---|---|---|---|
| everything | 8 427 | 3.70 M | 53.8 ms |
| sun shadows off | 4 168 | 1.77 M | 40.4 ms |

Half the draw calls and half the primitives are the sun's four shadow cascades drawing the scenery
again. The rest is the opaque pass (with its depth prepass). Godot does not join instances into one
call by itself; only fewer instances drawn, or a `MultiMesh` for repeated mesh and material pairs
(`TODO.md`), make fewer calls.

## Tried: the origin-centred box only for submodels with a visibility range

`E3DOptimizedBackend` and `E3DNodesBackend` give every submodel a custom AABB widened to be centred
on the model's origin (`E3DInstanceBackend::_visibility_aabb()`), because Godot measures a
visibility range from the box's centre. The idea: a submodel without a range culls tighter with its
mesh's own box, in the view and in every cascade.

* **Measured:** 8 427 draw calls and 53.8 ms GPU against about 8 450-8 500 and 53.3 ms before -
  nothing. Most submodels have a range of their own or their node's.
* **Given up** (2026-10-18), the code reverted.

## Tried: no `discard` in the opaque material shaders

Every lit material shader (`types/default.gdshader`, `normalmap`, `normalmap_specgloss`,
`detail_normalmap`, `sunlessnormalmap`, `shadowlessnormalmap`, `parallax`, `parallax_specgloss`)
holds `discard`, switched by the `transparency` uniform. A shader with `discard` does not get
Godot's shared depth material in the shadow and depth passes, so every surface draws there with its
own shader.

* **Measured** (the `discard` taken out of all eight for one run - an upper bound, the cut-out
  materials then drawn wrong): GPU 53.3 -> 51.1 ms (-2.2 ms, 4%), the sun shadows' cost 12.7 ->
  11.3 ms, the renderer's CPU 20.7 -> 20.0 ms.
* **Done** (2026-10-18), the variants as files, the opaque ones without `discard` - measured in
  `changes.md`: under 1 ms, as most surfaces in the shadow passes are cut out.

## Measured: how far a MultiMesh could join the scenery's instances

A throwaway counter in `E3DOptimizedBackend::_add_submodel()` (never committed) counted every
submodel instance built on Wrzosy by its mesh, material and visibility range.

* **26 055 instances, 4 735 different (mesh, material, range)**; 19 490 of them (75%) in groups of
  8 or more identical ones, 7 658 in groups of 128 or more.
* **Not one is without a visibility range** - the submodel's own or its scenery node's - which is
  also why the tighter box above changed nothing.

| `range_end` | Instances | In groups of 8 or more |
|---|---|---|
| up to 100 m | 8 967 | 7 418 |
| up to 300 m | 5 300 | 3 758 |
| up to 1 000 m | 10 224 | 7 025 |
| up to 2 000 m | 163 | 139 |
| beyond | 1 383 | 1 134 |
| a far LOD (`range_begin` > 0) | 6 477 | 0 |

A `MultiMesh` instance has one visibility range, measured from the centre of its box. Joining
instances moves the distance each one appears and disappears at by up to the size of its cell - on
a 100 m range, with a 20 m cell, up to 14 m. That changes the picture, against the rule of this
work; the operator chose the opaque shaders without `discard` instead (2026-10-18). Within the rule a join would need instances whose
range is long against the cell (the 1 546 beyond 1 000 m), too few to matter.

## Measured for #301: the Mover stepped at 1/30 s

Issue #301 asks for one fixed 30 Hz simulation tick with the Mover stepped once per tick at
dt = 1/30 s, and the picture interpolated between ticks. Today `MaszynaMoverVehicleServer`
integrates every frame in sub-steps of at most `PHYSICS_STEP` = 0.01 s, as the original does
(`drivermode.cpp:196`, "normalnie 0.01s"), and refreshes every vehicle's location and neighbours
before each sub-step (archive, 2026-09-27 "couplers stiffened by a long frame").

**Cost** (one run, `PHYSICS_STEP` set to 1/30 for it only and reverted; Wrzosy, 1280x720, on AC):

| | 0.01 s | 1/30 s |
|---|---|---|
| Mover step per simulated second | 0.444 s | **0.234 s** |
| whole simulation (k) | 0.708 | 0.599 |
| FPS | 15.0 | **16.5** |
| frame (median) | 66.7 ms | 60.0 ms |

The Mover step costs about half (not a third: the step's per-vehicle work outside the sub-steps
stays); the AI and the drawing are the rest of the frame.

**What a 1/30 s step changes in the physics** (measured 2026-10-18):

The eszelon of `calkowo_v2_eszelona.scn` (ST44-700) does not move without the player, so the check
took the AI's EU07-193 on Wrzosy (`wrzosy_eie8310.scn`, 18 vehicles, starting at 60 km/h, no
timetable). The simulation was paused and advanced by hand by 1/30 s each frame for 120 simulated
seconds, once with `PHYSICS_STEP` = 0.01 and once with 1/30 (the experiment reverted).

| t [s] | 0 | 5 | 10 | 20 | 30 | 40 | 60 | 80 | 120 |
|---|---|---|---|---|---|---|---|---|---|
| speed [m/s], 0.01 s | 16.64 | 16.48 | 16.31 | 12.62 | 8.74 | 7.02 | 10.08 | 10.97 | 10.55 |
| speed [m/s], 1/30 s | 16.64 | 15.91 | 15.11 | 10.47 | 1.88 | **0** | 0 | 0 | 0 |

* **The AI's commands were the same in both runs** (`ai.log`, the same commands at the same
  seconds up to 18 s), yet already at 2-7 s, with the train coasting and no brake command sent, the
  train slows down at 0.20-0.27 m/s² against 0.03 m/s². The difference is in the Mover, not the
  driver. With 0.01 s the largest deceleration after the start is 0.60 m/s². With 1/30 s it is 1.36,
  and the train stops at 32 s and stays there.
* Which term it is was not looked for. The candidates are the coupler term above
  (`CouplerForce()`, `Mover.cpp:4779-4784`) and the explicit pneumatics and adhesion, which the
  original never ran at a step above 0.01 s.
* **Conclusion for #301:** a 30 Hz tick cannot step the Mover once at dt = 1/30 s without changing
  how trains drive. A fixed tick that keeps sub-steps of 0.01 s inside it (three per tick) keeps
  the physics. It saves none of the Mover's cost, which comes from the sub-steps, and gains only a
  fixed rate.

Derailments seen in these runs (`cause=collision`, road cars at crossings, 52-75 a run) are there
in every build, with the 0.01 s step as well - not the step's.

## Measured: what is left on the main thread (Fedora, rendering on its own thread)

* **The Mover step is 44%** of the main thread. The Mover's own physics (`_integrate`,
  `ComputeTotalForce`, `UpdatePipePressure`) is about 25%, the server's placing and neighbours
  (`vehicle_process_movement`, `_update_neighbours`, the axle transforms) about 20%.
* **The Mover draws from one random engine for all vehicles** (`Random()`, `utilities.cpp`:
  adhesion, track irregularities, brake delays). Stepping the trainsets on several threads would
  change the order of the draws: the behaviour the same in distribution, not bit by bit. Not done.
* **`Curve3D::sample_baked` is called into the engine three times per axle** per sub-step
  (`_axle_transform()`), with `get_baked_length()`: about 3%. Sampling the baked points here would
  have to give the engine's result to the bit (linear, `cubic = false`). Not done.

## A ShaderMaterial's property list waits for the rendering thread

* **Seen:** with the game's rendering on its own thread, the main thread and the rendering thread
  were both busy under 90% at 56 FPS - neither the limit. Godot's log held about 2 300 warnings
  "causing RenderingServer synchronizations" a run, from the material factory.
* **Why:** `ShaderMaterial.get_property_list()` lists the shader's parameters through
  `RenderingServer.shader_get_parameter_list()`, and a parameter the material does not set reads
  its default with `shader_get_parameter_default()`. Both return a value, so with a rendering
  thread they wait until it has run everything queued - the whole frame being drawn.
  `Resource.duplicate()` walks the property list too.
* **Rule:** no `ShaderMaterial` property list, `duplicate()` or unset parameter read on a path that
  runs while a scenery streams. What is needed from a material type is read once and kept.
