# Quirks of the original engine and its data

What the original (`~/src/maszyna`) or the game data does wrong, oddly or not at all, found while
porting. Each entry: where it is, what it does, and what the wrapper does instead. The rule for
all of them is in `AGENTS.md`: port the behaviour, not the quirk - and where a quirk must be kept
for the data to work, it lives in a factory or a `MaszynaLegacy*` class, never in a core
interface.

## Timetable (`world/mtable.cpp`)

* **`IsMaintenance()` is never true.** `is_maintenance` is tested on the token that ends the
  facilities column - the track count, `1` or `2` - so `contains(s, "pt")` never holds
  (`mtable.cpp:486`). Wrapper: not ported.
* **100 km/h without a timetable.** `NewName()` sets `TTVmax = 100` with the comment "wykasowac"
  (`mtable.cpp:224`), so a train with `Timetable:none` is capped at 100. Wrapper: no timetable is
  no limit (`MaszynaLegacyDriverTimetable`).
* **The first station is reached through a fake name.** `NewName()` sets `NextStationName =
  "nowhere"`, and the `Timetable:` command then calls `UpdateMTable(..., "nowhere")` so that it
  matches and moves on to the first station (`Driver.cpp:4533`). Wrapper: `take()` names the
  first station directly.
* **`DirectionChange()` tests `StationIndex > 0`**, which is always true once a timetable is taken
  (`mtable.cpp:634`).

## AI driver (`vehicle/Driver.cpp`)

* **The W4 name is compared over `sizeof(std::string)` characters.**
  `compare(19, sizeof(asNextStop), ...)` uses the size of the string object (32 bytes on 64 bit),
  not the length of the name (`Driver.cpp:1101`). Wrapper: the whole name, case-insensitive.
* **Dead code in the speed table.** The branch that lets a standing train draw up to a W4
  (`Driver.cpp:918-925`) needs the W4's `fVelNext >= 0`, but `TableUpdateStopPoint()` has already
  set it to -1 for a standing train outside the stop. Wrapper: not ported.
* **The obstacle distance jumps at 100 m.** Beyond 100 m `scan_obstacles()` keeps
  `find_vehicle()`'s distance between the vehicles' positions on the track; nearer it switches to
  the distance between couplers (`Driver.cpp:6674-6677`). Wrapper: the gap between the ends at
  every distance (`RailVehicleServer.vehicle_find_vehicle()`).
* **The vehicle ahead is assumed to run the same way.** `adjust_desired_speed_for_obstacles()`
  compares our speed with the other's `Vel`, which has no sign (`Driver.cpp:7416`). Kept as is.
* **`find_vehicle()`'s range only limits which tracks are entered** - a vehicle on a track that
  begins within the range is found however far along that track it stands (`DynObj.cpp:7705`).
  Kept as is.
* **Bank leaves the distances of the previous order.** `determine_proximity_ranges()` has
  `case Bank: break;` with a TODO (`Driver.cpp:6803`). Kept as is.
* **Two writers of the brake handle while uncoupling.** `trainbrakeapply` sets
  `BrakeCtrlPosition = 3` with the comment that moving the handle elsewhere "should be switched
  off" (`driverhints.cpp:812`); it is not - `control_braking_force()` keeps running.
* **The obstacle check for a road vehicle and a train are one function** with `CategoryFlag`
  branches through every rule. Wrapper: rail only.
* **Uncoupling leaves the wagons' releasers pulled.** `UpdateDisconnect()` calls
  `BrakeReleaser(1)` on every vehicle it presses and never lets go (`Driver.cpp:7140`). Kept as
  is (the wrapper sends the same `brake_releaser`).
* **A coupler number can ask for what no shunter joins.** `Shunt <n> <coupler>` takes the raw
  `coupling::` bits, so it can ask for high voltage or a power line; `Attach()` sets them
  (`Driver.cpp:7021`), a player's crew cannot. Wrapper: only the elements a shunter joins count.
* **The front engine counts twice.** `CheckVehicles()` starts `ControlledEnginesCount` at 1 for a
  powered front vehicle, then counts again every powered vehicle under control that is not the
  driver's own - the front one among them when the driver sits elsewhere (`Driver.cpp:2470-2489`).
  Wrapper: every engine under control once (`MaszynaLegacyDriverTrainset._read_control()`).
* **A train is a goods train when its driver's locomotive is set to G.** `IsCargoTrain` is the
  occupied vehicle's own `BrakeDelayFlag & bdelay_G` (`Driver.cpp:2302-2304`), and a player's
  locomotive is never re-set (`Driver.cpp:2194`). A locomotive with `BrakeDelays=GP` starts at G
  (`Mover.cpp:12040-12042`), so a passenger train behind it (Galicja, Os33733 behind the SU42) is
  taken for a goods train once the order becomes Obey_train: it keeps further from obstacles and
  leaves every stop without waiting for the departure time (`Driver.cpp:1297`). Wrapper: the type
  comes from the cars (`RailVehicleServer.trainset_determine_type()`, by their `BrakeDelays` as
  `AutoRewident()` counts them), determined where `AutoRewident()` runs.
