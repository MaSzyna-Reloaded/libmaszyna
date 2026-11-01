---
name: vehicle-patches
description: Patch a MaSzyna vehicle's files the data lacks - its wiper arms, its cab's wiper handles and switch, its FIZ wiper list, its cab glass's rain material and wiper mask - in libmaszyna's vehicle patch (addons/libmaszyna/patches). Use before adding or changing any MaszynaVehiclePatch entry, before measuring wipers, handles, knobs or glass from a model, and when a player reports a cab whose wipers, handles or wet glass behave wrong. Answers "where does the patch of this vehicle go", "how is a wiper arm, a handle or a glass measured", "why does this handle shear" and "how do I prove the patch right before pushing".
---

# Patching vehicles

The data of many vehicles says nothing of their wipers: no `WiperList:` in the FIZ, no
`animwiperprefix:` and no wiper control in the MMD, no `rain_windscreen` `.mat` for the cab glass.
The game fills that in by hand, per file, in libmaszyna's vehicle patch - never by guessing from
names at run time (the operator removed `MmdManualWiperHandles` for that). The quirk is described in
`MASZYNA_ORIGINAL_QUIRKS.md` ("Wipers of no switch are driven by hand").

## Where it lives

* `addons/libmaszyna/patches/maszyna_vehicle_patch.tres` - a `MaszynaVehiclePatch`: only references,
  one per patched file.
* `addons/libmaszyna/patches/dynamic/<operator>/<vehicle dir>/<file>.tres` - a
  `MaszynaVehicleModelPatch` of one file of the data, named after it: `6da.e3d.tres`,
  `6dg.fiz.tres`, `okno_czolo_gora.mat.tres`; a wiper mask PNG beside the glass's `.tres`
  (`okno_czolo_gora_wipermask.png`, with its `.import`).
* The game hands it over: `game.tscn`'s `vehicle_patch` -> `MaszynaVehicleSystem.patch_set()`.
* `path` of an entry is the file's path in the game's directory: a model's `.e3d` and `.t3d` alike, a
  `.mat` where the original looks for it first (`<vehicle dir>/<texture>.mat`, or the texture's own
  path when it starts with `dynamic/`).

A patch is read only where the file has no definition of its own: a FIZ's `WiperList:`, an MMD's
`animwiperprefix:` or label, an existing `.mat`. A patch never overrides the data.

## The elements

| Element | Of | Read by | Fields |
|---|---|---|---|
| `MaszynaWiperArmPatch` | a vehicle's model | `MaszynaRailVehicle3DInstancer` -> `MaszynaE3DRailVehicleRenderingServer` | `submodel` (path), `pivot`, `axis` (its own frame), `turn` [deg, signed] |
| `MaszynaCabControlPatch` | a cab's model | `MmdCabinInstancer.parse()` | `label` (`wipers_sw`, `reloaded_manualwiper<N>`), `submodel` (path), `animation_type`, `scale`, `offset`, `mouse_drag_signs` |
| `MaszynaWiperListPatch` | a FIZ | `FizVehicleBuilder` (through `MaszynaVehicleSystem.fiz_get_patched_wipers()`) | `angle`, `positions` (`RailVehicleWiperListItem`) |
| `MaszynaMaterialPatch` | a missing `.mat` | `MaszynaMaterialParser` | `shader`, `textures` (data paths or the patch's `res://`), `parameters`, `vector_parameters` |

* A submodel is named by its path in the model's tree. A submodel of no name is an empty segment:
  a model whose root has no name gives `/banan/wycieraczka1`, not `banan/wycieraczka1` - the
  measurement missed it 65 times once, and the game then found none of those arms and handles.
* The FIZ parser compiles as the library starts, before any script class of the patch can be
  resolved: it takes a `RailVehicleWipers` (C++), never a patch class - a patch class there broke
  the whole FIZ loader in the game's export.

## Measuring, per vehicle family

Do it family by family, every cab of it, and look at each one (below). Rules of the data that cost
us before:

1. **Find every model of the vehicle from its MMD**, includes expanded: `include` with its file on
   the next line (181_v2), parameters `(pN)` over several lines up to `end`, `models: \main/x.t3d#`
   (a leading `\`, a trailing `#`), `cabNmodel:` with backslashes. The catalogue's vehicles are
   those of `textures.txt` (`MaszynaVehiclesBank`); carriages and people are left out.
2. **The cab and the body share one frame** in the composed transforms (verified: SM42 handles on
   the arms' hubs within 0.2 m, EN57 hubs right under the glass). Compose every submodel's matrix
   from the root.
3. **What kind of wipers the real vehicle has** is the operator's call, never inferred: manual
   handles (SM42, EU07), an automatic switch (6Dg: 0-3, 181: 0 north, 1 left, 2 right), and
   sometimes rotary switches in the model that are something else (EU07 `sw-wycieraczka-*`).
4. **Arms**: a separate submodel (SM42 `wycieraczka01-04`, EN57AL, EN57AKL, SP42) - its pivot is
   its hub (the thickest end along its length; the SM42's origin is mid-blade, the hub 0.376 m along
   Z), its axis the glass's normal in its own frame, its turn signed so the blade goes the way the
   real one does. Arms merged into a static mesh (`detale` of EN57_v2's cab, of EU07's
   `303e/base.e3d`) cannot turn: only the glass is wiped then.
5. **Handles and knobs**: `rot` turns about the submodel's own Y in its own frame. A matrix of
   uneven scale (EU07 `mech_wyc_raczka02`: 3.2 x 5.2 x 9.9, its mechanism 11 x 11 x 35) shears
   instead of turning - such a control needs turning in its parent's frame, not a scale or a sign.
   A handle's turn has the sign of the arm on its spindle; a knob's of its labels' order as the
   driver sees them.
6. **Glass**: the windscreen panes the blades sweep get the wiper mask; panes no blade reaches
   (EU07's middle pane, side windows) are wet but not wiped. Masks are RGBA, a channel per wiper,
   the value the phase of the sweep - written sRGB-encoded (the shader reads `source_color`). Panes
   sharing their UV (SM42's four front panes) need `wiper_pane_N` (the pane's middle in the mesh's
   own frame), or one wiper wipes them all. A glass of no blades to measure is swept across - its
   direction in the glass's frame, so `]` wipes from the driver's left to right.
7. **Rain density** `raindrop_grid_size`: some 141 drops a metre (EP09's front glass: 150 a UV unit
   at 1.06 m a unit), measured along the glass's triangles' edges - not across panes that share UV.

## Proving it before a push

* **Look at every cab**: render it with its patched handles, knobs and arms at position 0 and at the
  end of their sweep, and the patched glass coloured by its mask or sweep - in Godot, headless, in an
  own sway (never the operator's display, never beside the operator's editor), or offline from the
  composed meshes. A handle that shears, turns the wrong way or out of view, a mask on the wrong pane
  shows at once; numbers alone did not show any of it.
* Load the patch headless and read it back: `MaszynaVehicleSystem.patch_set()`, then the readers
  (`MaszynaMaterialParser.parse()`, `fiz_get_patched_wipers()`, `MmdCabinInstancer.parse()`).
* Compile every changed shader headless; run `demo/tests/test_vehicle_patch.gd` only where the
  operator's settings cannot be touched (the tests save to `user://test_settings.cfg`).
* A vehicle the operator reports is fixed in its family, and the family looked at again - the
  first data were rules applied to all, and the operator found them wrong vehicle after vehicle.
