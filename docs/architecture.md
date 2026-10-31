---
layout: page
title: "Architecture of the MaSzyna Port"
---

libmaszyna is a plugin of libreloaded. The core's architecture and its extension points are
described in libreloaded's [Architecture](https://maszyna-reloaded.github.io/libreloaded/architecture.html#plugins). This page describes what
libmaszyna adds at those extension points and how the port is organized.

* [The plugin](#the-plugin)
* [Extension points](#extension-points)
* [Inside the port](#inside-the-port)

## The plugin

### Build

`plugin.cmake` adds `src/maszyna` and `doc_classes/` to libreloaded's library, links
`addons/libmaszyna` into the project and names the plugin class `MaszynaPlugin`. It also builds
`maszyna-python-host`, a separate process that runs the Python screens of the cabs.

### `MaszynaPlugin`

`MaszynaPlugin` (`src/maszyna/MaszynaPlugin.cpp`) is libmaszyna's `ReloadedPlugin`:

* `register_classes()` registers libmaszyna's classes and sets:
  * `MaszynaMoverRailVehicleController` as the default vehicle controller
  * `MaszynaDataPath` as the resolver of data paths. MaSzyna's data was written for Windows, whose
    file names are case-insensitive.
  * the Lua modules for memory cells (`MaszynaLuaMemoryModule`) and `eu07.events`
    (`MaszynaLuaEventsModule`), when the core is built with Lua
* `create_servers()` creates libmaszyna's singletons: `MaszynaGameDirectory`, `MaszynaE3DParser`,
  `MaszynaE3DRenderingServer`, `MaszynaMoverVehicleServer`, `PythonScreenServer` and
  `MaszynaScenarioEventServer`. It registers them with the core's servers (see below) and adds the
  E3D resource loader.
* `free_servers()` frees them in reverse order.

### The addon

`addons/libmaszyna/libmaszyna.gd` registers the GDScript side:

* autoloads, such as `MaszynaMaterialManager`, `MaszynaVehicleSystem`, `MaszynaE3DModelManager`
  and `MaszynaSceneryChunkRenderingServer`
* custom types: `MaszynaE3DModelInstance` and `MaszynaRailVehiclePhysicsNode`
* the FIZ importer, the editor plugins and the `maszyna/*` project settings

Its part of the game is `addons/libmaszyna/libmaszyna_runtime.gd`, the plugin's
`ReloadedRuntimePlugin` (`MaszynaPlugin`'s `runtime_script`): it makes and registers
`MaszynaVehicleCatalogProvider` in its `_enter_tree()` - before the main scene, whose vehicles ask
for it - and unregisters it in its `_exit_tree()`.

## Extension points

| Core extension point | libmaszyna's implementation |
|---|---|
| Vehicle implementation | `MaszynaMoverVehicleServer`, named `maszyna_mover`: the original MOVER code |
| Controller and components | `MaszynaMoverRailVehicleController` and its `MaszynaMoverRailVehicle*` components |
| Engine parts | `MaszynaMoverDriveUnit`, `MaszynaMoverCircuitUnit`, `MaszynaMoverTractionMotorsUnit`, `MaszynaMoverDieselEngineUnit` |
| Vehicle node | `MaszynaRailVehiclePhysicsNode`, which builds the controller from a `.fiz` file (`FizVehicleBuilder`) |
| Appearance | `MaszynaRailVehicleAppearance` |
| Vehicle catalogue | `MaszynaVehicleCatalogProvider`, named `maszyna`: the vehicles in the game directory and their profiles |
| AI driver | `MaszynaAIDriver`, named `maszyna`: the original AI driver |
| Scenery provider | `MaszynaSceneryProvider`, named `maszyna`: compiles and loads `.scn` sceneries |
| Streaming provider | `MaszynaSBTTerrainProvider`: the terrain from `.sbt` region files |
| Streaming consumer | `MaszynaSceneryChunkRenderingServer`: draws the terrain |
| Signalling | `MaszynaSignallingImplementation`: the original signals, controlled by events |
| Scenario events | `MaszynaScenarioEventImplementation`, the original event types as actions (`MaszynaMemoryAction`, `MaszynaSwitchAction`, `MaszynaLightsAction`, ...) and `MaszynaEventCondition` |
| Scenario scripts | memory cells and `eu07.events` (see [Lua scripts](lua-scripting.html)) |
| Model backend | `MaszynaE3DRenderingServer`, named `maszyna`: E3D and T3D models |
| Track backend | `MaszynaTrackRenderingBackend`, named `maszyna`, for `MaszynaTrackData` |
| Traction backend | `MaszynaTractionRenderingBackend`, named `maszyna` |
| Materials | `MaszynaMaterialManager`: `.mat` files and their textures |
| Replaced `RailVehicleRenderingServer` | `MaszynaE3DRailVehicleRenderingServer`: draws a vehicle from its E3D models |
| Replaced `TractionServer` | `MaszynaTractionServer`: computes the power network like the original |
| Cab logic | `MaszynaCabinLogic` (`addons/libmaszyna/cabin`) and the cab scene `MaszynaDynamicTrainCabin` |
| Translations and game data | `MaszynaGameDirectory`: the game directory, its `lang/` catalogues, reloading data when the directory changes |

The demo project selects these implementations in `project.godot`. The settings
`reloaded/models/backend`, `reloaded/tracks/backend`, `reloaded/traction/backend` and
`reloaded/vehicles/provider` are set to `maszyna`.

## Inside the port

* **Vehicles.** A `.fiz` file is parsed into a vehicle description (`addons/libmaszyna/fiz`). The
  vehicle is simulated by the original MOVER code, which is never modified. See
  [Wrapping the MOVER](wrapping-mover.html). `MaszynaVehicleSystem.vehicle_create()` creates a
  vehicle from a scenery's `dynamic` entry.
* **Cabs.** A vehicle's MMD file is read into its cab (`addons/libmaszyna/mmd`: `MmdCabinDefinition`,
  `MmdCabinInstancer` and the catalogue of the original controls). The cab logic is
  `MaszynaCabinLogic`, running on the core's `CabinSystem`. The cab screens run the original Python
  scripts in `maszyna-python-host` (`PythonScreenServer`). The vehicle's sounds are built from the MMD file
  into gnd-sfx banks that carry the original's formulas (`MmdSoundBankInstancer`, `MmdSoundFormulas`),
  fed the vehicle's quantities by `MaszynaVehicleSoundServer`.
* **Models and materials.** `MaszynaE3DParser` reads E3D files and `MaszynaE3DRenderingServer` draws
  them. `MaszynaMaterialManager` reads `.mat` files and textures (`addons/libmaszyna/e3d`,
  `addons/libmaszyna/materials`).
* **Sceneries.** The `.scn` parsers (`src/maszyna/parsers`, `addons/libmaszyna/scenery`) compile a
  scenery into the core's data: tracks, traction, models and trainsets. They also build its
  scenario: events (`maszyna_event_factory.gd`), memory cells, signals and the Lua scripts it
  declares.
* **Data paths.** `MaszynaGameDirectory` knows where MaSzyna's data is. Every reader resolves paths
  through it.

Behaviour of the original engine that the port reproduces on purpose is listed in
`MASZYNA_ORIGINAL_QUIRKS.md`.