* **A series motor's power is "taken off" when it is not.** `DecMainCtrl(2)` of an electric series
  motor returns true whatever it did (`Mover.cpp:2616-2622`), so `ZeroSpeed()`'s loop ends only
  on the next position check. Wrapper: true only when the controller moved - a controller the
  vehicle refuses to turn back would otherwise hang the driver's update.
* **Releasing the EP brake switches it on.** `DecBrake()` of a handle whose EP holding equals its
  releasing calls `SwitchEPBrake(1)` (`Driver.cpp:3326-3328`), the same as `IncBrake()`. Kept as
  is.
* **`std::clamp()` with a lower bound over the upper one.** The coupler limit clamps
  `MaxAcc` between `HeavyCargoTrainAcceleration` (0.1) and `AccPreferred`, which falls to -0.3
  near a vehicle ahead (`Driver.cpp:7788`) - undefined behaviour. Wrapper: `clampf()`, the lower
  bound wins.
* **SN61 would not start without the AI's hack.** `dizel_StartupCheck()` refuses a plain diesel
  whose master controller stands where the throttle table gives no fuel (`RList[].R == 0`,
  `Mover.cpp:7841`) and drops the start-up; SN61's position 0 is such one. The AI gets round it
  with a special case, "specjalnie dla SN61 żeby nie zgasł" - the controller up to the first
  position with the clutch in, before closing the circuit and after setting the reverser
  (`Driver.cpp:2840-2843, 5778-5784`). Wrapper: `MaszynaLegacyDriverHints.set_idle()` before the
  line breaker, as the original; the reverser's copy is not ported.
* **One cab, three vehicles, two ways to reach them.** An EMU's cab car (EN57's `ra`/`rb`, the
  control cars with `EngineType` none) has no engine, no pantographs and no battery of its own:
  the motor car `s` has the first two, the other cab car `rb` the battery (`BatteryStart=Disabled`
  in `ra` and `s`, `misc/fiz_brakes_*_oerlikon.inc`). The cab acts on three `TMoverParameters` at
  once - `mvOccupied` (the car it is in), `mvControlled` (`FindPowered()`: 83 `OnCommand_*`
  handlers go straight there - line breaker, converter, compressor, controllers, reverser,
  pumps) and `mvPantographUnit` (`FindPantographCarrier()`) - while other commands go to the cab
  car and reach the rest through `SendCtrlToNext()` along the couplers (the battery, the brakes).
  The AI keeps the same three (`mvOccupied`, `mvControlling`, `mvPantographUnit`) and checks each
  step on whichever of them holds the device. Which command takes which path is written nowhere
  but in each handler. Wrapper: the driver reads the three (`MaszynaLegacyDriverTrainset`); the
  cab's own `mvControlled` - see TODO.md.
* **9.81 means "no blended brake".** `IncBrakeEIM()`/`DecBrakeEIM()` test `MED_amax != 9.81`
  (the struct's default) to tell a vehicle with the blended EP/ED brake from one without
  (`Driver.cpp:3208, 3373`), and brake with the driver's own hard-coded `fMedAmax` = 0.8 instead of
  the vehicle's (`Driver.h:377`). Kept as is (`NO_MED_DECELERATION`, `EIM_MAX_DECELERATION`).
* **IncBrake() divides by `ActualProximityDist` unguarded** in the DMU's stronger braking
  (`fBrakeDist / ActualProximityDist`, `Driver.cpp:3140`). Kept as is.
* **The wait at a stop is a negative timer.** The station's passenger exchange sets
  `fStopTime` to minus the longest exchange (`WaitingSet()`, `Driver.cpp:2652`), the timer counts
  up (`Driver.cpp:5910`), `check_load_exchange()` pushes it back down while any car still
  exchanges (`Driver.cpp:6767-6781`), a speed above 2 km/h resets it (the "force timer reset" HACK,
  `Driver.cpp:7449-7452`) and `VelDesired` is 0 while it is negative (`Driver.cpp:7454-7457`); the
  same field is the wait of `Wait_for_orders` and `Shunt`. The doors are closed in `Doors(false)`,
  called from the tractive force code before adding power (`Driver.cpp:7949`). Wrapper: not
  ported as such - `StationServer` keeps the dispatch as steps (exchange, wait for the departure,
  doors closed), each over on the cars' own events, and the driver stands while there is one.
* **A pantograph car's line breaker.** The data gives an EMU's car without traction motors a
  power of its own (36WE B, `36wea-b_kd.fiz`: `PWR=2`, `EnginePower=CurrentCollector`) - without
  one its current collector would not count (`LoadFIZ_Power`). The driver's readiness then counts
  its `Mains` among the consist's line breakers (`Power > 0.01`, `IsAnyLineBreakerOpen`,
  `Driver.cpp:6143-6144`), and the original closes it like any other: `MainSwitch_()` sets
  `Mains` on any vehicle with a master controller (`MainCtrlPosNo > 0`, `Mover.cpp:3621-3645`),
  engine or not. Wrapper: the line breaker is the engine's (`RailVehicleEngine`); a car without
  an engine has none and is not counted (`MaszynaLegacyDriverTrainset`) - counted, it stood open
  for ever and the driver never took the unit ready.

