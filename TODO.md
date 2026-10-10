# TODO

## Split from the game

* `demo/examples/mover_demo.tscn` and `cabin_demo.tscn` point at `res://vehicles/sm42/*`, deleted in
  af72fa4c0 (`cabin_demo` also at the missing `environment/sky.gd`) - the simple Mover demo the
  demo is to keep does not load.
* The core input actions live in both `demo/project.godot` and the game's `project.godot`: Godot
  keeps no input map per addon.
* A vehicle (`MaszynaRailVehicle3D`, `TrainSet3D`) is not rebuilt on
  `GameDataServer.data_reload_requested` as models, cabs, tracks and materials are: a game directory
  set while `demo_3d` runs leaves its vehicles unbuilt until the scene starts again.
* Tests, the demo and the game share one user directory (`MaSzyna-Reloaded`): a test setting
  `maszyna/game_dir` to the fixtures and restoring it in `after_each` undoes a directory the
  operator sets while the test runs.
* libsimulator: the MaSzyna-independent core (servers, vehicles, player, HUD state) split out of
  libmaszyna.

## Architecture rework (#184)

Each stage is one PR, titled `(#184) <area> - <what>`, and leaves the game runnable.

* **B** - the five common values (`velocity`, `speed`, `mass_total`, `total_distance`,
  `direction`): only the first two have a server getter.
* **C** - `vehicle_component_create` does not exist: components still come from
  `ClassDBSingleton::instantiate()`/`memnew`. `vehicle_generic_component_find` has no callers.
  `GenericVehicleComponentNode` finds its vehicle via `get_parent()` - turning it round changes
  how a modder authors a component, to be decided first. A script component still publishes
  through `_get_component_state` instead of being walked for its variables
  (`PROPERTY_USAGE_SCRIPT_VARIABLE`). The dump mixes nine `prefix/key` namespaces with flat
  `component_key` names.
* **D** - a vehicle assembled by hand out of nodes is still built by `VehiclePhysicsNode`;
  registering the name and the commands still hangs off `attach_to_system()`.
* **E - not started.** None of the ~20 proxy nodes; no `VehicleControllerNode`.
* **G** - 9 cabin scripts still take the whole dump; `cabin_windscreen_wipers.gd` does it per
  frame.
* **H - not started.** No `UpdatePhase`; the order is hand-written in
  `MaszynaMoverVehicleServer::stepping_advance()`. Check it against
  `TMoverParameters::ComputeMovement`/`Update` and the three ordering bugs on record (#57 line
  breaker, `Mred`, `roof_light_enabled`). A `test_vehicle_doors.gd` must exist first -
  `RailVehicleDoors` ticks and has no test.
* **Scenery teardown vs. streaming** (`FINDINGS.md`, 2026-09-22): the preload
  (`e3d_model_manager.gd::load_model`, a full `ResourceLoader.load()`) creates renderer resources
  off the main thread, so a teardown overlapping a running stream can still race. Remedy: parse on
  the worker, build on the main thread. Reproduce by clearing `user://cache/rail_vehicle` and
  `fiz` and running `test_zzz_ep07_cabin_main_switch`.
* **Fixture .fiz files differ from the current game data**: `sr61v1.fiz` lacks
  `DirChangeMaxPos=2`, the ED72/EN57 cars have `BM=P10-Bg BSA=16` where the game has
  `BM=P10-Bgu BSA=5`. Decide whether to refresh them from the game dir (copy only).
* **The `.fiz` path has not been run in the game** since the components stopped being nodes -
  only in tests.

### Rail concepts in interfaces named "Vehicle"

Rail-only interfaces under generic names (rail-term count per header): `RailVehicleBrake` 43,
`RailVehicleElectricEngine` 34, `RailVehicleBuffCoupl` 13, `RailVehicleWheels` 12,
`RailVehicleSecuritySystem` 2, `RailVehicleSpringBrake`/`RailVehicleElectroPneumaticDynamicBrake`
1-3. Either rename to `Train*` (cost: `RailVehicleWheels` 13 files / 50 mentions,
`RailVehicleBrake` 24 / 283) or split a generic base from a rail subclass - only once something
road-side shares the base.

### What still reaches a class by name from C++

Our own classes still in GDScript, fixed when their base moves to C++:

| Class | Named accesses | Where |
| --- | --- | --- |
| `E3DModelInstance` | 3 | `get_e3d_instance`, `e3d_instance_created` (connect, disconnect) in `RailVehicle3D` |
| `TrackCurve` | 10 | `p1`, `c1`, `c2`, `p2`, `roll1`, `roll2` in `TrackServer` and `RailVehicleServer` |

### Readers still on the state dump

* `TrainSoundSystem._build_brake_events()` and `MaszynaBrakeSfxEventFactory.build_events()` read
  the config dump at build; the brake handle positions have getters
  (`RailVehicleBrake.get_handle_position()`).

