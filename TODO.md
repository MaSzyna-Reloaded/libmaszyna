# TODO

Open work of libmaszyna. What is open in libreloaded, the core, is in its own `TODO.md` and
`LIBMASZYNA_SPLIT.md`; the game's in `maszyna-reloaded`'s `TODO.md`.

## Split from libreloaded

What libreloaded's `LIBMASZYNA_SPLIT.md` leaves open on libmaszyna's side (details there, by number):

* **LS-005** - libmaszyna is compiled into libreloaded's library (`plugin.cmake`,
  `LIBRELOADED_PLUGINS`); it becomes a library of its own with `reloaded_plugin_create()`, a
  manifest and its addon's `.pck`, once the core's plugin loader exists.
* **LS-048** - the addon packed into `libmaszyna.pck`; its autoloads made by the plugin, not
  added to the project by `libmaszyna.gd`.
* **LS-064** - `addons/libmaszyna/traction/maszyna_traction_rendering_backend.gd` to C++, as the
  track backend (`MaszynaTrackRenderingBackend`) already is, after libreloaded's
  `TractionRenderingServer`.
* **Found by the LS-023 classification** (libreloaded's `LIBMASZYNA_SPLIT_LS023.md` and
  `TODO.md`) - not changed, the operator decides: the FIZ `AirLeakRate` scaled by 0.01 twice
  (`fiz_train_brake_parser.gd`, `MaszynaMoverRailVehicleBrake.cpp`); config written to the Mover
  that the vendored Mover never reads; properties nothing reads; Mover start modes no FIZ sets; the
  `*_rpm` properties holding revolutions per second.