## Scenario events (`world/Event.cpp`, `world/EvLaunch.cpp`)

* `whois` dereferences a null activator.
* An unknown token hangs the `updatevalues` parser.
* A second trigger of a delayed event already queued is dropped with its activator, so a
  repeated delayed event never runs a second time.
* An event queued with delay 0 runs only on the next frame (the comparison is a strict `<`).
  Wrapper: in the same pass.
* An event's passivity is decided at load time, from the command it has then.
* `message` does nothing.
* `traintriggered` computes a scaled radius and then compares against the unscaled one.
* A duplicate event name makes the first definition `m_ignored` and the second its sibling
  (`Event.cpp:2296-2349`). Wrapper: the factory joins them; the server keeps the later name.
* **Setting the clock by hand can break a scenario.** A time-of-day launcher fires only in its
  exact minute (`EvLaunch.cpp:197-211`), so one jumped over never fires, and the timetables
  compare departures with the clock, so every train is late or early by the jump. Kept as is;
  the event queue and the drivers run on the simulation time, which a jump does not touch.

## Scenery model lights (`model/AnimModel.cpp`)

* **Blinking stops while the model is not drawn.** `RaAnimate()` advances `m_lighttimers` and is
  called only by the renderer as it draws the instance (`opengl33renderer.cpp:2914`,
  `openglrenderer.cpp:2396`), so a culled semaphore's blinking pauses and resumes out of phase.
  Wrapper: every blinking light runs on one clock (`E3DRenderingServer::_process_lights()`),
  drawn or not.
* **A negative dark/home value is never lit.** The mode is taken from `|value|`, the threshold from
  the signed value - `lsLights[i] - ls_Dark` (`AnimModel.cpp:601`, `:611` for `ls_Home`) - so
  `-3.4` gives a threshold of -6.4. Wrapper: the fraction of `|value|` (`LegacyLightMode`).
* **A model hides different submodels depending on who loads it.** `TSubModel::Load()` and
  `BinInit()` hide a `*_on` submodel only when the model is loaded as `dynamic` - a vehicle, its
  cab, its load - and in any other model hide `Light_On*` alone, compared with its letter case
  (Model3d.cpp:275, 2221). The same file thus shows its `_on` controls in the scenery and hides
  them on a vehicle; a shunting dwarf (`sem/karzelki/ktmnb`) keeps its lamps under a transform
  named `_on`. Wrapper: the parser marks `*_on`/`*_xon` `dynamic_hidden`, and only an
  `INSTANCE_KIND_DYNAMIC` instance hides them; `light_on*` is hidden whatever its case.
* **Only the first `Light_OnNN` of a name is switched** (GetFromName(), AnimModel.cpp:306); a copy
  nested under it shows with it (`ktmnb` keeps each lens under a transform of the same name).
  Wrapper: the same - a nested copy stays visible, any other copy stays hidden.
* **`ls_winter` (5) is declared and never handled** (`AnimModel.h:34`): `RaPrepare()` has no case
  for it, so such a light stays as it was. Wrapper: parsed as off.

## Mover (`src/legacy/maszyna-mover`, vendored)

* **A cab switched on sends itself only backwards.** `SendCtrlToNext()` picks the coupler from the
  sign of the cab (`d = (1 + Sign(dir)) / 2`, "wysyłanie tylko w tył"), and `CabActivisation()`
  does nothing when a cab is already active - so a cab switched on before the unit was coupled
  never reaches the other cab car, whose inactive cab (`InactiveCabFlag` emergencybrake) then
  vents the pipe for good. Wrapper: no cab switched on until the trainset is coupled.
* **An inactive cab car's alerter would brake the train.** A cab made active by the unit's master
  (`RunCommand("CabActivisation")`) counts as "just activated" for `TSecuritySystem::update()` -
  harmless only because the original never enabled that vehicle's alerter. Wrapper: the alerter is
  enabled by `CabActivisation()` alone, as there.
* **The brake handle has three positions and only one of them is compared.**
  `CheckLocomotiveParameters()` sets `BrakeCtrlPos` and `BrakeCtrlPosR` but not `fBrakeCtrlPos`,
  and `BrakeLevelSet()` returns early when `fBrakeCtrlPos` already equals the new position - so a
  vehicle set up a second time kept its FV4a handle at lap and never charged the pipe
  (`FINDINGS.md`, 2026-09-26). Wrapper: `fBrakeCtrlPos` synced before `BrakeLevelSet()` in
  `MoverRailVehicleController::initialize_mover_state()`.
* **`BrakeOpModes` defaults to a mode no FIZ asks for.** Wrapper: `BRAKE_OP_MODE_NONE` as the
  default, `pnep` parsed.
* **The spring brake reads its two valve areas crossed.** FIZ `ValveOnArea` goes into
  `SpringBrake.ValveOffArea` and `ValveOffArea` into `ValveOnArea` (`Mover.cpp:11025-11026`).
  Wrapper: the same crossing, in the FIZ parser, because the data is authored against it.