### What still reaches the vehicle's controller

* Typed server getters to add: `VehicleServer` - `vehicle_get_direction`, `vehicle_get_power`,
  `vehicle_get_max_velocity`, `vehicle_get_mass_total`; `RailVehicleServer` -
  `vehicle_get_train_type`, `vehicle_get_coupler_stretched`, `vehicle_get_train_damage`; the load
  through its component. Move onto them the AI (`maszyna_legacy_driver_*`, `ai_driver`,
  `auto_rewident`, `station`), sound (`train_sound_system`, `brake_sound_model`,
  `running_sound_model`), `external_camera.gd`, `RailVehicleRenderingServer.cpp` (smoke, load) and
  the start-up tests' helper, so only assembling a vehicle keeps the controller
  (no `vehicle_get_controller(` outside assembly); `CODE_STYLE.md` still allows
  `controller.max_velocity`.
* The AI's fallbacks and copies of the Mover's defaults -> typed server getters, no invented
  values: `driver_braking.gd:799, 975, 982`, `driver_pantographs.gd:46, 70, 78`,
  `driver_traction.gd:133-134, 324, 330, 380-393, 479`, `ai_driver.gd:695, 874`.
* Where the original's AI bypasses the cab (`driverhints.cpp`: `mvOccupied->RelayReset()`, ...),
  the AI sends `VehicleServer.vehicle_send_command()`, not `CabinSystem.act()`: `fuse_bt` and
  `converterfuse_bt` (`MaszynaLegacyDriverHints.BUTTONS`).
* `LegacyCabinDirectionKey` (`legacy/cabin/direction_key.gd`) reads `direction` from the dump
  until there is `vehicle_get_direction()`.

### Source layout - the GDScript side

The MaSzyna-specific scripts of `addons/libmaszyna/` outside `legacy/` (e.g. `sound/maszyna_*`)
are still to decide and move (preload/`res://` paths and `.tscn`/`.tres` references follow).

## Player and HUD

The game's screens and HUD are the game repository's (`MaSzyna-Reloaded/maszyna-reloaded`), and
so is their open work.

* The developer console (`addons/libmaszyna/console/console.gd`) still reads keycodes for
  Ctrl+~ (size), Escape, the arrows, Page Up/Down and Tab - only its toggle is an action
  (`console_toggle`).