* **`test_zzz_machine_room_low_poly_interior.gd` is red since the import** of libmaszyna `652c2348`
  into libreloaded ("hi-fi cab 1 hides its low-poly counterpart", "camera should stay in the cabin
  in the machine room"); the change that turned it red is not found yet.
* **The track features table has only what the fixtures show**
  (`addons/libmaszyna/scenery/maszyna_track_features.tres`): `1435mm/tpb*` crushed stone,
  `asphalt*` asphalt, `*solidwhite*` a centre line. The operator fills in the rest of the game's
  textures.

## Past the core's extension points

Found while documenting what libmaszyna plugs into libreloaded (`docs/architecture.md`); each one
reaches past the core's extension points, and each is the operator's to decide (a public operation
in libreloaded, or a change here):

* `MaszynaSceneryProvider` and `maszyna_event_factory.gd` write `SceneryContentNode`'s private fields
  (`_signalling_system_rids`, `_model_rids`, `_provider_rids`, `_event_system_rids`, `_scenery_radio`,
  `_isolated_rids`, `_pickable_rids`, `_event_rids`) instead of handing them over by `content_own()`.
* `MaszynaMaterialManager` sets `ReloadedMaterialServer.material_set_provider()` and never clears it
  on leaving (the core's slot has no unset).
* `MaszynaPlugin` sets the process-wide `VehiclePhysicsNode::set_controller_implementation()` - a
  second vehicle plugin could not coexist.
* `libmaszyna.gd` declares a setting in the core's namespace, `reloaded/scenery/lights/distance`,
  read only by `MaszynaE3DRenderingServer`.
* `MaszynaSceneryProvider` is an autoload only of `demo/project.godot`, not of `libmaszyna.gd`, and
  it registers the driver (`MaszynaAIDriver`) and the traction backend in its `_enter_tree()` rather
  than the plugin; it depends on the core's `TractionRenderingServer` autoload being first.

## Split from the game

* `demo/examples/mover_demo.tscn` and `cabin_demo.tscn` point at `res://vehicles/sm42/*`, deleted in
  af72fa4c0 (`cabin_demo` also at the missing `environment/sky.gd`) - the simple Mover demo the
  demo is to keep does not load.
* A vehicle (`MaszynaRailVehicle3D`, libreloaded's `TrainSet3D`) is not rebuilt on
  `GameDataServer.data_reload_requested` as models, cabs, tracks and materials are: a game directory
  set while `demo_3d` runs leaves its vehicles unbuilt until the scene starts again.
* Tests and the demo share one user directory (`Libmaszyna-Demo`): a test setting
  `maszyna/game_dir` to the fixtures and restoring it in `after_each` undoes a directory the
  operator sets while the test runs.

## Architecture rework (#184)

Each stage is one PR, titled `(#184) <area> - <what>`, and leaves the game runnable.

* **G** - cabin scripts still take the whole dump (libreloaded's `cabin_windscreen_wipers.gd` does
  it per frame).
* **H - not started.** No `UpdatePhase`; the order is hand-written in
  `MaszynaMoverVehicleServer::stepping_advance()`. Check it against
  `TMoverParameters::ComputeMovement`/`Update` and the three ordering bugs on record (#57 line
  breaker, `Mred`, `roof_light_enabled`). A `test_vehicle_doors.gd` must exist first -
  `RailVehicleDoors` ticks and has no test.
* **Scenery teardown vs. streaming** (`FINDINGS.md`, 2026-09-22): the preload
  (`maszyna_e3d_model_manager.gd::load_model`, a full `ResourceLoader.load()`) creates renderer
  resources off the main thread, so a teardown overlapping a running stream can still race. Remedy:
  parse on the worker, build on the main thread. Reproduce by clearing `user://cache/rail_vehicle`
  and `fiz` and running `test_zzz_ep07_cabin_main_switch`.
* **Fixture .fiz files differ from the current game data**: `sr61v1.fiz` lacks
  `DirChangeMaxPos=2`, the ED72/EN57 cars have `BM=P10-Bg BSA=16` where the game has
  `BM=P10-Bgu BSA=5`. Decide whether to refresh them from the game dir (copy only).
* **The `.fiz` path has not been run in the game** since the components stopped being nodes -
  only in tests.

### Readers still on the state dump


### What still reaches the vehicle's controller

* Typed server getters to add in libreloaded: `VehicleServer` - `vehicle_get_direction`,
  `vehicle_get_power`, `vehicle_get_max_velocity`, `vehicle_get_mass_total`; `RailVehicleServer` -
  `vehicle_get_train_type`, `vehicle_get_coupler_stretched`, `vehicle_get_train_damage`; the load
  through its component. Move onto them the AI (`ai_driver/maszyna_*`, `maszyna_auto_rewident.gd`),
  sound (`train_sound_system`, `brake_sound_model`, `running_sound_model`),
  `MaszynaE3DRailVehicleRenderingServer.cpp` (smoke, load) and the start-up tests' helper, so only
  assembling a vehicle keeps the controller (no `vehicle_get_controller(` outside assembly);
  `CODE_STYLE.md` still allows `controller.max_velocity`.
* The AI's fallbacks and copies of the Mover's defaults -> typed server getters, no invented
  values: `maszyna_driver_braking.gd`, `maszyna_driver_pantographs.gd`,
  `maszyna_driver_traction.gd`, `maszyna_ai_driver.gd` (the line numbers were the game's copy's,
  before the AI came here - to be found again).
* Where the original's AI bypasses the cab (`driverhints.cpp`: `mvOccupied->RelayReset()`, ...),
  the AI sends `VehicleServer.vehicle_send_command()`, not `CabinSystem.act()`: `fuse_bt` and
  `converterfuse_bt` (`MaszynaDriverHints.BUTTONS`).
* `MaszynaCabinDirectionKey` (`cabin/direction_key.gd`) reads `direction` from the dump until
  there is `vehicle_get_direction()`.

## Cabins

* **Next, right after the split commit (operator, 2026-10-09):** the occupancy layer drives the cabs
  by plain vehicle commands instead of `set_driver_cabin_kind()` (libreloaded's
  `RailVehicleServer.cpp:1395`): `cabin_activate` (today's `cab_activation`), `cabin_change`
  (`ChangeCab()`, Mover.cpp:736) and, if decided, `cabin_occupy`. Every `cab_*` command renamed to
  `cabin_*` at once. Open: whether `CabOccupied` is the vehicle's control state (any sender may set
  it) or the persons' fact (only `RailVehicleServer` sends it) - the first changes `CODE_STYLE.md`,
  "A vehicle is commanded, not called". `mover_demo` then activates its cab without a seated person.

* **Gauges that gate their command but have no catalog entry yet** - when ported, they get
  `requires_gauge` (Train.cpp line in brackets): `antislip_bt` (2232), `nextcurrent_sw` (1584),
  `signalling_sw` (2602), `converterlocal_sw` (4463), `compressorlocal_sw` (4629).
* **`MaszynaMaterialFactory._get_shader_variant()` builds its shader variants with
  `code.replace()`** (alpha blend, cull disabled, specgloss): to be static variant files with the
  render modes they have now (`CODE_STYLE.md`, "A shader variant is a file"). A vehicle's glass
  uses the blended variant, so the first vehicle near the camera compiles it in the game.
* **A translucent skin submodel is forced whether or not the skin has an alpha channel**
  (`MaszynaE3DInstanceBackend::_submodel_translucency()`); the original draws it in the alpha pass
  only when it has one (`DynObj.cpp:331-346`, `textures_alpha`).

### Controls not in the cab yet

Each needs a catalog entry and, where missing, a vehicle command (with its `type()` branches):
`epbrake_bt` (ggEPFuseButton, Train.cpp:2350), `doorrightpermit_sw` (Train.cpp:7263),
`compressorlist_sw`, `autosandallow_sw`.

### E186 controls - what is still simplified

* The E186 screen's pantograph page (`traxx_renderer.py`) toggles its "odbiornik prądu"
  1 / 2 / 1+2 without OP1/OP2 being pressed - not checked against the original.
* `MaszynaMoverRailVehicleEnginePowerSource::pantograph()` opens the master valve itself when a
  pantograph is raised (1c0c044); the original opens it only from pantselected_sw or by its start
  mode (`PantEPValveStart`). Remove it once every cab has a way to the master valve.
* Light presets: `SetLights` runs on a preset change only - the original also runs it on cab
  (de)activation, battery and direction changes (Train.cpp:2924-3137, `2440`, `2463` when
  `LightsPosNo > 0`). The model's lamp inventory (iInventory) is not known, so a rear end that
  could show red markers or plates shows the markers (DynObj.cpp:7367).
* `headlights_dimmed` is state only - nothing renders a headlight beam to dim.
* Distance counter: the double-press start (FIZ `DCMB`/`DCDPP`, not in the vendored Mover), the
  switch-off after the train's length and its sound (Train.cpp:10153).
* The radio message lamp's sounds (`i-radiomessage` soundinc/sounddec) play at their own gain; the
  original plays them at the radio's volume (Train.cpp:10258-10261).

### Gauge lamps (`<name>_on`)

Only the reverser buttons have their `state_light`. Train.cpp:11995-12040 binds a flag to about
forty more gauges (speed control buttons, door permits, door step, ...), each needing a state key
and a catalog `state_light`. The lamps also light without low voltage - TGauge gates them on it
(Gauge.cpp:379). The door permit lamps (`doorleftpermit_sw`/`doorrightpermit_sw` `_on`,
`i-doorpermit_left:`/`_right:`, Train.cpp:11754, 12033, 8511-8516) blink by
`DoorsPermitLightBlinking` unless a door of that side in the trainset is open.

### Mouse operation of the MMD cabs

The mouse system itself is libreloaded's (`CabinHUDMouseSystem`); what is left of
drivermouseinput.cpp in it is in libreloaded's `TODO.md`.

* Key hints only for the first widget of a label (`mmd_cabin_instancer.gd`, `_build_widget()`).
* **Brake valve drag direction - cause not found.** `brakectrl` forces its signs in the catalog
  (`"mouse_drag_signs"`) because the grip heuristic (libreloaded's `_increase_signs`) got SM42's
  valve backwards in game while a headless check predicted it right. Find what differs in game
  (6d/6d1/6da have opposite `rot` signs, the occupied cab, the cab's transform), then drop the
  force.
* Captions are taken when a control is built (`MmdCabControlCaptions`) - a language change shows
  after re-entering the cab.

### Python integration

* **Windows runtime tested under wine only** - `maszyna-python-host.64.exe` with Steam MaSzyna's
  `python27.dll` and `python64/` starts and renders a PIL screen; not yet on a player's Windows.
  libreloaded's `make python-runtime` builds only the Linux one.
* **`maszyna-python-host` in an export** (`[dependencies]` of libreloaded's
  `libreloaded.gdextension`, LS-009): not yet seen in an exported game; on Linux its executable bit
  must survive the export.
* **Non-ASCII paths on Windows**: CPython 2.7 takes paths in the ANSI code page, the host hands
  it UTF-8 (`maszyna/python/home` set to an absolute path, the game directory and the scripts'
  paths). The default home is relative, so only a game directory outside the code page breaks.
* **Keys with no source yet**: `off_from_dimmer` (no dimmer positions in the vendored Mover),
  `lights_compartments` (`compartmentlights_sw` not ported), `doors_no_N` (MMD `animations:`
  count, held by the model layer), lamps beyond the five carried in `lights_front`/`lights_rear`/
  `lights_train_*`; powered cars are told by engine type where the original tests
  eimc[eimc_p_Pmax] > 1 and fills `eimp_cN_*`/`diesel_param_N_*` in one shared count.
* **Cab keys**: `universal10`..`29` are read, but only `universal0`..`9` have a catalog entry.
* **AI/timetable keys**: `velocity_desired`, `velroad`, `vellimitlast`, `velsignallast`,
  `velsignalnext`, `velnext`, `actualproximitydist`, `train_atpassengerstop`, `train_length`,
  `trainnumber`, every `train_*` key (mtable.cpp:641), and `$timetable=` (dictionary.cpp:36).
* **Tests**: `vehicle_get_coupled()` stopping at a coupling without the flags asked for;
  `PythonScreenState.compose()` beyond the EIM row.
* **Commands a script returns are not executed** (two scripts send `lightsset`); map
  `simulation::commandMap` names onto vehicle commands (PyInt.cpp:138-194).
* **Touch input** (`touches`, `screen_touch_list`, Train.cpp:10713) is always empty.
* `pyrylandia` is referenced by an MMD and exists nowhere under `dynamic/`.
* **Another game directory keeps the interpreter** and the runtime it was loaded from until a
  restart.

### Other

* The `horn_bt` lever holds `horn_low` at +1 and `horn_high` at -1; the original animates
  `ggHornButton` the other way round (`Train.cpp:7958`). Check against a cab model before flipping.
* `VirtualCabin` for cabs without a hi-fi model (`cabNmodel: none` or missing, e.g. su46): the
  original keeps them enterable with the low-poly interior (`Train.cpp:8692`, `DynObj.cpp:1214`).
  Hook: `MaszynaDynamicTrainCabin` builds an empty cabin with `has_cab_model = false`.
* Diesel-electric shunt mode on the second controller (`ShuntModeAllow`/`ShuntMode`, `AnPos` by
  0.025 per step, `Train.cpp:1190-1197`, `1351-1357`); `shuntmodepower:` (`Train.cpp:10542`)
  unmapped.
* Shift+V/Ctrl+V (pantograph compressor) in cab 0 of the pantograph unit without
  `pantcompressor_sw` (`Train.cpp:2872`, `2915`).
* Rest of TDynamicObject::Update's driver block (DynObj.cpp:3240-3400): the ED/PN brake force
  split of an induction motor trainset, `EqvtPipePress = GetEPP()`, the unpowered-car copy of
  MainCtrlPos/SpeedCtrl (DynObj.cpp:3272-3276).
* Wheels turn at half speed: `MaszynaMoverRailVehicleWheels` adds `rad_to_deg(V*dt/D)`, the
  original `rad_to_deg(2*V*dt/D)` (DynObj.cpp:3780-3784 at df5a8a8);
  `test_rail_vehicle_wheel_angles.gd` checks only the front/rear ratio. Waiting for the operator.
* Source citations drifted: many `DynObj.cpp`/`Train.cpp`/`Mover.cpp` line numbers point at an
  older checkout. Refresh against one named revision.
* EIM `Imaxrpc` and `BRVto` (Mover.cpp:11304-11305) - the vendored Mover lacks them.
* Spring brake: `springbrakerelease` (`Train.cpp:6874`) and the `springbrakepress:` gauge
  (`Train.cpp:12221`) have no cab control or key.
* E186 (`dynamic/pkp/e186_v2`) labels outside `MmdSemanticCatalog`: `pantselected_sw:`
  (`Train.cpp:3405-3549`), `pantfrontoff_sw:`, `pantrearoff_sw:`, `lights_sw:`,
  `dimheadlights_sw:`, `radiostop_sw:`, `radiovolumenext/prev_sw:`, `universalbrake1_bt:`,
  `doorpermitpreset_sw:`, `distancecounter_sw:`, `universal0-8:`, gauges `brakepressb:`,
  `limpipepress:`, `clock:`, lamps `i-mainpipelock:`, `i-tempomat:`, `i-malfunction:`. Four
  pantographs (`CollectorsNo=4`); the wrapper animates two.
* `MaszynaCabinBattery`, `MaszynaCabinCabActivation`, `MaszynaCabinManualBrake`,
  `MaszynaCabinWipers` only register what `MaszynaCabinUnmodelledControls` registers anyway - fold
  them in.
* A tile's placeholder guesses its width (the game's `TileGrid.PLACEHOLDER_STRETCH`): FIZ `Dim=` is
  parsed nowhere for `MaszynaSceneryInfo.Vehicle`.

## Sounds

* MMD offsets of non-running sounds are used raw - not turned 180 degrees like the model
  (`MASZYNA_VEHICLE_FRAME`), so horns, compressor, brakes etc. sit mirrored (x, z). Running sounds
  convert (`MmdSoundBankInstancer._build_running_events()`).
* Missing running sounds: `tractionacmotor:`/`inverter:`/`motorblower:` (`DynObj.cpp:5745-5800`,
  `8012-8080`), `wheelflat:` (`4722`), `derail:` (`5910`), `transmission:` (`5822`), cab
  `huntingnoise:` (`Train.cpp:8284`).
* Wiper sounds (`wiperfrompark:`, `wipertopark:`, `DynObj.cpp:4082-4099`) not played; the arm
  swing direction not checked in game.
* Wheel clatter bump (`AccVert`, `DynObj.cpp:3533`, cab shake only).
* Open cab window (`Global.CabWindowOpen`, `DynObj.cpp:4638`, `Train.cpp:8274`); no cab window
  state yet.
* `pitchvariation:` (`sound.cpp:375`) is parsed, never applied.
* `pantographup:`/`pantographdown:` play at the bank's position; the original places them at the
  pantograph that moved (`DynObj.cpp:3881-3934`, `4007-4036`). Dump the E186 bank.
* Brake sounds (`MmdSoundFormulas`): a bank heard again after a cut takes the pressure change made
  while it was cut as one step of its rates (`DERIVATIVE`), where the original started its rates
  over. Not compared by ear with the original.
* Spring brake (`MmdSoundFormulas`): the original stops the other of its two sounds as one starts
  (`DynObj.cpp:4749-4759`); a gnd-sfx trigger cannot stop another event.

## Vehicles

* **Road cars - what is not ported** (`MaszynaCarDriver`, `MaszynaRoadNetwork`,
  `MmdSoundFormulas`):
  * a car standing in the scenery (velocity 0) stays on the road's centre line; the original puts
    it on the roadside by the road's width with its shoulders (`WidthTotal()`, `DynObj.cpp:2329`,
    `Track.cpp:2071-2078`), whose `tex_width` only the track drawing has
    (`MaszynaTrackRenderingBackend`)
  * a car placed by hand (`MaszynaRailVehicle3D`, `RailVehiclePhysicsNode`) is still a rail vehicle
  * a crossroad (`cross`) is read for its roads only and stays undrawn (`Track.cpp:1519` draws it
    otherwise than a track)
  * a car on a crossroad's way or on a loop is on no track: the `cross` track's events do not
    fire for it
  * a `DieselEngine` car (13 of the 47 in `dynamic/road`) sounds its engine by the `Dumb` formula:
    the original's diesel one (`DynObj.cpp:8275-8285`) reads the Mover's diesel model
    (`dizel_fill`, `dizel_nmax`), which a car of RoadVehicleServer has not got
  * a car's lamps are lit at any distance and with no battery; the original lights its red markers
    within 400 m only (`DynObj.cpp:870`) and its lamps only with 24 V or 110 V
    (`DynObj.cpp:1210-1247`)
  * a world's (`WorldSceneryProvider`) cars get no road network - only MaszynaSceneryProvider
    builds one
  * `MaszynaRoadNetwork` makes a road of every `road` track; the tracks between two parting places
    joined into one road wait for a road laid along several tracks (libreloaded's `TODO.md`)

* **EN76 main pipe does not fill** (reports#12, l053_poludnie.scn, EN76-005a, local build): all
  four units with the main reservoir at 8.26 bar, `pipe_pressure` ~0, the hand brake released,
  the train brake handle of 005a at 0, of 005b-d at 1; the driving aid's hints contradict each
  other (screenshot in the report). Diagnose from `snapshot.json` against the original's EN76 FIZ.
* **Wagons with full cylinders show no brake force** (reports#13, l053_poludnie.scn,
  ES64F4-846 + 20 wagons, build 20261007-1333): all wagons `brake_air_pressure` ~3.95 bar with
  `brake_force` 0.0 while the train creeps on at 0.05-0.1 km/h; the loco itself 365 kN (hand brake
  20, independent brake full, handle 6). "182/183 do not brake" - check the wagons' `BrakeForce()`
  inputs (NBpA, BCN, the brake's friction) first.
* **Left of the move of the state to its owners**: `MaszynaMoverRailVehicleBrake` fills the brake
  valve and the compressor for a wagon too; the master controller's, reverser's and cab's commands
  (`main/second_controller_*`, `direction_*`, `cab_activation*`, `cab_change`) and the cab
  activation config are still the controller's; `ground_relay_reset` and
  `cntrl_ground_relay_start_mode` are the controller's while the relays are the engine's;
  `RailVehiclePowerSupply.power_changed` has no listener; the driver's code looks a component up
  per call.
* **A vehicle's first build costs ~50-140 ms on the main thread** (headless, warm caches): the
  `instance_build()` of each model 12-40 ms, nearly all of it the materials created for the first
  vehicle of a type and skin (`MaszynaMaterialManager.get_submodel_material()`: `.mat` parse, DDS
  loads, path resolution) - the same model built again with its materials held takes ~1 ms. A
  vehicle coming within the draw distance is one such stall a frame (`BUILD_BUDGET_MSEC`); moving
  the material and texture reads to a preload thread is the item below.
* **"has no skins set, but submodel requires material #3"** for `pkp/11xa_v2` and `pkp/14xa_v1`:
  check what the original draws for a replaceable skin index the skin has no file for, and warn
  once per vehicle type, not per submodel and vehicle.
* **The stepping of parked vehicles** (Wrzosy, profiling build, after the neighbour scan cache):
  the step is 30.9% of the main thread - forces and movement ~8%, the remaining neighbour scans
  ~4% (vehicles near moving ones), placements reported to rendering ~5%. Parked EMUs/DMUs keep
  their physics on by the Mover's own rule (Mover.cpp:4487-4489).
* **WeatherNode costs ~16 ms a frame** on Wrzosy with one rain volume - measure inside.
* **Textures loaded on the streaming's preload threads**: every owner's preload parses the `.mat`
  and loads all its textures into `MaszynaMaterialManager`'s cache, the build only assembles.
  Measure before and after with the Braniewo route probe under `gamescope --backend headless`; the
  disk cache removal is not measured yet either.
* **Braniewo station renders ~5400 draw calls and ~11 M triangles a frame** (~30 fps on an
  RX 580).
* **One `SfxBank` per vehicle type** instead of per vehicle: needs `random_choices`,
  `pitch_variation` and `start_fraction` per player in libreloaded's vendored gnd-sfx.
* The vehicle build exceeds `MaszynaVehicleSystem.BUILD_BUDGET_MSEC` (~16 ms a vehicle).
* Vehicles and the editor:
  * "Edit FIZ" (libreloaded's `RailVehicleRenderingServer.vehicle_set_editable()`) shows the
    exterior and the low-poly interior as nodes; the passengers, attachments, coupler adapters and
    cargo stay instances, and nothing edited there is kept - the nodes are built again from the
    model.
  * Trainsets tab: "Show" puts marker nodes and selects them; a hand-made `TrainSet3D` lists no
    vehicles right after its scene is opened.
  * Vehicles tab (spawner): a vehicle is placed only by a track name and offset [m]; spawning at
    a kilometre needs the scenery's mileposts and a lookup from kilometre to track and offset
    (none exists - only `TimetableEntry.kilometre`).
  * Operator's direction: a toolbar tool "Make trainset" for 1+ selected `MaszynaRailVehicle3D`s
    puts them into a new `TrainSet3D`; vehicles that belong to a trainset are detached from it
    into the new one.
  * A vehicle coming within detail distance is built twice
    (`instance_attach_object_instance_id()` then `instance_set_instancer()`).
  * `compiled.nodes` still carries the Time/Config/Atmo nodes.
  * "Edit FIZ" logs errors: `cabin_python_screen.gd` and `maszyna_dynamic_train_cabin.gd`
    disconnect in `_exit_tree()` what they connected once; `!is_inside_tree()` transform reads -
    count again.
* A distant vehicle's low-poly interior keeps its baked emission:
  `instance_set_emission_energy()`/`instance_set_part_emission_energy()` reach only the NODES
  backend.
* The low-poly interior's compartment and corridor sections stay unlit (`DynObj.cpp:1334-1352,
  2425-2433`, CompartmentLights).
* No test for `MaszynaE3DRailVehicleRenderingServer`'s low-poly cab lights, nor for the lamps drawn
  from the lighting (`_update_lights()`).
* `MaszynaVehicleStructure` and `MaszynaRailVehicleAppearance` - candidates for a better name.
* The 8 m shift of a trainset's first vehicle on a track with `Event0`
  (simulationstateserializer.cpp:1050-1057).
* The `structure-vN` tag (`maszyna_vehicle_system.gd`) is bumped by hand.
* `LoadFIZ_Cntrl` start modes `CompressorStart`, `PantCompressorStart`, `MainStart`,
  `ConverterOverloadWhenMainIsOff` (Mover.cpp:10905-10925) are not parsed; their properties belong
  on `VehicleController`, not `RailVehicleElectricEngine`. The `converter` command is still an
  electric engine's only.
* `BrakeValveParams` (Mover.cpp:10397) is never set, so every ESt distributor is built as ESt4
  (no `TRura`/`Podskok` for ESt3, `AL2`, `PZZ`, `HBG300`, `3d`/`4d`, `-ED` dropped; ~200 FIZ).
* The current sent to the wire is `get_current0() / collecting`
  (`vehicle_collect_current()`), not the ported `fPantCurrent` - open. The energy meter has no
  test.
* Tests still take the controller and call it directly; "tests go by RID" is not done.
* `demo/examples/mover_demo.*` and the `custom_*_train_part` examples assume components as nodes
  and do not run.
* **Headlight properties nobody reads**: `RailVehicleLighting.head_light_color` and the normal
  and high-beam multipliers are parsed but not drawn - the original tints and scales the head
  lights by them (`rendering/lightarray.cpp:78-109`); the wrapper applies only the dimmed
  multiplier (`MaszynaE3DRailVehicleRenderingServer.cpp:1125-1126`) and a fixed 1.0 otherwise
  (`MaszynaE3DNodesBackend.cpp:112`), and has no high beam. `instrument_type` has neither a writer
  nor a reader.
* `RailVehicleBrake.friction_elements_per_axle` defaults to 1, the original's `NBpA` to 0
  (`MOVER.h:1607`), which decides `Mover.cpp:5225`.

## Rendering

* **GPU 22 ms of a 33 ms frame** (RX 580, 3607 draw calls) - measure on libreloaded's
  `make compile-profiling` and split by pass before any hypothesis.
* **A light's submodels have two managers**: `MaszynaE3DRenderingServer` (`lights_state`) and the
  cab widgets (libreloaded's `CabinIndicator3D`, `CabinSpotLight3D`) writing `visible`; the widgets
  should ask the model.

### Self-illumination of light submodels

* `MaszynaE3DModelInstance` nodes never light automatically: `_normalize_lights_state()`
  (`maszyna_e3d_model_instance.gd:278`) pushes `false` for every light not set.
* A `colored` light submodel has no emission (`COLORED_MATERIAL` first).
* `lightcolors` do not tint the submodel (`SetDiffuseOverride`, `AnimModel.cpp:625`).
* Baked, not per frame: the original lights while `Global.fLuminance < fLight`
  (`opengl33renderer.cpp:3452`); `maszyna_material_manager.gd:371` decides once.
* Emission 1.0 after AgX unmeasured - needs a rendered frame of the operator's scenery.

### Smoke emitters

The emitters are drawn by libreloaded's `SmokeServer`; what is left of `particles.cpp` there is in
libreloaded's `TODO.md`.

* Vertical decay (`particles.cpp:365-380`): needs `Global.AirTemperature` (a `#define` in the
  vendored Mover), overcast and speed.
* `min_inclination` dropped (only `smokesource_st45`).
* Vehicle emitters are not switched off beyond `2 * BaseDrawRange * fDistanceFactor`
  (`particles.cpp:452`).

### Other

* E3D: VNT1, VNT4+ userdata and TRA1 are not read (no data uses them, `Model3d.cpp:1977`, `:2135`).
* T3D: `priorityLoadText3D` (`Model3d.cpp:1634`), the "banana" root (`Model3d.cpp:2447`), a
  material's `selfillum` (`Model3d.cpp:483`); `stars`/`point` not drawn; `hotspotpower:` /
  `light_energy` not passed by `MaszynaE3DModelBuilder`.
* Vehicle headlamps and cab lights not measured for shadow acne at night.
* **Windows: td.scn ends the game without a message** (RTX 4080 SUPER, Vulkan, build
  20261007-1333). The `--verbose` log ends at starting the Python interpreter and `--no-python`
  runs without fault (`docs/findings-archive.md`, 2026-10-09): Python now runs in
  `maszyna-python-host`, so the game survives it. **Still open: why it failed for that player** -
  the next build's `app.log` has the host's stderr and exit code; asked of the player meanwhile:
  `--verbose 2> stderr.txt` from `cmd`, the event log's faulting module and exception code,
  whether eu07.exe shows the Python screens, another Python 2.7 in the registry
  (`PythonCore\2.7`) or on `PATH`. Also unguarded: a pending vehicle build whose appearance was
  set to none - libreloaded's `RailVehicleRenderingServer::_build_models()` reads the appearance
  without a check.
* Normal maps: Godot's tangents not checked against the original's `f_tbn`.
* Overexposure in the demo scenery unmeasured. Candidates, one at a time: `tonemap_mode`,
  `soft_shadow_filter_quality` 3 -> 1, `directional_shadow/size=8192` (`042b392`), `fog_enabled`,
  `cloudiness` 0.35 -> 0.21 (`ab75bbe`).
* Unmapped original shaders: `clouds`, `stars`, `invalid`, `normalmap_phys`.
* Specgloss: `detail_normalmap_specgloss` masks with the normal map's alpha instead of
  `reflblend`; `shadowlessnormalmap` binds no normal map.
* `rain_windscreen.gdshader`: droplets ignore speed and wind; transparent things behind the glass
  fade under the film; "down" not checked on a real cab glass.

## Scenery loading

* calkowo_sn61_zima.scn: `Cannot load include file: .../scenery/F` - a cut name, an include whose
  name has a space or a parse slip; not looked into.
* After a scenery has loaded, `Unicode parsing error ... Invalid UTF-8 leading byte (b0)` once
  (td.scn) - source not found.
* Air temperature (libreloaded's `EnvironmentNode.temperature`) is consumed by nothing - the
  vendored Mover has `#define Global_AirTemperature 15.f`.
* Other `config` entries dropped (`scenario.time.override/offset/current`, `Globals.cpp:356-385`).
* Include instancing (`grass.inc` included 24078 times): classify includes as `instanced`/`full`,
  cache in local space, invalidate by dependency list.
* Lazy loading off (default): a vehicle's cargo (`_build_load()`) and coupler adapters
  (`_update_coupler_adapters()`) still load through `model_load()` at their build, outside
  `ResourceLazyLoader` - their files change while the game runs. After a data reload a resource
  resident by registration is loaded again only by its next use.
* The subscene cache is used only by queued parsing; `parse_file()` reparses every include.
* **River and cross tracks are not imported** (#318) (`maszyna_node_track_importer.gd` drops them
  whole, without a warning); linia61_towarowy1.scn drops 39 river and 9 cross. Port: rivers drawn
  as roads (`Track.cpp:1686-1712`, `fTexHeight1` negated :507, `fTexLength` instead of the
  material's length), crossings (`tt_Cross`, :1520-1683, the extra edge segments :771-787).
* Tracks and traction have no worker `preload`.
* A terrain chunk's material is looked up on the main thread (`.mat` parse, DDS decode, md5 key) -
  move to the worker preload once the streaming panel shows terrain dominated by it.
* Stream baked 1 km chunks (MultiMesh per mesh+material, merged triangles, ready track meshes,
  cached on disk) instead of pieces; switch blades stay outside.
* `MaszynaSceneryChunkRenderingServer` (deprecated) - remove: `SceneryStreamingServer` streams a
  triangle chunk itself.
* Memory: every model placement keeps a full `MaszynaE3DInstanceData`; a subscene's terrain sink is
  kept whole until parsed; a terrain chunk holds arrays beside its mesh while built.
* Load model placements and packed `nodes` per 1 km chunk; needs events and signal heads to reach
  a model otherwise than by its RID. Read `[SceneryMemory]` first.
* Terrain triangle order follows the parse workers (may differ between conversions).
* Main-thread stalls: `MaszynaTractionServer.network_build()`, libreloaded's
  `TrackServer.topology_rebuild()` unbudgeted; libreloaded's `ScenerySoundServer.sound_create()`
  rebuilds the bank per sound (O(n^2)) - needs an append in `gnd-sfx`.
* `[SceneryLoad]` lines not measured on a heavy scenery.
* `maszyna_node_track_importer.gd` drops `turn`, `table`.
* Lamp head colour does not match its tinted pool - needs a flag in the `MaszynaE3DMaterialResolver`
  key.
* An economy-mode merged light takes the max `energy`, not the sum.
* `latarnial_betdziur`'s `zarowka` stays off - should the `_on`/`_off` suffix rule apply to
  scenery models (`AnimModel.cpp:303`)?
* `m_lightopacities` transition (`AnimModel.cpp:542-549`); `notransition` ignored.
* `Overcast` folded into the light level instead of subtracted at the threshold
  (`AnimModel.cpp:598`).
* "Edit SCN" sectors: the scenery's vehicles in a sector (a proxy on `RailVehicle3D.set_vehicle()`),
  a sector kept until deleted, edits written back (and the stream entry moved), the inspection
  restarted when the tab is shown again.
* A model's `angles` replace its whole rotation in the original (`simulationstateserializer.cpp:956`,
  `AnimModel.cpp:365`); the importer adds the context's rotation. `scale` (`AnimModel.cpp:373`) is
  not read.
* An `include` with no filename in the real data - source unknown.
* Unloading a scenery leaves its weather; a next scenery without `atmo` inherits it.

## Signalling (#296)

The signalling servers are libreloaded's (`SignallingServer`, `SignalHeadNode`); their open work
is in libreloaded's `TODO.md`.

* **No data joins an isolated section to a source**: `SignallingServer.source_set_isolated()`
  reports a section through a source, but the legacy scenery has no interlocking, so nothing
  creates those sources yet.
* **Point sensors (our USRK layer, not a port)**: the original has no sensor object - a "sensor"
  in its data is a track with an event (`FINDINGS.md`, 2026-10-12 "The original has no point
  sensors"). If a source is ever fed from scenery data, the port is a bridge "track event ->
  `SOURCE_IMPULSE_EVENT`" (event1/event2 give the polarity), like the isolated section -> source
  bridge - not a new sensor object on a track.
* **The logical aspect for trains**: a signalling implementation has only the lights, nothing it shows
  reaches a train (the driver reads the memcells).
* **`ls_Dark`/`ls_Home` from a `lights` event** (value 3, 24 times): no light following the
  daylight.
* **Semaphore arms** - the `animation` event on a named submodel (`Event.cpp:1569-1735`).
* **Telling signal heads apart** from other lit models (street lamps included).

## Scenario events

* **Track events**: the direction filter by the intended direction (`eventfilter`,
  `TrkFoll.cpp:117-121`); delay <= -1 events
  queued on every move along the track (`TrkFoll.cpp:249-260`); a crewed vehicle is any DRIVER,
  the original's `Mechanik->primary()` one per trainset.
* **`putvalues`/`getvalues`**: `CabSignal` acts on the vehicle crossing it; the Mover's other
  commands (`Load=`, `UnLoad=`, `BrakeDelay`, ... `Mover.cpp:12187-12720`) are dropped.
* `updatevalues`/`addvalues` of a memory on a track: the `:sent` event (`StopCommandSent()`).
* **Event types without an action**: `whois` (`Event.cpp:993-1153`), `logvalues`, `texture`
  (`:1474-1543`), `friction` (`:2100-2104`). `switch` ignores the blade speed and delay
  (`:1855-1873`); `animation` has no `digital` or `.vmd` mode (`:1654-1682`); a radio message is
  played and transcribed only when heard at its start.
* **Scenery sounds**: player defaults except `max_distance`; ambient fade and the 2750 m start
  (`audiorenderer.cpp:184-199`, `sound.cpp:364-371`). Not checked by ear.
* **Memory and the AI**: pushing a memory to the vehicles on its track when it changes
  (`Event.cpp:538-548`), `bCommand`/`CommandCheck` and `:sent` (`MemCell.cpp:52-99`, `196-205`).
* **`departuredelay`**: the original asks the vehicle's driver only when `primary()`
  (`Event.cpp:2431-2435`).
* **Launchers**: numeric key codes, `-10000` (`EvLaunch.cpp:182-186`), `traintriggered`; a timed
  launcher of a radius is checked only at its minute, on a memory change and at the clock start.
* **A click on a scenery model** is not hidden by what is in front; Alt picking toggle ignored
  (`drivermode.cpp:493-500`).
* The `queueevent` console command.
* Station announcements (`load_sounds()`, `mtable.cpp:644-671`).
* Events of one include cannot refer to events of another `MaszynaIncludeNode`.
* Proxy nodes for editor-built scenes (`ScenarioEventNode`, `ScenarioMemoryNode`,
  `ScenarioLauncherNode`).

## Scenario scripts (Lua)

* **`dynobj_putvalues` on a vehicle nobody drives** is dropped; the original hands it to the Mover
  (`lua.cpp:293-294`).

## Drivers (#297)

* **High-level vehicle operations through the servers, executed by components**
  (`maszyna-architecture`, "Servers forward to components", 2026-10-09): libreloaded's
  `RailVehicleServer` couples (`vehicle_couple()`, `uncouple`, `is_coupled_by`,
  `is_coupler_automatic`, adapters, `vehicle_get_coupler_joinable_flags()`) through
  `RailVehicleController`/`MaszynaMoverRailVehicleController`; the coupling logic moves to the
  server, executed by the `RailVehicleBuffCoupl` component (`MaszynaMoverRailVehicleBuffCoupl` on
  the Mover), the per-end `AllowedFlag` (`BuffCoupl1/2`) and `control_type` with it. The same for
  walking through the cabins, managing trainsets and moving a vehicle. The AI then couples by a
  server operation instead of the `coupler_connect` command. Plan with the operator first.
* **Commands a vehicle does not have are sent and fail**: `fuse_reset` from the cab key (N, every
  cab, `mmd_semantic_catalog.gd:221`) and `security_cabsignal_trigger` from an SHP event
  (`MaszynaVehicleCommandAction.cpp:40`) on an SN61 - no electric engine, no security system;
  "Unknown command" in the log each time.
* Cab logic without the 3D cab: an AI caller must pass what a widget would have worked out (knob
  and switch limits, spring return, the horn's value); `MaszynaCabinControls` parses the MMD with no
  random choices. The `brake_level_drive` `CabinCommand` node still carries `command`/
  `command_param`, only as the guard of its key.
* **Cab targets**: the motor car's ammeters, voltmeters and lamps (`mvControlled` in
  `update_gauges`) need `target` in `MmdSemanticCatalog`.

## Game data

* `MaszynaVehicleProfileManager._ensure_viewport()` (`maszyna_vehicle_profile_manager.gd:261`) is
  an `ensure_*` API: create the viewport where the manager is.

## Tests

* **Tests run inside the demo's project** (operator, 2026-10-12: tests may not live in the demo or
  game project). `demo/tests` uses the demo project's `user://` (`Libmaszyna-Demo`); a tests
  project of its own is still to be made - its own `project.godot` and `custom_user_dir_name`, its
  addons linked as the build links them into `demo/addons`, fixtures for what the tests take from the demo (`vehicles/`,
  `materials/`, `sounds/`, scenes), and the Makefile's `test`, CI, `.gutconfig.json`, `AGENTS.md`
  and the testing skill updated.
* **Tests that switch the game dir** (`MaszynaGameDirectory.save_game_dir()`) write the user's
  `settings.cfg`, and a crash skips the restore: `test_audio_stream_manager.gd`,
  `test_e3d_lights_state.gd`, `test_fiz_train_controller.gd`,
  `test_maszyna_node_dynamic_importer_direction.gd`, `test_material_manager_variants.gd`,
  `test_nodebank_library_builder.gd` and the tests spawning a vehicle of
  `demo/tests/fixtures/dynamic/`. Needs a non-persistent override; each switch also reloads the
  game's data (`GameDataServer.data_reload()`).
* **The start sequence**: still open - the converter and the master controller through the cab,
  an EP09 and an ED78/36WE fixture (`ep09_v1/104e-039.fiz`, `impuls_v1/ed78-028-a..d_zachpom.fiz`:
  `LMaxVoltage=24`, control cars `a`/`d`, motor cars `b`/`c`). The five tests b5e744f1 took
  `battery_voltage` from (`test_driver_server.gd`, `test_driver_braking.gd`,
  `test_train_controller_radio_channel.gd`, `test_train_ep_fuse_switch.gd`,
  `test_train_sound_system.gd`) get `build_power_supply()` back where they relied on the low
  voltage. The tester's report (EU07, ED78, 36WE unstartable) is not reproduced: needs the build
  number, `godot.log`, the step that stops, and a probe of an ED78/36WE trainset.
* **Start-up tests not cut**: EN57AKL, EN57AKM 2013, EN71AKS and EW58 lead no trainset of any
  scenery; EU44's cab (es64u4.mmd) models only its horns; EP03's FIZ gives pressures in MPa.
* **Test audit 2026-10-04** (`docs/findings-archive.md` "Test audit") - each listed script is
  rewritten or deleted:
  * *The test hands in what it is about* - move onto fixture FIZ vehicles:
    `test_traction_power_pantograph.gd`, `test_train_electric_induction_engine.gd`,
    `test_rail_vehicle_idle_pantograph_voltage_regression.gd`,
    `test_rail_vehicle_pantograph_geometry.gd`, `test_cab_lights.gd`, `test_train_wipers.gd`,
    `test_train_battery.gd`, `test_maszyna_cabin_button_types.gd`,
    `test_maszyna_cabin_cab_activation.gd`, `test_maszyna_cabin_unmodelled_controls.gd`,
    `test_scenario_script_server.gd`.
  * *Asserts what it set* - keep only what a parser or `apply_config` turns into an effect:
    `test_train_controller_param_dimensions.gd`, `test_train_universal_controller.gd`,
    `test_property_bindings.gd`, `test_train_ep_dynamic_brake_blending.gd`,
    `test_train_electric_engine_circuit.gd`, `test_train_engine_common.gd`,
    `test_train_brake_cntrl.gd`, `test_train_diesel_engine_mechanical.gd`,
    `test_train_controller_cntrl.gd`, `test_relay_list.gd`, `test_train_ai_hints.gd`,
    `test_train_engine_cntrl.gd`, `test_brake_pressure_table.gd`.
  * *A key's presence instead of its value*: `test_fiz_import.gd`, `test_fiz_train_controller.gd`,
    `test_train_electric_engine_cntrl.gd`, `test_train_speed_control.gd`, `test_train_heating.gd`,
    `test_train_brake_cntrl.gd`, `test_train_diesel_engine_mechanical.gd`,
    `test_train_electric_engine_power_source.gd`, `test_compressor_list.gd`, `test_cab_lights.gd`;
    `test_vehicle_state_bench.gd:158,173` assert `true` and belong outside CI.
  * *Physics without simulated time*: `test_train_*` (brake, engine, controller, switches, spring
    brake, speed control, EP fuse, heating, lighting presets), `test_traction_power_pantograph.gd`,
    `test_rail_vehicle_track_movement.gd`, `test_rail_vehicle_at_rest.gd`,
    `test_rail_vehicle_start_track.gd`, `test_rail_vehicle_idle_*_regression.gd`.
  * *Stops before what the player sees*: `test_driver_server.gd` (`Prepare_engine` only to
    `battery_enabled`, `:185` asserts an open main switch).
  * *Fixtures that build a 0 V vehicle unnoticed*: `test_vehicle.fiz`, `test_wagon.fiz`,
    `dynamic/test/synthetic_v1/synthetic.fiz`.
  * *Private members*: `test_maszyna_environment_node.gd`, `test_scenery_compiled_cache.gd`,
    `test_track_rendering_server.gd`, `test_weather_controls.gd`, `test_mmd_cabin_instancer.gd`,
    `test_cabin_spot_light_3d.gd`, `test_mmd_sound_bank_instancer.gd`,
    `test_train_sound_system.gd` and 8 more; hand-written state dictionaries in
    `test_brake_sound_model.gd:107,120,184`.
* **`test_zzz_ep07_cabin_main_switch` crashes or flips** (SIGSEGV in about half of the runs, core
  dump in `_free_owned_rids`): the headless dummy renderer's mesh storage is not thread safe and
  the streaming worker preloads models while the main thread loads the cab's. Decide: one lock in
  `MaszynaE3DModelManager.load_model()`, or load on the worker only with a real renderer. It also
  loads `scenery/td.scn` from the game dir - needs a fixture scenery.
* **A headless test run sometimes crashes at exit** (SIGSEGV inside Godot after the main loop,
  no symbols; `test_scenery_subscene_cache` 2 of 6 runs) - a debug build of Godot and a core dump
  to find which singleton frees out of order.
* **The EP07 fixture has no models**: its pantograph arms, the cab's visibility of the low-poly
  interior and the detail are not tested on it.
* **The EN57 start-up test failed once** after the EZT `Imin` default and passed four times
  since - watch it. Again 10-06 in `test_zzz_driver_hints_en57_2000_v1`, in a sequential run
  after nine other scripts: "line contactors on the first power position", the controller at 0
  after the key; green alone twice right after.
* **EN57-702ra drives without the battery and the main switch** (report): not reproduced on the
  fixture; needs the operator's scenery and a probe of the ra/s/rb start state.
* `test_train_electric_induction_engine.gd` fails 3 tests: `_powered_up_eim()` never closes the
  line breaker.
* Stary Jawor's eszelon at x10 runs ~20 km/h wanting 70 (controller jumps 0-7, `Ft` 0 half the
  time); at x1 it once stood 100 m short of E4 for ~17 s while 20 wagons released. The operator's
  stop with the local brake applied in full is not reproduced.
* **No regression test for the couplers stiffened by a long frame**: needs a free-rolling (or
  long pulled) trainset fixture, stepped at 0.017 s and at 0.17 s a frame.
* **No HUD panel test on a non-diesel** - one per engine kind (diesel, series, induction).
* **The turbo sound** (`turbo:`, `TurboPos:`) has no test of the sound itself.

## Physics performance

* The frame drop with a trainset in a scenery is the scenery's dynamic lights; nothing bounds how
  many are lit (`FINDINGS.md`, 2026-09-21). Measure the count first.
* Multi-core physics - not worth it as measured (`docs/findings-archive.md`, 2026-10-06 parked
  vehicles): only the sub-step loop of `MaszynaMoverVehicleServer::stepping_advance()` could run
  per island (a coupled trainset plus vehicles in collision range) on
  `WorkerThreadPool::add_group_task`, one island per task, one wait per slice - at most ~10% of
  the main thread. Blocked by the Mover's global `std::mt19937` (`utilities.cpp:35-36`, used by
  `Adhesive`, `ComputeMovement`, `CouplerForce`): per-thread engines need a change of the vendored
  Mover and give up bit-reproducible runs. Also to move off the loop first: the switch forcing in
  `_move_placement()` (writes TrackServer, emits signals) and `Curve3D` baking on first sample.
  The original's function is `vehicle_table::update()`, `DynObj.cpp:8686`.

## Data the original reads that the wrapper does not read yet

* **The destination sign**: `DestinationFind`, `pydestinationsign:` (133 MMDs),
  `destinationsignbackground:`, the lit sign on the low voltage (`DynObj.cpp:3044-3062, 6935-6962,
  7780-7830`).
* **Cab controls in the low-poly interior and lamps in the exterior model** (`Gauge.cpp:187`,
  `Button.cpp:59`): needs the cab logic to drive the low-poly and exterior instances.
* **Headlight dimmer** `ModernDimmer=`/`DimmerList:` (`Train.cpp:725, 5888-5939`): read, not
  applied (`SetLightDimmings`, DynObj.cpp:7298-7358); no vehicle uses it.
* **`.flac`** sounds: Godot reads no FLAC; no game data uses it.
* **`eimscreen: i j`**: no gauge reads `fEIMParams` (no data uses it).
* **`Cntrl.` keys no FIZ parser reads** (`LoadFIZ_Cntrl`, Mover.cpp:10707; grep of every
  `extract_value()` key against `addons/libmaszyna/fiz/`, 2026-10-06 - each to be checked,
  some may be read under another spelling): `BackwardsBranchesAllowed`, `BBHT`, `BDelay` (the
  plain one), `ConverterOverloadWhenMainIsOff`, `DBAM`, `DBPN`, `DCDPP`, `DCMB`, `HAO`, `HGDP1`,
  `HGDP2`, `HideDirStatusSpeed`, `HideDirStatusWhenMoving`, `HMO`, `IBTB`, `MaxTachoSpeed`, `OMP`,
  `OPD`, `SBBBH`, `SBD`, `SCIM`, `SplitEDPneumaticBrake`. `MaxBPMass` was one of them (EN57's
  trailers braked at MaxBP and locked their wheels, `docs/findings-archive.md` 2026-10-06).

## Grass and trees vanishing up close (report 2026-10-05)

* Models: whether a model's node range (`MaszynaE3DOptimizedBackend.cpp:226-234`) still goes by the
  submodel's own centre instead of the model origin (`opengl33renderer.cpp:2935-2942, 3617, 3654`).
* Measure at the reported spot (Linia053_Wrzosy, the 36WE) before changing it. The triangles were
  fixed 2026-10-12 (`docs/findings-archive.md`, "Całkowo's ground missing under its houses").