* **The spring brake's struct defaults describe a vehicle that has none.** `ShuttOff{true}`,
  `IsReady{false}` - and `LoadFIZ_SpringBrake` ends by overwriting all three
  (`Mover.cpp:11030-11032`). Port only the `extract_value` lines and every vehicle in the game
  starts with its spring brake shut off and braking, whatever the driver does.
* **One FIZ key, two destinations.** `MaxVoltage` is read into the engine's power source *and*
  into `collectorparameters.MaxV` (`Mover.cpp:11622`). Grepping for the first `extract_value` of a
  key and porting that one is how the E186's line breaker ended up tripping on any voltage above
  200 V.
* **A load is not always cargo.** `AssignLoad()` branches on the cargo's *name*, and `pantstate`
  is not a load at all (`Mover.cpp:8420`): the amount is read as a bitmask that raises pantographs
  and picks the vehicle's direction. That is how a scenery starts a locomotive with its
  pantographs up - written as a load, on a vehicle that carries nothing.
* **A vehicle nobody drives is not simulated.** `ComputeTotalForce()` integrates only while
  `CabActive != 0 || Vel > 0.0001 || |AccS| > 0.0001 || LastSwitchingTime < 5 || EZT || DMU`
  (`Mover.cpp:5008`), and switches the physics off otherwise, reporting nothing. "It does not
  move" is this before it is a bug.
* **The vendored sources need the original's precompiled header to compile correctly.** Every file
  of the original is built with `stdafx.h`, which includes `<stdlib.h>`; libstdc++ then puts
  `std::abs`'s overloads in the global namespace. Without it the unqualified `abs()` in
  `hamulce.cpp` (11 calls) is C's `int abs(int)`, so every difference below 1 bar truncates to
  zero and the control reservoir of `MHZ_6P`, `MHZ_EN57`, `MHZ_K5P`, `M394` and `St113` freezes.
  It compiles silently. Wrapper: CMake force-includes `stdlib.h` into `src/legacy/maszyna-mover/*.cpp`.
* **The pantographs' master valve opens by itself unless the FIZ says otherwise.** The struct
  default of every `basic_device` is `start_t::manual` (`McZapkie/MOVER.h:1323`), but
  `LoadFIZ_Cntrl` gives `PantsValve` a missing `PantEPValveStart` as automatic, "legacy code
  behaviour, there was no pantographs valve" (`McZapkie/Mover.cpp:10929`), while each
  pantograph's own valve stays manual. The E186 declares none of these keys, so in the original
  `P` alone raises its pantograph; porting only the keys that are present left the master valve
  shut, and the wrapper grew a workaround opening it on every raise. Wrapper: the loader's
  defaults in `RailVehicleElectricEngine`, the workaround removed.
* **The couplers depend on the frame rate.** `CouplerForce()` (`Mover.cpp:4779-4784`) takes a
  coupler's length as the distance set by the last refresh plus *ten times* the relative
  movement since (`dMoveLen`), and the original refreshes once a frame, before all its physics
  sub-steps (`DynObj.cpp:8691-8699`). The longer the frame, the stiffer and more wrongly loaded
  every coupler: at 60 fps nobody notices, at a slow frame or a faster simulation a long train
  locks up - the Stary Jawor eszelon, 21 vehicles, stood at 0.18 m/s with 391 kN at the wheels at
  0.17 s a frame, and ran to 14 m/s at 0.03 s. Wrapper: locations and neighbours refreshed before
  every sub-step (`MaszynaMoverVehicleServer::stepping_advance()`, `FINDINGS.md` 2026-09-27); the Mover untouched.
* **The FIZ loader is not in the vendored copy.** Ours is 9598 lines against the original's 12813
  and holds no `LoadFIZ_*` at all, so every quirk of how a FIZ key reaches a Mover field has to be
  read in `~/src/maszyna`, not in `src/legacy/maszyna-mover`.

* **`LoadFIZ_Engine` and `readMPT()` pick their keys by the engine type.** The clutch of a plain
  diesel (`minVelfullengage`, `engageDia`, `engageMaxForce`, `engagefriction`) is read from the
  header of `MotorParamTable:`, not from `Engine:` (`Mover.cpp:11394-11406`), and the rows of that
  same section are motor parameters of an electric motor or the gears of a diesel
  (`readMPTDieselEngine()`, `Mover.cpp:9175`: idx, mIsat, fi, mfi - the ratio, the lowest and the
  top speed of the gear).
* **A key given twice in one FIZ line: the first one counts.** `extract_value()` looks the key up
  with `find(" " + Key + "=")` (`utilities/utilities.h:170`), so a repeated key's later value is
  never read. The data relies on it: BR285's `Engine:` says `Vadd=5.5 Cr=1 Vadd=0.0 Cr=1.0`, and
  the original runs it with `Vadd = 5.5 / 3.6`. `Vadd = 0` turns the diesel-electric traction
  force into `1000 * 0 / (0 + 0)` (`Mover.cpp:5310`) the moment the line contactor closes with
  the vehicle standing and `tempPmax` still zero (the engine not up to speed, or `eimic` at zero),
  and the NaN then stays in `V`, `Vel`, the brakes and the wheels for good. Wrapper:
  `FizLineUtil.read_key_values()` keeps a key's first value too; it used to keep the last, and
  that was the BR285's NaN speed (`docs/findings-archive.md`, 2026-09-30).
