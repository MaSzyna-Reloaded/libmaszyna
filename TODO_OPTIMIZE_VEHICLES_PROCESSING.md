# Vehicles processing - what to optimize after the separation

The plan for the step after `dev/separate-libmaszyna`: what costs the main thread on the heaviest
scenery we have and is wrong by design, not merely slow - the cab layer instantiated per vehicle.
The measurements behind it are in `CANDIDATES_TO_CPP_PORT.md` (MaSzyna-Reloaded/reports#49) and
libreloaded's `docs/findings-archive.md` (2026-10-03).

Fixed by the operator (2026-10-25): the AI keeps driving through the cab (`CabinSystem.act()`) and
every driven vehicle keeps its cab logic. What is wrong is how the cab layer is instantiated.

## One cab logic, not one per vehicle

Today every vehicle with a cab holds its own `MaszynaCabinLogic`. Whenever its driver sits down
(`CabinSystem._on_vehicle_driver_cabin_changed()`, `vehicle_attach_cab_logic()`) it parses the MMD
again (`MaszynaCabinControls.from_mmd()`), makes about 25 behaviour objects and registers dozens
of callables per cabin (`cabin_logic.gd:66-170`); `main_switch.gd` and `door_permits.gd` add a
process each that `CabinSystem._on_simulation_advanced()` calls on every advance. On Wrzosy: 309
cabs, 618 calls an advance, ~2.3 ms a frame - for cabs where nothing happens.

The aim, with the behaviour and the tests unchanged:

* every behaviour (`MaszynaCabinMainSwitch`, `MaszynaCabinDoorPermits`, ...) is one instance,
  registered once in `CabinSystem`, and works on the cabin it is handed (its RID and
  `CabinState`) - the way Godot's servers take a handle;
* what a cab has (button types, which controls exist) is worked out once per MMD file and shared,
  not per vehicle and per sitting down;
* a step visits only the cabs where something runs: the line breaker's relay while its closing
  control is held, a door permit's timer while its push switch is held. A cab joins that list on
  the press and leaves it when done - never all cabs on every advance;
* the cabin of the 3D view keeps its display processes (`register_display_process()`), visited by
  the shown vehicle, not found by scanning every cab.

`CabinSystem` (libreloaded) and the legacy cab (`addons/libmaszyna/cabin/`) both change. This
replaces porting the cab loop to C++ (`CANDIDATES_TO_CPP_PORT.md`, section 2).