* `PlayerServer.player_enter_vehicle()` into a vehicle of the trainset the player drives (the
  game's vehicle card "Enter cabin") leaves the trainset with nobody at the controls: the driver
  the take-over made an observer gets the controls back only for another trainset
  (`trainset_left_drivers_take_control`), and a gangway walk back keeps the player an observer -
  the chip shows the vehicle unmanned until Q, F5 or the chip (reports#16). Left as it is on the
  operator's decision; what an observer visit should do to the trainset is open.

## Cabins

* **Next, right after the split commit (operator, 2026-10-09):** the occupancy layer drives the cabs
  by plain vehicle commands instead of `set_driver_cabin_kind()` (`RailVehicleServer.cpp:1007`):
  `cabin_activate` (today's `cab_activation`), `cabin_change` (`ChangeCab()`, Mover.cpp:736) and,
  if decided, `cabin_occupy`. Every `cab_*` command renamed to `cabin_*` at once. Open: whether
  `CabOccupied` is the vehicle's control state (any sender may set it) or the persons' fact (only
  `RailVehicleServer` sends it) - the first changes `CODE_STYLE.md`, "A vehicle is commanded, not
  called". `mover_demo` then activates its cab without a seated person.

* **Cab elements run on a clock of their own** (`SimulationClock`, the simulation's time in their
  `_process`): every `BaseCabinTool3D` (buttons, switches, knobs, gauges, blinkers), the lamps and
  lights (`CabinIndicator3D`, `CabinSpotLight3D`, `CabinOmniLight3D` - which also poll the vehicle
  every 0.1 s) and the windscreen wipers. Their state belongs to the simulation step (#301): taken
  on an event of `CabinSystem` after a step and after a command; what they show - a lever's
  travel, a lamp's blinking, a light's fade - is the presentation, interpolated between the
  previous and the current step's state in the render frame, as #301 lays out for the vehicles.
* **Gauges that gate their command but have no catalog entry yet** - when ported, they get
  `requires_gauge` (Train.cpp line in brackets): `antislip_bt` (2232), `nextcurrent_sw` (1584),
  `signalling_sw` (2602), `converterlocal_sw` (4463), `compressorlocal_sw` (4629).
* **`MaszynaMaterialFactory._get_shader_variant()` builds its shader variants with
  `code.replace()`** (alpha blend, cull disabled, specgloss): to be static variant files with the
  render modes they have now (`CODE_STYLE.md`, "A shader variant is a file"). A vehicle's glass
  uses the blended variant, so the first vehicle near the camera compiles it in the game.
* **A translucent skin submodel is forced whether or not the skin has an alpha channel**
  (`E3DInstanceBackend::_submodel_translucency()`); the original draws it in the alpha pass only
  when it has one (`DynObj.cpp:331-346`, `textures_alpha`).

### Controls not in the cab yet

Each needs a catalog entry and, where missing, a vehicle command (with its `type()` branches):
`epbrake_bt` (ggEPFuseButton, Train.cpp:2350), `doorrightpermit_sw` (Train.cpp:7263),
`compressorlist_sw`, `autosandallow_sw`.

### E186 controls - what is still simplified

* The E186 screen's pantograph page (`traxx_renderer.py`) toggles its "odbiornik prądu"
  1 / 2 / 1+2 without OP1/OP2 being pressed - not checked against the original.
* `MoverRailVehicleEnginePowerSource::pantograph()` opens the master valve itself when a
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

### Mouse operation (CabinHUDMouseSystem) - not ported from drivermouseinput.cpp

* Absolute slider for the `*set` levers: `mouse_slider` maps 60% of the window height onto the
  whole range (drivermouseinput.cpp:27-155).
* Right button as the control's second binding (decrease), panning only off a control
  (drivermouseinput.cpp:405-414).
* Repeat rate growing with the cursor's distance while held (drivermouseinput.cpp:437-443,
  482-484), and the Shift "fast" variants (:418-428).
* Debug-mode tooltip with the submodel name (drivermode.cpp:377-382).
* Key hints only for the first widget of a label (mmd_cabin_instancer.gd:396).
* **Brake valve drag direction - cause not found.** `brakectrl` forces its signs in the catalog
  (`"mouse_drag_signs"`) because the grip heuristic (`_increase_signs`) got SM42's valve backwards
  in game while a headless check predicted it right. Find what differs in game (6d/6d1/6da have
  opposite `rot` signs, the occupied cab, the cab's transform), then drop the force.
* Captions are taken when a control is built - a language change shows after re-entering the cab.
* Hand-authored cabin scenes register no occluders.
* EP07: round buttons get a rectangular outline. First dump the button's submodel (AABB, faces,
  alpha texture): a quad with an alpha-tested texture outlines as its quad.

### Python integration

* **Windows runtime tested under wine only** - `maszyna-python-host.64.exe` with Steam MaSzyna's
  `python27.dll` and `python64/` starts and renders a PIL screen; not yet on a player's Windows.
  `make python-runtime` builds only the Linux one.
* **`maszyna-python-host` in an export** (`[dependencies]` of `libmaszyna.gdextension`): not yet
  seen in an exported game; on Linux its executable bit must survive the export.
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
* `CabinSwitch` has no `mesh_rotation_offset`/`mesh_position_offset`, so the MMD offset is dropped
  (e.g. SM42 `dirkey: kier rot -0.09 0.01`, `Gauge.cpp:456`); `CabinButton` already has them.
* Rest of TDynamicObject::Update's driver block (DynObj.cpp:3240-3400): the ED/PN brake force
  split of an induction motor trainset, `EqvtPipePress = GetEPP()`, the unpowered-car copy of
  MainCtrlPos/SpeedCtrl (DynObj.cpp:3272-3276).
* Wheels turn at half speed: `MoverRailVehicleWheels` adds `rad_to_deg(V*dt/D)`, the original
  `rad_to_deg(2*V*dt/D)` (DynObj.cpp:3780-3784 at df5a8a8). Waiting for the operator.
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
* `LegacyCabinBattery`, `LegacyCabinCabActivation`, `LegacyCabinManualBrake`, `LegacyCabinWipers`
  only register what `LegacyCabinUnmodelledControls` registers anyway - fold them in.
* A tile's placeholder guesses its width (`TileGrid.PLACEHOLDER_STRETCH`): FIZ `Dim=` is parsed
  nowhere for `MaszynaSceneryInfo.Vehicle`.
* **Cab shake** (`Cabin3D::_process_engine_shake()`, `TDynamicObject::update_shake`,
  DynObj.cpp:8048-8137): the acceleration-driven base shake (`AccN`, `AccVert`, `AccSVBased` with
  `Global.ShakingMultiplier*`, `:8102-8109`) and the hunting shake (`:8083-8097`) are not ported,
  and the cab shakes only when its vehicle has a diesel engine - the original's base shake moves
  every cab (`:8064-8065` limits only the engine's vibration to a diesel).

## Translations

* Units (`km/h`, `bar`, `%d m`, ...) are not msgids.

## Sounds

* **The UI's sounds follow the simulation's speed too** (`AudioServer.playback_speed_scale`). To
  do: a pitch multiplier on `SfxPlayer`/`SfxPlayer3D` (vendored `gnd-sfx`,
  `sfx_playback_runtime.gd:1191`) set on the vehicles' and the weather's players, and the global
  scale left at 1.
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
* Brake sounds (`BrakeSoundModel`): a loop due while out of earshot restarts from its opening
  bookend (`sound.cpp:360-367`); pressure rates reset when a bank is silenced;
  `TrainSoundSystem._process()` counts the `fmod` remainder twice for far banks. Not compared by
  ear with the original.
* The gnd-sfx tick is GDScript on a worker (12 ms/frame for 200 players, headless). If it limits,
  move the runtime to a C++ singleton.
* Scenery sounds (`ScenerySoundServer`): a one-shot played out of reach is not heard when the
  camera arrives mid-clip; a loop restarts when it comes back into reach; `sound_create()` setting
  `max_tracks` rebuilds the voices; a loop asked for during a one-shot starts only when back in
  reach (`scene.cpp:148-152`).

## Vehicles

* **EN76 main pipe does not fill** (reports#12, l053_poludnie.scn, EN76-005a, local build): all
  four units with the main reservoir at 8.26 bar, `pipe_pressure` ~0, the hand brake released,
  the train brake handle of 005a at 0, of 005b-d at 1; the driving aid's hints contradict each
  other (screenshot in the report). Diagnose from `snapshot.json` against the original's EN76 FIZ.
* **Wagons with full cylinders show no brake force** (reports#13, l053_poludnie.scn,
  ES64F4-846 + 20 wagons, build 20261007-1333): all wagons `brake_air_pressure` ~3.95 bar with
  `brake_force` 0.0 while the train creeps on at 0.05-0.1 km/h; the loco itself 365 kN (hand brake
  20, independent brake full, handle 6). "182/183 do not brake" - check the wagons' `BrakeForce()`
  inputs (NBpA, BCN, the brake's friction) first.
* **Left of the move of the state to its owners**: `MoverRailVehicleBrake` fills the brake valve
  and the compressor for a wagon too; the master controller's, reverser's and cab's commands
  (`main/second_controller_*`, `direction_*`, `cab_activation*`, `cab_change`) and the cab
  activation config are still the controller's; `ground_relay_reset` and
  `cntrl_ground_relay_start_mode` are the controller's while the relays are the engine's;
  `RailVehiclePowerSupply.power_changed` has no listener; the driver's code looks a component up
  per call.
* **A vehicle's first build costs ~50-140 ms on the main thread** (headless, warm caches): the
  `instance_build()` of each model 12-40 ms, nearly all of it the materials created for the first
  vehicle of a type and skin (`MaterialManager.get_submodel_material()`: `.mat` parse, DDS loads,
  path resolution) - the same model built again with its materials held takes ~1 ms. A vehicle
  coming within the draw distance is one such stall a frame (`BUILD_BUDGET_MSEC`); moving the
  material and texture reads to a preload thread is the item below.
* **"has no skins set, but submodel requires material #3"** for `pkp/11xa_v2` and `pkp/14xa_v1`:
  check what the original draws for a replaceable skin index the skin has no file for, and warn
  once per vehicle type, not per submodel and vehicle.
* **Road vehicles return with roads:** a vehicle on an unregistered track (every road) is left out
  at load (`SceneryInstancer._attach_objects()`).
* **The stepping of parked vehicles** (Wrzosy, profiling build, after the neighbour scan cache):
  the step is 30.9% of the main thread - forces and movement ~8%, the remaining neighbour scans
  ~4% (vehicles near moving ones), placements reported to rendering ~5%. Parked EMUs/DMUs keep
  their physics on by the Mover's own rule (Mover.cpp:4487-4489).
* **WeatherNode costs ~16 ms a frame** on Wrzosy with one rain volume - measure inside.
* **Textures loaded on the streaming's preload threads**: every owner's preload parses the `.mat`
  and loads all its textures into `MaterialManager`'s cache, the build only assembles. Measure
  before and after with the Braniewo route probe under `gamescope --backend headless`; the disk
  cache removal is not measured yet either.
* **Braniewo station renders ~5400 draw calls and ~11 M triangles a frame** (~30 fps on an
  RX 580).
* **One `SfxBank` per vehicle type** instead of per vehicle: needs `random_choices`,
  `pitch_variation` and `start_fraction` per player in the vendored gnd-sfx.
* The vehicle build exceeds `MaszynaLegacyVehicleSystem.BUILD_BUDGET_MSEC` (~16 ms a vehicle).
* Vehicles and the editor:
  * "Edit FIZ" (`RailVehicleRenderingServer.vehicle_set_editable()`) shows the exterior and the
    low-poly interior as nodes; the passengers, attachments, coupler adapters and cargo stay
    instances, and nothing edited there is kept - the nodes are built again from the model.
  * Trainsets tab: "Show" puts marker nodes and selects them; a hand-made `TrainSet3D` lists no
    vehicles right after its scene is opened.
  * Vehicles tab (spawner): a vehicle is placed only by a track name and offset [m]; spawning at
    a kilometre needs the scenery's mileposts and a lookup from kilometre to track and offset
    (none exists - only `TimetableEntry.kilometre`).
  * Operator's direction: a toolbar tool "Make trainset" for 1+ selected `MaszynaRailVehicle3D`s
    puts them into a new `TrainSet3D`; vehicles that belong to a trainset are detached from it
    into the new one.
  * The HUD's "remove trainset" frees only vehicles built from MaSzyna data.
  * A vehicle coming within detail distance is built twice
    (`instance_attach_object_instance_id()` then `instance_set_instancer()`).
  * `compiled.nodes` still carries the Time/Config/Atmo nodes.
  * "Edit FIZ" logs errors: `cabin_python_screen.gd` and `maszyna_dynamic_train_cabin.gd`
    disconnect in `_exit_tree()` what they connected once; `!is_inside_tree()` transform reads -
    count again.
* A distant vehicle's low-poly interior keeps its baked emission:
  `instance_set_emission_energy()`/`instance_set_submodel_emission_energy()` reach only the NODES
  backend.
* The low-poly interior's compartment and corridor sections stay unlit (`DynObj.cpp:1334-1352,
  2425-2433`, CompartmentLights).
* No test for `RailVehicleRenderingServer`'s low-poly cab lights, nor for the lamps drawn from the
  lighting (`_update_lights()`).
* `MaszynaVehicleStructure` and `RailVehicleAppearance` - candidates for a better name.
* `RailVehicleServer.trainset_move(vehicle, distance)` takes a vehicle RID - rename (e.g.
  `vehicle_move_coupled`).
* A vehicle without a start track does not move by its own velocity (the KA car in `demo_3d`).
  Decide whether `RailVehicleServer` should move off-track vehicles.
* `RailVehicleLighting::LightEnd` duplicates `RailVehicleController::CouplerEnd`.
* The 8 m shift of a trainset's first vehicle on a track with `Event0`
  (simulationstateserializer.cpp:1050-1057).
* The `structure-vN` tag (`maszyna_rail_vehicle_3d_manager.gd`) is bumped by hand.
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
* `README.md`, "No simulation time is ever dropped": describes `step_frame()`, owed time and
  `MAX_PHYSICS_ITERATIONS`, none of which exists - rewrite from
  `MaszynaMoverVehicleServer::stepping_advance()` and `SimulationServer`.
* The vehicle selector lacks "Stop and repair", "Reset position", "Refill main tank" and "Rupture
  main pipe" (vehicleparams.cpp:268-287).
* The vehicle whose card is open is not marked in the world (idea: a diamond marker with its
  distance).
* A vehicle is clicked in free camera only while its model is detailed; its tooltip shows the
  name taken at registration.
* **Headlight properties nobody reads**: `RailVehicleLighting.head_light_color` and the normal
  and high-beam multipliers are parsed but not drawn - the original tints and scales the head
  lights by them (`rendering/lightarray.cpp:78-109`); the wrapper applies only the dimmed
  multiplier (`RailVehicleRenderingServer.cpp:1404`) and a fixed 1.0 otherwise
  (`E3DNodesBackend.cpp:110`), and has no high beam. `instrument_type` has neither a writer nor a
  reader.
* `RailVehicleBrake.friction_elements_per_axle` defaults to 1, the original's `NBpA` to 0
  (`MOVER.h:1607`), which decides `Mover.cpp:5225`.
* The switch blade is stepped on the wall clock (`TrackServer`, `Time::get_ticks_usec()`), the
  original's on the simulation's delta (`Track.cpp:1944`).
* **The second configuration pass runs only with the whole vehicle**
  (`VehicleController::apply_configuration()`): a component applied again alone - `set_enabled`,
  `mark_dirty`, `add_component` to a running vehicle, the wipers' count from the appearance - does
  not run `apply_vehicle_config()` (REQUIRED_CLEANING RC-045).

## Rendering

* **GPU 22 ms of a 33 ms frame** (RX 580, 3607 draw calls) - measure on `make compile-profiling`
  and split by pass before any hypothesis.
* **Mirror reflections (`PlanarMirror3D`) are smeared** (Impuls 36WEa, `td_impuls`). Not
  measured: TAA on the per-frame overlay, the mirror camera's framing/projection
  (`mirror_view_projection`), the texture size in game. Dump the mirror's viewport texture from a
  running game first.
* **A light's submodels have two managers**: `E3DRenderingServer` (`lights_state`) and the cab
  widgets (`CabinIndicator3D`, `CabinSpotLight3D`) writing `visible`; the widgets should ask the
  model.

### Self-illumination of light submodels

* `E3DModelInstance` nodes never light automatically: `_merge_lights_state()`
  (`e3d_model_instance.gd:214`) pushes `false` as an override for every light.
* A `colored` light submodel has no emission (`COLORED_MATERIAL` first).
* `lightcolors` do not tint the submodel (`SetDiffuseOverride`, `AnimModel.cpp:625`).
* Baked, not per frame: the original lights while `Global.fLuminance < fLight`
  (`opengl33renderer.cpp:3452`); `material_manager.gd:118` decides once.
* Emission 1.0 after AgX unmeasured - needs a rendered frame of the operator's scenery.

### Smoke emitters

* Vertical decay (`particles.cpp:365-380`): needs `Global.AirTemperature` (a `#define` in the
  vendored Mover), overcast and speed.
* `MaszynaEnvironmentNode.wind_direction` cannot express a vertical part.
* The culling box follows the emitter, not the plume (`opengl33particles.cpp:38`,
  `particles.cpp:284-291`).
* `min_inclination` dropped (only `smokesource_st45`).
* Lifetime per emitter, per particle in the original (`particles.cpp:132`).
* The "Modern" flipbook repeats visibly - needs variants.
* Smoke is lit by Godot's sun, not the original's daylight modulation
  (`opengl33particles.cpp:60-66`).
* Vehicle emitters are not switched off beyond `2 * BaseDrawRange * fDistanceFactor`
  (`particles.cpp:452`).

### Other

* A vehicle's detection area stays in the physics space while its node is out of the tree.
* E3D: VNT1, VNT4+ userdata and TRA1 are not read (no data uses them, `Model3d.cpp:1977`, `:2135`).
* T3D: `priorityLoadText3D` (`Model3d.cpp:1634`), the "banana" root (`Model3d.cpp:2447`), a
  material's `selfillum` (`Model3d.cpp:483`); `stars`/`point` not drawn; `hotspotpower:` /
  `light_energy` not passed by `E3DModelBuilder`.
* Vehicle headlamps and cab lights not measured for shadow acne at night.
* Skydome: an option to disable `light_angular_distance` (`FINDINGS.md`, 2026-09-20).
* Skydome installs the sun shafts effect twice at start ("Installed sunshafts compositor effect"
  printed twice before `(Re)Initialized`); the earlier one is removed, but the install runs twice.
  The D3D12 crash fix (`FINDINGS.md`, 2026-10-06) is unconfirmed on Windows until a player reports.
* **Windows: td.scn ends the game without a message** (RTX 4080 SUPER, Vulkan, build
  20261007-1333). The `--verbose` log ends at starting the Python interpreter and `--no-python`
  runs without fault (`docs/findings-archive.md`, 2026-10-09): Python now runs in
  `maszyna-python-host`, so the game survives it. **Still open: why it failed for that player** -
  the next build's `app.log` has the host's stderr and exit code; asked of the player meanwhile:
  `--verbose 2> stderr.txt` from `cmd`, the event log's faulting module and exception code,
  whether eu07.exe shows the Python screens, another Python 2.7 in the registry
  (`PythonCore\2.7`) or on `PATH`. Also unguarded: a pending vehicle build whose appearance was
  set to none - `_build_models()` reads the appearance without a check.
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
* Air temperature (`MaszynaEnvironmentNode.temperature`) is consumed by nothing - the vendored
  Mover has `#define Global_AirTemperature 15.f`.
* Other `config` entries dropped (`scenario.time.override/offset/current`, `Globals.cpp:356-385`).
* Include instancing (`grass.inc` included 24078 times): classify includes as `instanced`/`full`,
  cache in local space, invalidate by dependency list.
* `ResourceLazyLoader` shares only a held copy: a per-model load lock, and the loader holding the
  copy with a release for a dropped preload.
* Lazy loading off (default): a vehicle's cargo (`_build_load()`) and coupler adapters
  (`_update_coupler_adapters()`) still load through `model_load()` at their build, outside
  `ResourceLazyLoader` - their files change while the game runs. After a data reload a resource
  resident by registration is loaded again only by its next use.
* The subscene cache is used only by queued parsing; `parse_file()` reparses every include.
* Streaming keeps the track/traction instances and drops only meshes (~16k empty in `baltyk`).
* A track streams by the chunk of its first curve point, not its nearest point.
* Tracks and traction have no worker `preload`.
* A terrain chunk's material is looked up on the main thread (`.mat` parse, DDS decode, md5 key) -
  move to the worker preload once the streaming panel shows terrain dominated by it.
* The loading screen does not wait for the first pass (`pending_builds == 0`).
* Stream baked 1 km chunks (MultiMesh per mesh+material, merged triangles, ready track meshes,
  cached on disk) instead of pieces; switch blades stay outside.
* `MaszynaSceneryChunkRenderingServer` (deprecated) - remove: `SceneryStreamingServer` streams a
  triangle chunk itself.
* Memory: every model placement keeps a full `E3DInstanceData`; a subscene's terrain sink is kept
  whole until parsed; a terrain chunk holds arrays beside its mesh while built.
* Load model placements and packed `nodes` per 1 km chunk; needs events and signal heads to reach
  a model otherwise than by its RID. Read `[SceneryMemory]` first.
* Terrain triangle order follows the parse workers (may differ between conversions).
* `SceneryLoadMeasurement`'s per-stage peak reads `MEMORY_STATIC` only.
* Main-thread stalls: `TractionServer.network_build()`, `TrackServer.topology_rebuild()`
  unbudgeted; `ScenerySoundServer.sound_create()` rebuilds the bank per sound (O(n^2)) - needs an
  append in `gnd-sfx`.
* `[SceneryLoad]` lines not measured on a heavy scenery.
* In the editor streaming follows 3D viewport 0 only.
* A freed streaming camera is not noticed: `SceneryStreamingServer::camera_id` stays set,
  `streaming_has_camera()` stays true and the streaming stops where it was, silently.
* `maszyna_node_track_importer.gd` drops `road`, `river`, `cross`, `turn`, `table`
  (`Track.cpp:1554` on).
* Lamp head colour does not match its tinted pool - needs a flag in the `E3DMaterialResolver` key.
* An economy-mode merged light takes the max `energy`, not the sum.
* Scenery light brightness calibrated by eye (`scenery_light_*`).
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
* `SimulationServer.simulation_pause()` does not hold the environment clock, `TractionServer`,
  `TrackServer` switches and the smoke.

## Signalling (#296)

* **The isolated sections become the system's sources** (`SignallingServer.system_add_source`).
* **The logical aspect for trains**: a signalling implementation has only the lights, nothing it shows
  reaches a train (the driver reads the memcells).
* **`ls_Dark`/`ls_Home` from a `lights` event** (value 3, 24 times): no light following the
  daylight.
* **Semaphore arms** - the `animation` event on a named submodel (`Event.cpp:1569-1735`).
* **Telling signal heads apart** from other lit models (street lamps included).
* A scenery signal head's light states read `LIGHT_STATE_OFF` until its first aspect.
* `SignalHeadNode.model` set while the node is in the tree is not connected
  (`e3d_instance_created` is connected on `ENTER_TREE` only), and `EXIT_TREE` disconnects the new
  model instead of the old one.
* **`SignalAspect.lights` are plain numbers** in the inspector, not an enum.

## Scenario events

* **Track events**: the direction filter by the intended direction (`eventfilter`,
  `TrkFoll.cpp:117-121`); the placement point stands for the primary axle; delay <= -1 events
  queued on every move along the track (`TrkFoll.cpp:249-260`); a crewed vehicle is any DRIVER,
  the original's `Mechanik->primary()` one per trainset.
* **Occupancy counts vehicles, not axles** (`TrkFoll.cpp:88-91`).
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
* **Duplicate event names**: the later wins; the original joins them as siblings
  (`Event.cpp:2296-2349`).
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
  (`maszyna-architecture`, "Servers forward to components", 2026-10-09): `RailVehicleServer`
  couples (`vehicle_couple()`, `uncouple`, `is_coupled_by`, `is_coupler_automatic`, adapters,
  `vehicle_get_coupler_joinable_flags()`) through `RailVehicleController`/
  `MoverRailVehicleController`; the coupling logic moves to the server, executed by the
  `RailVehicleBuffCoupl` component (`MoverRailVehicleBuffCoupl` on the Mover), the per-end
  `AllowedFlag` (`BuffCoupl1/2`) and `control_type` with it. The same for walking through the cabins,
  managing trainsets and moving a vehicle. The AI then couples by a server operation instead of the
  `coupler_connect` command. Plan with the operator first.
* **`scenery_loaded` comes before the vehicles are placed**: `TrainSet3D` places its trainset in
  `_process` on the next frame (`train_set_3d.gd:119-140`), after `MaszynaScenery._load_content()`
  emitted `scenery_loaded` (`maszyna_scenery.gd:89`) - the player (`player.gd:104`), `world.gd`,
  `ScenarioKeyboard` and the tests read it as "built, coupled and placed". A `vehicle_find_vehicle()`
  right after it finds no neighbour. Needs a "scenario ready" event at its owner (operator: a task of
  its own; where it lives is to be decided - scenery node or a server).
* **Commands a vehicle does not have are sent and fail**: `fuse_reset` from the cab key (N, every
  cab, `mmd_semantic_catalog.gd:224`) and `security_cabsignal_trigger` from an SHP event
  (`MaszynaLegacyVehicleCommandAction.cpp:38`) on an SN61 - no electric engine, no security system;
  "Unknown command" in the log each time.
* Cab logic without the 3D cab: an AI caller must pass what a widget would have worked out (knob
  and switch limits, spring return, the horn's value); `LegacyCabinControls` parses the MMD with no
  random choices. The `brake_level_drive` `CabinCommand` node still carries `command`/
  `command_param`, only as the guard of its key.
* **Cab targets**: the motor car's ammeters, voltmeters and lamps (`mvControlled` in
  `update_gauges`) need `target` in `MmdSemanticCatalog`.

## Game data (GameDataServer)

* The build named at the top of `app.log` is `res://build_number.txt`, written by the last full
  build - a checkout built incrementally logs an older number than the code it runs.
* `MaszynaVehicleProfileManager._ensure_viewport()` is an `ensure_*` API: create the viewport
  where the manager is.
* An owner that is not a scenery's rebuilds its streamed pieces when the scenery reloads itself in
  the same reload - wasted clears and builds.

## Tests

* **`test_sun_shafts_compositor_effect.gd` is pending in CI**: it renders on a GPU, and the CI runs
  GUT `--headless` (no RenderingDevice). Locally it runs under `gamescope --backend headless`. A
  GPU runner (lavapipe) would make it a CI test; D3D12 itself is not covered anywhere.
* **Tests that switch the game dir** (`UserSettings.save_maszyna_game_dir()`) write the user's
  `settings.cfg`, and a crash skips the restore: `test_maszyna_rail_vehicle_3d_manager.gd`,
  `test_audio_stream_manager.gd`, `test_e3d_lights_state.gd`, `test_fiz_train_controller.gd`,
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
    `test_train_battery.gd`, `test_legacy_cabin_button_types.gd`,
    `test_legacy_cabin_cab_activation.gd`, `test_legacy_cabin_unmodelled_controls.gd`,
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
    `test_traction_power_sections.gd`, `test_rail_vehicle_track_movement.gd`,
    `test_rail_vehicle_at_rest.gd`, `test_rail_vehicle_start_track.gd`,
    `test_rail_vehicle_idle_*_regression.gd`.
  * *Stops before what the player sees*: `test_driver_server.gd` (`Prepare_engine` only to
    `battery_enabled`, `:185` asserts an open main switch).
  * *Fixtures that build a 0 V vehicle unnoticed*: `test_vehicle.fiz`, `test_wagon.fiz`,
    `dynamic/test/synthetic_v1/synthetic.fiz`.
  * *Private members*: `test_maszyna_environment_node.gd`, `test_scenery_compiled_cache.gd`,
    `test_track_rendering_server.gd`, `test_cabin_switch.gd`, `test_weather_controls.gd`,
    `test_mmd_cabin_instancer.gd`, `test_cabin_spot_light_3d.gd`,
    `test_mmd_sound_bank_instancer.gd`, `test_train_sound_system.gd` and 8 more; hand-written
    state dictionaries in `test_brake_sound_model.gd:107,120,184`.
  * *Harness*: CI runs on pull requests and tags, not on a push to `main`.
* **`test_zzz_ep07_cabin_main_switch` crashes or flips** (SIGSEGV in about half of the runs, core
  dump in `_free_owned_rids`): the headless dummy renderer's mesh storage is not thread safe and
  the streaming worker preloads models while the main thread loads the cab's. Decide: one lock in
  `E3DModelManager.load_model()`, or load on the worker only with a real renderer. It also loads
  `scenery/td.scn` from the game dir - needs a fixture scenery.
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

## CI

* The Linux library is built in the SDK container without ccache.
* The engine is built without Swappy and AccessKit, which the official builds carry.

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
  `extract_value()` key against `addons/libmaszyna/legacy/fiz/`, 2026-10-06 - each to be checked,
  some may be read under another spelling): `BackwardsBranchesAllowed`, `BBHT`, `BDelay` (the
  plain one), `ConverterOverloadWhenMainIsOff`, `DBAM`, `DBPN`, `DCDPP`, `DCMB`, `HAO`, `HGDP1`,
  `HGDP2`, `HideDirStatusSpeed`, `HideDirStatusWhenMoving`, `HMO`, `IBTB`, `MaxTachoSpeed`, `OMP`,
  `OPD`, `SBBBH`, `SBD`, `SCIM`, `SplitEDPneumaticBrake`. `MaxBPMass` was one of them (EN57's
  trailers braked at MaxBP and locked their wheels, `docs/findings-archive.md` 2026-10-06).

## Grass and trees vanishing up close (report 2026-10-05)

* Ranged triangles are merged per 1 km chunk and Godot measures `visibility_range` from the chunk
  AABB centre (`maszyna_scenery_chunk_rendering_server.gd:146-151`); the original merges in 250 m
  cells and measures from the shape's centre (`scene.cpp:834-837`, `opengl33renderer.cpp:2866-2873`).
* Models: whether a model's node range (`E3DOptimizedBackend.cpp:196-205`) still goes by the
  submodel's own centre instead of the model origin (`opengl33renderer.cpp:2935-2942, 3617, 3654`).
* Measure at the reported spot (Linia053_Wrzosy, the 36WE) before changing either.