* **A FIZ declares no cab, no horns and no radio.** The horns a vehicle has are implied only by its
  MMD's buttons and sounds (`horn_bt:`/`hornlow_bt:`/`hornhigh_bt:`/`whistle_bt:`, `horn1:`-`horn3:`;
  `Train.cpp` `OnCommand_horn*activate`, `DynObj.cpp` `WarningSignal`), the radio is the cab's
  (TTrain's channel and volume), and only an MMD makes a cab. What a FIZ has is `Cntrl.` - on every
  vehicle, a wagon too, because it carries the brake keys (`BrakeSystem=`, `BrakeDelays=`,
  `MaxBPMass=`) - and 239 wagons of the data, pedestrians and a bicycle among the "vehicles", write
  `MCPN=1` there (`zssk/lgs_v1/lgs.fiz`, `road/men/man.fiz`). The original takes any
  `MainCtrlPosNo > 0` for "has steering" (`Mover.cpp:712, 2380, 3258`; `Driver.cpp:3412`), yet one
  position is nothing to turn and `IncMainCtrl()` needs an active cab (`Mover.cpp:2380`) that only
  an MMD gives. Wrapper: the FIZ factory makes a master controller only for `MCPN > 1`
  (`FizTrainCntrlParser.MASTER_CONTROLLER_MIN_POSITIONS`), and the horns and the radio only for a
  vehicle that has one; a wagon gets none of the three (`docs/findings-archive.md`, 2026-10-04).
* **A FIZ declares no battery of its own.** Every Mover has a battery and the low-voltage circuits;
  a vehicle without one simply has `NominalBatteryVoltage` 0, and the battery logic is gated on it
  (`Mover.cpp:941`). What says a vehicle has a low voltage at all is `Light:`'s `LMaxVoltage` (the
  battery's nominal voltage) or `Cntrl.`'s `BatteryStart`/`ConverterStart`/`ConverterStartDelay`.
  Wrapper: the FIZ factory makes a power supply only for a vehicle with one of these
  (`FizTrainPowerSupplyParser`); a vehicle with none - a freight wagon - has no power supply
  component, which stands for the original's zero nominal voltage.

## Pantograph geometry (`vehicle/DynObj.cpp`)

* **A pantograph's arms are measured from the vehicle's model, not read from its FIZ.** The original
  gives every pantograph the dimensions of its type (`PantType=`, AKP_4E by default,
  `DynObj.cpp:90-194`) and then overrides them with what it measures off the model's lower arm, upper
  arm and slider (`DynObj.cpp:5404-5480`); `pantfactors:` of the MMD places a slider the model cannot
  be measured by (`DynObj.cpp:5577-5633`). No FIZ of the game data names `PantType=`. Wrapper: a FIZ
  that names it takes that type's dimensions; one that does not - every FIZ today - has its arms
  measured from the model as the original does, until the FIZ files name their pantographs
  (`RailVehicleRenderingServer::_publish_pantograph_geometry`).

## Cab definitions (MMD data)

* `radiocall3_sw { radio_3 ... }` (E186, `base.mmd.inc`) has lost its colon; the original walks
  over tokens it does not know, a stricter parser reads every following block from the wrong end
  (`docs/findings-archive.md`, 2026-09-25).
* `brakeopmode_sw` and `doormode_sw` have no trailing colon in the original's caption table and
  never get a caption.
* The SM42 6D cab has no `mainctrl` - its master controller is `jointctrl`. Anything that
  commands "the master controller" by name must fall back to it
  (`MaszynaLegacyDriverHints.master_controller()`).
* **What a cab command does depends on the gauge's `type:` in the MMD.** `Train.cpp` handlers
  branch on `ggX.type()` (`rg -o 'gg\w+\.type\(\)' vehicle/Train.cpp` lists them): the same key
  flips a two-state `main_sw`, but an impulse one acts only on release; a push pump runs while held,
  a toggle one sets `*SwitchOff`. A switch without `type:` is a toggle (`Gauge.h:89`). Wrapper: the
  MMD factory maps the type onto the widget, the branching lives in `legacy_cabin/` behaviours.
* **A vehicle's own MMD takes parameters.** The original never opens it directly: it parses the
  text `include <TypeName>.mmd <name> <TypeName> <skin> end` (`DynObj.cpp:5260`), so the MMD's
  `(p1)` is the vehicle's scenery name, `(p2)` its type name and `(p3)` its skin. The data relies
  on it: SN61 has only `include sn61.mmd.inc (p2)` and the include's `models: (p1).t3d#` - read
  without the parameters the body model is `none` and only the low-poly interior is drawn;
  SM42 6D and PWM10 name their `attachments:` by `(p3)`, 4E (`4e-staraklima`) by `(p1)`. Wrapper:
  every reader of a vehicle's MMD takes `MmdCabinInstancer.vehicle_parameters()`; a structure read
  from an MMD that names `(p1)` is cached for its vehicle alone (`names_vehicle()`).
* **A comment eats the parameters of an include.** Six ST44/M62 MMDs pass theirs as
  `include st44.mmd.inc 2M62-0685-A // 2M62-0571-A end` (`2M62-0571-A/B`, `2M62-0662-A/B`,
  `2M62-0685-A/B`, `m62-1579`): the comment runs to the end of the line, `end` with it, and
  `st44-324.mmd` passes only one. The include's `attachments: { components/decals/(p2).t3d }` is
  then `components/decals/none`, which `cParser` gives any parameter not passed (`parser.cpp:290`).
  The original fails to find the model, logs "Bad file: failed to locate 3d model file" once and
  remembers the miss (`TModelsManager::GetModel()`, `MdlMngr.cpp`). Wrapper: the same miss, warned
  by `E3DModelManager.load_model()` for every vehicle.

* **Every vehicle has three cab positions, whatever its MMD defines.** The Mover's `CabOccupied`
  is -1, 0 or 1 for any vehicle: a scenery `headdriver`/`reardriver` sets it to ±1 straight from
  the `.scn` (`DynObj.cpp:1994-2019`), and `create_controller()` makes the driver (`TController`)
  whether or not the MMD has a `cab1definition:`/`cab2definition:` (`DynObj.cpp:2604-2627`) - the
  cab definition matters only to the player's `TTrain` (`InitializeCab()`, `Train.cpp:10481`).
  A cab change steps through all three positions the same way and fails only after moving onto a
  missing one (`TTrain::CabChange()`, `Train.cpp:10324-10351`). Wrapper: a rail vehicle has the
  cabins its MMD defines a cab for (`MaszynaVehicleStructure.cabin_kinds`), no more. For the data
  that puts a driver at an end without a cab definition, `MaszynaLegacyVehicleSystem._build()`
  adds that end's cabin - only for a vehicle that can have a cab, one with a master controller
  (`FizTrainCntrlParser`, `MCPN` > 1). A wagon given a driver stays without one (a warning): the
  original's AI only moves to vehicles under its control (`FirstFind(dir, coupling::control)`,
  `Driver.cpp:2079`) and works a wagon's doors and lights through the trainset, never from a cab.
