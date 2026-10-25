# Vehicles processing - what to optimize after the separation

The plan for the step after `dev/separate-libmaszyna`: what costs the main thread on the heaviest
scenery we have and is wrong by design, not merely slow. Two subjects - road cars handled as rail
vehicles, and the cab layer instantiated per vehicle. The measurements behind it are in
`CANDIDATES_TO_CPP_PORT.md` (MaSzyna-Reloaded/reports#49) and libreloaded's
`docs/findings-archive.md` (2026-10-03, 291 road cars: ~15 ms of physics and ~3 ms of drivers a
frame in a debug build).

Fixed by the operator (2026-10-25): the AI keeps driving through the cab (`CabinSystem.act()`) and
every driven vehicle keeps its cab logic. What is wrong is how the cab layer is instantiated, and
that a car is a rail vehicle.

## 1. What a road car is today

Wrzosy (`wrzosy_eie2620.scn`) has 285 road cars (and 6 sailboats), in `xx_auta.scm` (234) and
`xx_auta_l053.scm` (51). Every one of them goes the whole way of a train:

* a `VehicleServer` vehicle with a deep copy of its FIZ controller (`VehicleServer.cpp:282`), a
  `TMoverParameters`, a `RailVehicleServer` placement (two axle followers, two neighbour scans) and
  a trainset of one vehicle (`scenery_server.gd:230-245`);
* the whole Mover step: 0.01 s sub-steps, location and neighbours in every sub-step, forces,
  movement, components (`MaszynaMoverVehicleServer.cpp:91-174`);
* the cab logic read for every vehicle (`maszyna_vehicle_system.gd:192`), attached whenever the
  MMD has a cabin (`:282-283`), and a `MaszynaAIDriver` for its `headdriver`
  (`scenery_server.gd:251-255`) running every module of a train's driver.

### The category is known early enough

* `Param. Category=` of the FIZ becomes `VehicleController.category`, `CATEGORY_ROAD` for `road`
  and `unimog` (`fiz/fiz_train_controller_parser.gd:14-20, 62-67`; libreloaded
  `VehicleController.hpp:65-69`).
* The description is read on a worker (`maszyna_vehicle_system.gd:188`) before `_build()` calls
  `RailVehicleServer.vehicle_attach()` (`:221`) - the branch for a car goes there.
* The catalog knows it too (`ROAD_CAR/BUS/TRUCK`, `maszyna_vehicle_catalog_provider.gd:28-30`), but
  `VehicleData` carries no category, so a scenery `dynamic` has none.

What stands in the way:

* `FizVehicleBuilder.build_description_at()` always makes a `MaszynaMoverRailVehicleController`
  with the `Rail*` components (`fiz_vehicle_builder.gd:154-171, 215-216`);
* `MaszynaMoverVehicleServer` casts every controller it steps to `RailVehicleController`
  unchecked (`:139, :144`);
* libreloaded has no vehicle controller and no server that is not rail. `docs/architecture.md:33`
  names "a road vehicle" and nothing implements it.

### How the cars are driven on Wrzosy

No program of their own. Each is a `trainset none <road> 0 50` (10 of them 30) with one
`headdriver`; nothing in the scenery sends them a command, a timetable or a loop. They meet only:

* the level crossings' memcells (`SetVelocity 0 0`, `0.1 -1`, `-1 -1`,
  `scenery/pkp/_b14m11m_2s2d_2r60eeg5_spr.inc:26-68`), read by `event1`/`event2` of 41 road tracks;
* 12 speed limit memcells on the roads of L053 (`l053_drogi_dwschod.scm:35-54`).

What the original's driver does with them:

* no timetable: `Prepare_engine, Shunt`, the speed capped at `fShuntVelocity` 40 km/h
  (`Driver.cpp:5243-5248, 5316-5317, 7322-7327`);
* a car's own constants: braking factor 0.03, a gap of 5 m, `fVelPlus` 10 / `fVelMinus` 2
  (`Driver.cpp:1875-1881`), acceleration within +-2.0 (`:7395-7398`), approach distances
  (`:6787-6800`), obstacles searched 250 m ahead (`:6665, 6674`), 1-3 s before moving off
  (`:2737-2755`);