* **A cab change stops on a cab position without a cab, and the gangway leads into one.**
  `TTrain::CabChange()` moves `CabOccupied` first and only then looks for the MMD's definition
  (`Train.cpp:10334-10349`): past a missing `cab0definition:` the driver stands in no cab, and one
  more press goes on. Through a gangway the neighbour is entered by the cab position facing it,
  `CabOccupied = ±1`, whether it has a cab or not (`Train.cpp:8301-8306`) - an EMU's middle car
  is entered with no cab view at all. After the change the handler applies two of its own
  "HACK"s: the door permit preset (`ChangeDoorPermitPreset(0)`) and the lights (`SetLights()`)
  (`Train.cpp:8309-8318`). Wrapper (`RailVehicleServer.person_change_cabin()`): a person always
  sits in a cabin - a position without one is passed by, and through the gangways the person
  goes on to the nearest vehicle that has a cabin, entering it from the side it came from. The two
  hacks are not ported.
* **A cab change while the AI drives moves the AI's cab with the player's view.** The player and
  the AI driver share one `TTrain`: with the AI at the controls Home/End only shifts the vehicle's
  `CabOccupied` by one (`TTrain::CabChange()`, Train.cpp:10326-10333), so the vehicle counts its
  driver in whichever cab the player looks from. Wrapper: the player and the driver are persons of
  their own - only the player's person goes over (`RailVehicleServer.person_change_cabin()`), the
  AI keeps its cabin and the vehicle answers to it.

## Scenery data

* **A trainset for the player is told apart only by a dash.** Every trainset of a scenario is
  loaded, AI trains and decorations alike, and an AI train has a `headdriver` like the player's;
  what the original's Starter offers is the trainsets whose mission description `//$o` does not
  begin with "-" - a convention of the data, written down nowhere in it and not followed by the
  original's own launcher, which lists every trainset (`launcher/scenery_list.cpp`). 1467 of the
  data's 1938 trainsets are marked so; `calkowo_tartak2.scn` declares 19 and offers one,
  6Dg-1248, as the Starter shows it, and every scenario offers at least one. Wrapper: the scenery
  selector lists the offered occupied trainsets (`MaszynaSceneryInfo.Trainset.is_offered()`), and
  every other one under its "all trainsets" check box.
* **A model's path has two roots.** A `node ... model` names its file either from the game
  directory (`models\linia053\peron_sandomierz.t3d` - Sandomierz's platform, `l053_tri.scm:59325`;
  `dynamic\pkp\...` for a vehicle standing as scenery) or from `models/` (`bud\dombale.t3d`). The
  original tries, for `.e3d` and then `.t3d`, the path as given and then `models/` + the path
  (`TModelsManager::find_on_disk()`, `MdlMngr.cpp:146-150`), and its scenery export strips a
  leading `models/` again (`AnimModel.cpp:760`). Wrapper: the same order
  (`maszyna_node_model_importer.gd`); it used to put `models/` in front of everything but
  `dynamic`, and lost every model named from the game directory.
* **A W4's name carries a unique suffix after `#`** (`JAWOR#1` ... `JAWOR#5`) that the timetable
  does not; the original's `putvalues` parser cuts it (`Event.cpp:720`). Wrapper: the event
  factory cuts it.
* **Stary Jawor, `eszelon`:** three trainsets name the timetable `rozklad`, which is an empty file,
  with a velocity of 0.1 (wait for a signal); the train is set going by a memory cell instead.
* **Stary Jawor, `osobowy1`:** `mps74142` and `mps47141` both stand at JAWOR 16:00-16:02 in
  their timetables, but the scenario places them some kilometres out; they arrive half an hour
  late and leave at once.
* **A scenery without a `time` section runs at 10:30.** `scenario_time` starts at 10:30
  (`simulation/simulationtime.h:21`) and `Time.init()` takes it after the load
  (`simulationtime.cpp:30-51`), whatever the timetables say. Zwierzyniec, `zwierzyniec_osob.scn`,
  has none (its RPE58102 is due at Pawianowo at 10:37); `zwierzyniec_posp.scn` sets 15:30.
  Wrapper: `MaszynaSceneryNode.START_TIME_DEFAULT`.
* **File names in the data ignore letter case.** The datapack is made on Windows: the scenery
  says `PKP\SN61_V2 SN61-179 SN61_v2` and `2M62-0571-A`, the files on disk are
  `pkp/sn61_v2/sn61_v2.mmd` - or, the other way round, uppercase `2M62-0571-A.fiz` exist as
  written, and 821 names on disk carry capitals. Wrapper: `MaszynaDataPath.resolve()` keeps the
  base directory, tries the relative path as authored, then its lowercase form
  (`docs/findings-archive.md`, 2026-10-02, 2026-10-10). Any other difference in case is not found
  on Linux: `przejazdy/plyty3_l.t3d` (`plyty3_L.e3d`) and `slupy_nn_400kv_*_atlas`
  (`slupy_nn_400kV_*_atlas`) of l053 and l204.
* **`include none`.** `l204/deko/204_trawky_ter.scm:33698` and
  `linia053_wrzosy/scm_wrzosy/1-tory.scm:51026` include a file named `none`. The original opens
  it, logs "Failed to open file" (`parser.cpp:89`) and goes on with an empty include. Wrapper: the
  same error (`MaszynaIncludeImporter`), and the file is then not cached
  (`context.cacheable = false`), so it is parsed again on every load.
* **Terrain that exists only as an SBT.** A scenery's shapes may come in a binary region file:
  `<scenario>.sbt` beside the `.scn`, or one named by a `terrain x.sbt endterrain` line
  (`simulationstateserializer.cpp:51-66`, `:786-799`). With one, the original reads every
  `triangles` shape from it (`basic_region::deserialize()`, `scene.cpp:1180`) and leaves out the
  scenery's own `triangles` nodes, its terrain models (`range_min` below 0) and every include whose
  name contains `_ter.scm` (`parser.cpp:330`, "SBT found, ignoring"); without one it parses them and
  writes the SBT itself (`scene.cpp:1141`) - so most `.sbt` in a game directory are the original's
  own cache (12 of 15 here postdate the datapack). The flag is global and set where the file is
  read, so it holds for everything parsed after it, in the including file too. Two ship with the
  datapack as the only copy of their terrain: `braniewo_szeroki.sbt` (1.2 GB) holds six `_ter.scm`
  that `l254/` includes and the datapack lacks (`deko/254_placki_ter.scm`, `254_roslinky_ter.scm`,
  `254_wielkopow_ter.scm`, `teren/fro_ter.scm`, `suh_ter.scm`, `tol_ter.scm`), and `l107/l107.scm`
  names `terrain l107/l107_teren.sbt` (955 MB) on its third line - after `l107_deko.scm`, before
  `l107_ziel.scm` with its `deko/107_*_ter.scm` and before `nmt100_podkarpackie_ter.scm`, which
  `linia_107_*.scn` include next; the original leaves all three out. A `terrain` line sets the
  include flag even when its file is missing; the shapes are left out only when the file is a
  region file. Wrapper: the two flags in `MaszynaImporterContext` (`binary_terrain`,
  `binary_terrain_state`), which `pop_state()` keeps; the region file is not read at load but
  supplied section by section as the camera comes near (`MaszynaLegacySBTTerrainProvider`, a
  `SceneryStreamingProvider` - a section is the streaming's own 1 km cell, its radius in the header
  says how far its shapes reach out of it). A small include is parsed
  in place, so a `terrain` line in it reaches the rest of the including file as the original's
  does; one in an include large enough to be parsed as a task of its own
  (`SceneryInstancer.INLINE_INCLUDE_MAX_SIZE`) reaches only what it includes itself - the datapack
  has none. The region file's lines are left out, as `lines` nodes are.
* A timetable file saved as UTF-8 rather than cp1250 keeps mangled Polish letters in its labels
  (`linia053/scenariusz_os`).
* **A load count with no type behind it is not a load.** The `dynamic` line gives the count
  first and the cargo's name only when the count is non-zero, and the original zeroes both on the
  spot - the comment is "idiotoodporność" (`simulation/simulationstateserializer.cpp:1032`).
  Reading the next token unconditionally eats `enddynamic` and desynchronises the rest of the node.