* oncoming cars ignored by their side offset (`DynObj.cpp:1614-1629`), the lane on the road
  (`DynObj.cpp:2310-2335`);
* the end of a road is 20 km/h, not a stop (`Driver.cpp:434-437, 907`); past it `TTrack::
  NullCreate` lays an invisible U-turn (`:loopstart`/`:loopfinish`, `Track.cpp:271-311`,
  `TrkFoll.cpp:79-84`) - the cars shuttle along their road for good;
* a `Dumb` engine: main controller then secondary up, secondary by two then main down, `SpeedSet`
  does nothing (`Driver.cpp:3570-3579, 3708-3711, 3964-3966`).

None of it is ported: our car stops at the end of its road, drives on the road's centre line and
with a train's constants (gap 50 m, +-0.9, 1000 m of obstacles; `maszyna_driver_speed.gd:26, 65,
192`, `maszyna_driver_route.gd:18-26, 464-471`).

### A wrong traction on top

`EngineType=Dumb` (Clio, Audi 80, Transit - most of `dynamic/road/`) is not built
(`fiz/fiz_train_engine_parser.gd:64-67`, a warning only): the car has no engine component, the
driver reads `NONE` (`maszyna_ai_driver.gd:808-812`) and takes the EN57 control car's traction
(`maszyna_driver_traction.gd:114-115`). In `TODO.md`, "Drivers".

## 2. Plan A - a car is a car

**libreloaded: a road vehicle beside the rail one.**

* A vehicle controller that is not rail (`RoadVehicleController : VehicleController`) and a
  `RoadVehicleServer` on the same `VehicleServer` RID, as `RailVehicleServer` is. It owns what a
  car has: its place on a road (`TrackServer`, `track_sample_position()`), its lane and side
  offset, following the vehicle ahead with a gap and an acceleration limit, the road's speed limit,
  what happens at the road's end.
* Stepped natively, one loop over the cars on `SimulationServer.simulation_advanced` - no Mover,
  no couplers, no trainset, no cabins, no sub-steps of 0.01 s.
* libreloadedpro's `ReloadedProRoadTrafficServer` (`src/traffic/`) already moves traffic this way
  (two lanes per road, time-headway following, junction exits from track connections), outside
  `VehicleServer`. Either the pattern or code to lift into the core - to be decided.

**libmaszyna: the branch at the build.**

* `MaszynaVehicleSystem._build()` takes a description with `CATEGORY_ROAD` to `RoadVehicleServer`
  instead of `RailVehicleServer.vehicle_attach()`; `FizVehicleBuilder` makes a road controller for
  it. No cab logic, no cabins, no Mover, no trainset of one.
* `SceneryServer.instantiate_trainsets()` does not put it into a `RailVehicleServer` trainset.

**libmaszyna: the car's driver (CarAIDriver).**

* In C++, a driver implementation for cars: one instance drives every car by its RID, no
  `MaszynaAIDriver` per car.
* The original's road rules above: Shunt at 40 km/h, the gap of 5 m, +-2.0 m/s^2, obstacles in
  250 m, oncoming cars by the side offset, `SetVelocity` of the crossings' and limits' memcells,
  20 km/h at the road's end and the `NullCreate` U-turn.
* None of a train's modules: the train brake, hints, doors, the security system, pantographs,
  headcodes (a car always has head lights and red markers, `Driver.cpp:6519-6522`).

**Open, for the operator before the work:**

* can the player take over a car (it has no cab to drive from today);
* a car's physics: the Mover's, or kinematics only;
* libreloadedpro's traffic code: lifted into the core, or only the pattern.

## 3. Plan B - one cab logic, not one per vehicle

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

## 4. Left over

* libreloaded `TODO.md:420` ("Road vehicles return with roads ... left out at load") looks stale -
  roads are imported (`maszyna_node_track_importer.gd:22-28`) and the 2026-10-03 measurement had
  291 cars built on them; to be checked and corrected there.