* **`offset == -1.0` is a sentinel, not an offset.** It means "reversed in the trainset"
  (`simulation/simulationstateserializer.cpp:1061`, `:1068`).
* **A double slip is not a `cross`.** `track cross` is a *road* intersection in the original
  (`world/Track.cpp:420`, `:425`, `iCategoryFlag = 2`); a double slip is four `track switch` nodes
  named `..._a/_b/_c/_d` plus four short connectors, and the two branch ends of each quarter sit
  about 0.2 m apart - inside any tolerance that looks "safe".
* **`none` is a material sentinel.** The parser reads it as no material at all
  (`world/Track.cpp:487`, `:498`) and draws no trackbed; the short connectors inside a switch group
  rely on that and borrow their trackbed from a neighbour
  (`TTrack::copy_adjacent_trackbed_material()`, `world/Track.cpp:3328`).
* **Traction resistivity is Ohm/km in the data and Ohm/m a line later.** `fResistivity *= 0.001f;
  // teraz [om/m]` (`world/Traction.cpp:112`), and a declared `0.01` is read as the default 0.075.
  Multiplying the raw token by a length in metres gives a line a hundred times too resistive,
  which only a vehicle actually drawing current reveals.
* **Every traction supply is declared twice under one name** - once `section`, once as a
  substation - and the name table keeps the **last** one on purpose:
  `mapping.first->second = itemhandle;` with the comment "update mapping to point to the new one,
  for backward compatibility" (`utilities/Names.h:38-39`). A port that took the first declaration
  would leave half the scenery unpowered.
* **`e186_v2/eu47.fiz` closes `WiperList:` with `endL` instead of `endwl`.** The original never
  closes the list either - it is `Size=` that bounds the switch (`vehicle/Train.cpp`,
  `OnCommand_wipers`/the wiper list parser).
* **`e186_v2/p160dc.fiz` never closes `LightsList:`** - there is no `endL`, and the next line is
  `WiperList:`. The original does not close it either: only `endL` clears `startLIGHTSLIST`
  (`McZapkie/Mover.cpp:9762`), the wiper rows win only because `startWiperList` is tested first
  (`McZapkie/Mover.cpp:10200-10213`), and once the wiper table ends every later line is handed to
  `readLightsList()` again. Wrapper: every section header ends the table that is open
  (`fiz_vehicle_builder.gd`), so the twelve presets are kept and nothing leaks into them.
* **A legacy sound value may join its files with commas.** The sound deserializer reads file
  names with `"\n\r\t ,;"` as delimiters (`audio/sound.cpp:105-111`), so
  `small-compressor: s-compressor-start.wav,s-compressor.wav,s-compressor-stop.wav 20` (36
  declarations in the datapack) is begin, main and end - it plays in the original. A tokenizer
  that splits only on white space reads it as one file name that does not exist. Wrapper:
  `MmdSoundSourceParser` splits such a value of a multipart sound.
* **Of 1 344 `airsound*` declarations in the whole datapack, exactly one sets `placement:`.** The
  rest fall back to `general`, which the interior/exterior attenuation model short-circuits to a
  constant - so the loudest brake sound in the game is outside that model entirely.

## Sound (`audio/audiorenderer.cpp`, `vehicle/DynObj.cpp`)

* **The original knows its running noise combs and works around it in one line.** Every vehicle of
  a trainset plays the same recording, and identical loops a few metres apart comb against each
  other. `audiorenderer.cpp` says so where it happens - "potentially adjust starting point of the
  last buffer (to reduce chance of reverb effect with multiple, looping copies playing)" - and
  `DynObj.cpp` starts each vehicle's `m_outernoise` at `Random(0.0, 80.0) * 0.01`. Two details
  that matter: it is a **fraction of the sample**, not a time, and it is drawn **once per
  vehicle**, so a vehicle keeps its own phase for good.
* **The pantograph "up" sound is keyed to the voltage, not to the arm.** `sPantUp` plays when
  `PantFrontVolt`/`PantRearVolt` goes from 0 to above 0 (`vehicle/DynObj.cpp:3881-3934`, with the
  original's own TODO to make it "a sound event for specific pantograph"), so raising a
  pantograph under a dead or missing wire is silent. `sPantDown` follows the pantograph's
  `is_active` instead (`vehicle/DynObj.cpp:4007-4036`). Wrapper: the same two conditions, reported
  as `RailVehicleElectricEngine.pantograph_up`/`pantograph_down`; which pantograph it was is not yet
  used for the sound's position (`TODO.md`).

## Cab Python screens (`pyscreen:`, the original's Python 2 scripts)

* **A missing key is a blank screen, not a missing value.** A script reads `state['hours']`
  directly, so a key the dictionary lacks raises `KeyError`, `render()` fails whole and nothing is
  drawn. The 145 scripts together read about 250 keys of `GetTrainState()`.
* **The scripts assume the game directory is the working directory** (`./fonts/`, `./textures/`,
  `from scripts import`), and importing a module writes a `.pyc` beside it - inside the game data.
