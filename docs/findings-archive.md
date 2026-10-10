# Findings - archive

The full entries behind the rules in `FINDINGS.md`: the symptom, what proved the cause, the fix,
and the rule. Headings keep their date and title, because comments in the code cite them
(`see FINDINGS.md, 2026-09-23`). Open work belongs in `TODO.md`, not here.

## 2026-10-09 - Mover configured five times, spring brake released

* **Symptom:** none reported directly - the review of `REQUIRED_CLEANING` (RC-004) and the operator's
  concern that a Mover initialised twice ends in a wrong state (brakes, other functions).
* **What proved it:** traced per scenery vehicle: CheckLocomotiveParameters 3x, every component's
  `_apply_configuration` 5x (direct + again through `simulation_configured`, and a fifth time on the
  first step because `controller_configure`'s `duplicate_deep` marked every component dirty).
  `test_vehicle_build_configuration.gd`: an E6ACT standing at load had its spring brake released;
  its first step announced 14 configuration changes.
* **Cause:** passes after the last CheckLocomotiveParameters undid it (spring brake, brake load flag
  and delays, `CntrlPipePress`); the fifth pass rewrote `MainCtrlPos`, `BatteryVoltage`, the current
  collector, door permits over what had been done since the build. `AssignLoad` ran after CLP.
* **Fix:** one pass in the original's order (DynObj.cpp:2020-2075); components no longer re-applied
  through `simulation_configured`; `apply_config()` consumes the component's `dirty`. Three tests
  that added components to a built vehicle and relied on the extra passes build through a
  description now, as the game does; one applies its changed setting with `apply_config()`.
* **Rule:** the backend is configured once, CheckLocomotiveParameters last; a test builds through
  the description.

## 2026-10-09 - SN61 coupled for ever on `Shunt -3 -99`

* **Symptom:** calkowo, SN61 at night: SN61-02's AI stood by the train it was to couple to, sent
  `coupler_connect 1` (and `universal_brake_button`) about every 0.5 s from 2386 s on and never took
  its next order (the operator's `ai.log`, screenshots at Tm7).
* **What proved it:** the scenery's `Shunt -3 -99` (`events_noc_zimowa.ctr`, `events_os.ctr`, many
  times) asks for 99 = coupler | brakehose | mainhose | heating; the SN61 has `AllowedFlag=39`, no
  heating. `test_zzz_driver_shunt_coupling.gd`: joined by coupler and both hoses within a few
  seconds, then `coupler_connect` again and again in CONNECT.
* **Cause:** `MoverRailVehicleController::coupler_connect()` joins only what both couplers allow,
  and `_is_coupled_as_asked()` waited for all of 99. The original's `Attach(..., iCoupler)` sets
  `CouplingFlag = iCoupler` whatever `AllowedFlag` is (Mover.cpp:576-583) and compares it with
  `iCoupler` (Driver.cpp:7028) - it never waits, at the price of a heating line on a vehicle without
  one (a quirk, not ported).
* **Fix:** `RailVehicleServer.vehicle_get_coupler_joinable_flags()` - what both ends allow, the
  control line only between equal control types, used by `coupler_connect()` too; the driver waits
  for the asked couplings it can join. `DriverSystem.driver_order_changed` lets the test follow the
  driver by events.
* **Rule:** what a scenery order asks of the couplers is masked by what the pair can join.

## 2026-10-09 - SN61 stood on its hand brake after the player took it on scenery_loaded

* **Symptom:** `test_zzz_driver_hints_sn61_v2.gd` red since 2e6e863: SN61-02 started by the
  hints, master controller at 8, Ft = -57 kN, yet `speed` 0 with `brake_force` 107 840 N and
  empty cylinders.
* **What proved it:** the test run with the state logged per step: `brake_manual_position` was
  20 (ManualBrakePosNo) before the first step and stayed there. 85 kN (MBF) x BCN 3 x friction
  0.42 = 108 kN, the force measured.
* **Cause:** `CheckLocomotiveParameters(ReadyFlag = fVel != 0)` (DynObj.cpp:2034) puts every
  standing vehicle on its full hand brake (Mover.cpp:8946). Only `AutoRewident()` releases it,
  and only on vehicles nobody drives by hand (`AIControllFlag || d != pVehicle`,
  Driver.cpp:2193; `MaszynaLegacyDriverBraking._set_brake_delays()`). The original cues no
  "manualbrakoff" hint. Before 2e6e863 the player took the vehicle a frame after
  `scenery_loaded`, after the AI's first update had released it; since then the player is in the
  cab first. The game matches the original; the test followed the hints only.
* **Fix:** the test releases the hand brake by its key before moving off, as
  `MaszynaStartupTest.run_startup()` does.
* **Rule:** a vehicle braked with empty cylinders - read `brake_manual_position` first; a test
  that drives a vehicle taken from the start releases its hand brake itself.

## 2026-10-01 - style-check red behind a green local check

* **Symptom:** `style-check / clang-tidy` failed on the PR with
  `llvm-prefer-static-over-anonymous-namespace` (RailVehicleRenderingServer.cpp),
  `readability-avoid-nested-conditional-operator` and `readability-math-missing-parentheses`
  (MoverRailVehicleLoad.cpp), while a local clang-tidy run on changed files reported nothing.
* **What proved it:** the local clang-tidy was LLVM 18; CI runs 22.1.4 (`LLVM_VERSION`). The three
  checks do not exist in 18, and 18 also refuses the `.clang-tidy` key `RemovedArgs`. clang-tidy
  22.1.0 from PyPI reported the same findings, and none after the fix.
* **Fix:** a static function out of the anonymous namespace, an if/else for the nested
  conditional, parentheses around the divisions.
* **Rule:** run clang-tidy of CI's major version before pushing C++.

## 2026-09-30 - EP07's brake valve handles could not be grabbed

* **Symptom:** in the EP07 cab the end of the main brake valve's handle (`brakectrl`), of the
  independent brake's (`localbrake`) and the reverser's (`dirkey`) could not be taken with the
  mouse.
* **Cause:** "a cab control is its own submodel only" (the entry above, E186's op12) took from
  every control the meshes under it, the handle among them - the handle (`raczkaKranu` on SM42)
  is a submodel under the valve's own. Not measured on the EP07 model: the game data is not in
  the session that made the fix.
* **First fix, wrong:** only `CabinKnob` took its handle - a rule by widget class left the
  reverser (a `CabinSwitch`) without it. A class or a cab is no criterion; the model's tree is.
* **Fix:** `CabinHUDMouseSystem.control_create()` takes a control's mesh with every mesh under it,
  unless another control's mesh lies under it: then the control is a panel and its own mesh only
  (E186's universal1 holds op1/op2, so op12 on it is nobody's). Whichever registers first, the
  outer control gives up its children when the inner one comes. Tests:
  `test_child_mesh_is_part_of_the_control`,
  `test_control_holding_another_control_is_its_own_mesh_only`.
* **Rule:** a control is its mesh and its subtree, a panel (a control over another control) its
  own mesh only.

## 2026-09-30 - the torch put out the signals

* **Symptom:** switching the player's torch on made the signal lights and the cab's emissive
  displays (the radio's) disappear.
* **Cause:** `headlamp_glow.gdshader`, a full-screen quad, wrote `screen * (1 + glow)` over the
  whole screen. `hint_screen_texture` is copied after the opaque pass, before translucent geometry
  is drawn, and the quad (nearest, drawn last) put that copy back over everything translucent -
  outside the cone as well, where the glow is 0. Read off Godot's pipeline, not measured in-game.
* **Fix:** `blend_add`, and the pass writes only the light it adds (`screen * boost * glow`).
* **Rule:** a full-screen pass that reads the screen texture only adds to the screen.

## 2026-09-30 - a test ran on the stack of the signal it awaited

* **Symptom:** a test spawning a vehicle and awaiting its `vehicle_built` left the vehicle alive
  after the test: GUT reported "3 unfreed children" and the next test's vehicle of the same name
  "two vehicles named ...".
* **What proved it:** a print around `vehicle.free()` in `after_each()`: "Object is locked and can't
  be freed", with `after_each` called from the test's own frame. `await signal` resumes the test
  inside the emission - here `MaszynaRailVehicle3D._process()` emitting `vehicle_built` - so the rest
  of the test and its `after_each()` ran on the vehicle's stack, where the vehicle cannot be freed.
* **Fix:** `MaszynaGutTest.spawn_maszyna_vehicle()` waits one frame after `vehicle_built`.
* **Rule:** code resumed by `await` of a signal runs inside the emitter; it may not free the
  emitter.

## 2026-09-30 - reordering a trainset hung the editor

* **Symptom:** dragging a vehicle of `ImpulsTrainset` to another place among its siblings in the
  editor froze it; a headless probe doing `move_child()` on the trainset never reached its next
  frame.
* **What proved it:** the probe stopped after the reorder, the trainset placed anew. Before the fix
  `RailVehicleServer::trainset_place()` coupled the new pairs on top of the old ones: the vehicle
  moved to the front coupled to the old first one, whose pairs were still there - a ring. The next
  walk along the trainset (`vehicle_get_coupled()`, from the couplers' drawing) never ended.
* **Fix:** `trainset_place()` uncouples every coupling between vehicles of the trainset before it
  couples them in the new order (`test_train_set_3d.gd` asserts the trainset open at both ends).
* **Rule:** a trainset placed again in another order lets go of its old pairs first.

## 2026-09-30 - couplings undone by the second configuration

* **Symptom:** coupling a trainset on `vehicle_placed` coupled nothing that lasted.
* **What proved it:** the order of the build: `VehiclePhysicsNode` built an empty vehicle on entering
  the tree, then `MaszynaRailVehiclePhysicsNode` gave it its `.fiz` controller from a deferred
  `_reload()`; `controller_configure()` restarts the vehicle - `release()` - and every coupler is
  cleared. The vehicle was placed after the first, empty configuration, so whatever coupled it on
  that event was undone by the second. The per-frame wait in `_wait_for_vehicles()` hid it.
* **Fix:** a vehicle is configured once: the controller is given before the node enters the tree
  (the instancer sets `controller`) or built by the node as it enters (`_build_controller()`); the
  deferred `_reload()` is gone.
* **Rule:** a vehicle is configured once, before anything can see it.

## 2026-09-30 - a builder's paths saved into the scene

* **Symptom:** with `MaszynaRailVehicle3D` made a `RailVehicle3D` subclass, everything its instancer
  set on the vehicle - `controller_path`, the part paths, `cabin_scene` - would have been saved with
  the scene and the scenery cache, and a saved `controller_path` to a child built at run time would
  have held the loaded node waiting for a controller nobody built.
* **What proved it:** the exported properties of a native base are the subclass's own - storage
  follows them; the instancer wrote them on the node.
* **Fix:** nothing a builder builds goes through the node's properties: the MaSzyna instancer hands
  `RailVehicleRenderingServer` the vehicle's appearance and `CabinSystem` its cab by the vehicle's
  handle; `RailVehicle3D`'s paths are for vehicles assembled by hand.
* **Rule:** a builder hands the servers what it built by the node's handle, never through the node's
  exported properties.

## 2026-09-30 - a script subclass shadows the native lifecycle

* **Symptom:** a GDScript `extends RailVehicle3D` defining `_enter_tree()`/`_ready()` would have
  replaced `RailVehicle3D`'s own, which subscribed to the tracks and took the vehicle.
* **What proved it:** Godot calls a virtual on the script instance first; the extension class's
  override of the same name is not reached.
* **Fix:** `RailVehicle3D` does its work in `_notification(ENTER_TREE/EXIT_TREE)` (as `Cabin3D`
  does), and binds nothing under `_process`.
* **Rule:** a native node a script may subclass does its own lifecycle work in `_notification()`.

## 2026-09-30 - vehicles stood off their tracks in the editor

* **Symptom:** after `RailVehicle3D` stopped creating its own RID (RC-026), the vehicles of
  `demo_3d` stood off their start tracks in the editor, and snapped onto them only when
  `start_track_offset` was touched. The game was fine.
* **What proved it:** `test_rail_vehicle_start_track.gd` - tracks announced (`tracks_changed`)
  before the vehicle is built fails without the fix and passes with it. `_apply_start_track()`
  cleared `pending_start_track_retry` and called `vehicle_set_track(RID(), ...)`, which the server
  ignores silently. In the game the vehicle is built on entering the tree, before the tracks; in the
  editor only once the `.fiz` is read, after them.
* **Fix:** the placement stays pending until the node has its vehicle; the vehicle's arrival
  (`_on_vehicle_changed()` -> `_process_dirty()`) places it. Since `RailVehicle3D` became a proxy
  with no tick: `set_vehicle()` or `tracks_changed`, whichever comes last, calls
  `_place_on_start_track()`.
* **Rule:** an action that needs two things is spent only when both exist - a call with an invalid
  handle is ignored without a word, and the flag that said "still to do" is gone.

## 2026-09-30 - the Mover server freed the Movers under live controllers

* **Symptom:** twelve `Failed to retrieve non-existent singleton 'MaszynaMoverVehicleServer'` at
  every exit, once the Movers belonged to the server.
* **What proved it:** the probe's exit log. The extension unregisters and deletes the
  implementation server - and with it every `TMoverParameters` - before the scene lets go of the
  controllers; `release()` then walked `mover->Couplers` of a freed Mover.
* **Fix:** the controller keeps the implementation it took the Mover from as an `ObjectID`; gone,
  the Mover went with it and nothing is read.
* **Rule:** what points into another owner's memory asks for that owner by `ObjectID`, not by the
  singleton's name - at teardown the name is gone first.

## 2026-09-30 - "Edit FIZ" disconnected what was never connected

* **Symptom:** the Edit FIZ probe printed `Attempt to disconnect a nonexistent connection` for
  `vehicle_trainset_changed`, 24 times, from `maszyna_auto_rewident_node.gd`.
* **What proved it:** the stack of each error: `_exit_tree()` disconnected on every exit, while
  `_ready()` - where it connected - ran only the first time; Edit FIZ takes the vehicle out of the
  tree and puts it back.
* **Fix:** it subscribes in `_enter_tree()`. The cabin scripts (`cabin_python_screen.gd`,
  `maszyna_dynamic_train_cabin.gd`) have the same asymmetry - in `TODO.md`.
* **Rule:** subscribe where you unsubscribe - `_enter_tree()`/`_exit_tree()` run on every
  re-entry, `_ready()` once.

## 2026-09-30 - a stored vehicle description came up half a vehicle

* **Symptom:** a description (the FIZ cache, a `.tres`) - a `VehicleController` with its
  components, never started - had its components joined to it, its commands registered and the
  configuration signal connected.
* **What proved it:** review, then `test_fiz_import.gd` (`test_the_description_is_not_a_vehicle`).
  Loading a resource sets `components`, and the setter did `add_component()`, which joins.
* **Fix:** the setter stores the list; the components join in `attach_to_system()` and leave in
  `release()`. Also, `VehicleController` had its own `get_rid()`, shadowing `Resource.get_rid()`
  (clang-tidy): it overrides `_get_rid()` instead, as a `Mesh` answers its server handle.
* **Rule:** a `Resource`'s setter stores data; joining, registering and connecting happen where the
  object becomes live.

## 2026-09-30 - "Edit FIZ" aborted the editor

* **Symptom:** toggling "Edit FIZ" on a vehicle aborted the editor (SIGABRT).
* **What proved it:** core dump + `addr2line`: `Variant(const Object*)` in
  `RailVehicle3D::apply_track_placement()` (`bogie_rest_global_bases.get(bogie_node)`).
  `_apply_editable_in_editor()` re-adds the vehicle; `E3DModelInstance._exit_tree()` freed the
  submodels without `e3d_loading`, so `RailVehicle3D` kept raw pointers to freed bogie nodes.
* **Fix:** `E3DModelInstance` announces `e3d_loading`/`e3d_loaded` where its submodels are freed
  and built (`_free_instance()`/`_create_instance()`); `RailVehicle3D` holds every scene node it
  does not own as an `ObjectID` and reads the vehicle's components from `RailVehicleServer` by RID.
  The toolbar no longer set `editable_in_editor` on `FizVehiclePhysicsNode`, which has none - the
  script error had left the vehicle shown.
* **Rule:** a node in the scene tree keeps no pointer to another object.

## 2026-09-30 - a raw pointer returned to GDScript freed the vehicle

* **Symptom:** after `VehicleController`/`VehicleComponent` became `RefCounted`, demo_3d crashed
  within a hundred frames (`vehicle_send_command: controller is null`).
* **What proved it:** a probe counting references: four through `VehiclePhysicsNode`, and after
  three `RailVehicle3D.get_controller()` calls from GDScript the controller was freed. A bound
  method returning a raw `T*` of a `RefCounted` takes a reference from it.
* **Fix:** every bound method returning one returns `Ref<>` (`get_controller()` of
  `RailVehicle3D`, `VehicleComponent`, `GenericVehicleComponentNode`, `get_coupled_controller()`,
  `vehicle_component_get()`).
* **Rule:** a `RefCounted` crosses a binding as `Ref<>`, never as a raw pointer.

## 2026-09-30 - the simulation clock never ticked in demo_3d

* **Symptom:** demo_3d run as the main scene: SU45's battery switched on with no effect -
  `battery_voltage` stayed at 110 V, `power24_available` false, no CA/SHP, no pumps; horns only
  from the debug panel. demo_scenery_loading worked. A headless probe loading demo_3d with
  `change_scene_to_file()` worked too.
* **What proved it:** the log's `Parent node is busy setting up children, add_child() failed` at
  `maszyna_environment_node.gd:218` - `SimulationServer.clock_hold()`. The first hold created the
  `SimulationClock` node and added it to the root while the root was adding the main scene; the
  add failed, but `clock_id` was set, so `_refresh_clock()` took the clock for running and never
  tried again. Nothing was ever stepped. A probe adding demo_3d to the root in `_initialize()`
  reproduces it.
* **The node's reason was wrong:** it existed because `process_frame` was taken to come after the
  nodes (2026-09-24). Measured with a node of priority -100, a node of priority 0 and a
  `process_frame` handler: the signal comes first, every frame.
* **Fix:** no clock node. `SimulationServer` connects to `SceneTree.process_frame` while its clock
  is held and not paused, and disconnects otherwise (as `TractionServer` does).
* **Rules:** a C++ singleton ticks on `process_frame` - nothing to add to the tree; and a state
  flag set before an operation that can fail says the operation happened when it did not.

## 2026-09-29 - the E186 screen's OP1/OP2 turned its page off

* **Symptom:** on the E186 screen's pantograph page (universal1), OP1/OP2 did nothing but bring
  the main page back; hovering any button of the panel lit them all, captioned "element ruchomy".
* **What proved it:** a probe on the real cab: `op1`/`op2` (pantfront_sw/pantrear_sw) are
  submodels under `opcje_panto` (universal1) - pushing universal1 turned the panel and moved them
  into view. `CabinHUDMouseSystem::_pickable()` gave a control its mesh and every mesh under it,
  so universal1 held op1's triangles too and took the cursor and the click for it.
* **Fix:** a mesh belongs to the nearest control above it - a control leaves out another
  control's mesh with everything under it, whichever is created first. Test:
  `test_a_control_under_another_control_is_its_own`.
* **Rule:** a control's pickable ends at the next control's submodel below it.
* **Superseded 2026-09-30:** the third button of the panel, `op12` (pantographs 1+2), has no
  control in `base.mmd.inc`, so it stayed one of universal1's meshes: hovering it lit universal1,
  a click pushed it. The original never gives a control its children: the pick pass colours every
  submodel on its own (`opengl33renderer.cpp:3756-3759`) and `control_mapper::find()`
  (`Train.cpp:64-76`) matches only the control's exact submodel - `op12` is nobody's control there
  and only hides what is behind it. Fix: `CabinHUDMouseSystem` takes a control's own mesh only;
  every cab mesh is an occluder already. Test: `test_child_mesh_is_not_part_of_the_control`.
  Rule: a cab control is its own submodel only.

## 2026-09-30 - E186's spring brake release could not be clicked

* **Symptom:** right after the fix above, the spring brake release (`springbrakeoff_bt`,
  `sw_ham_post_wyl`) no longer reacted to the mouse.
* **What proved it:** a probe on `kabina_1_160.e3d`: `sw_ham_post_wyl` has a child
  `przycisk4444444`, `sw_ham_post_wl` the lamps `sprezynowy_on/off` - same size, 1.6 mm in front,
  flagged translucent (0x20). Until then they were the control's own meshes; now only occluders,
  nearer than the button. Every cab mesh was an occluder, translucent ones included, while the
  original's pick pass draws only opaque submodels (`Render_cab(..., Alpha = false)`,
  `opengl33renderer.cpp:1208`, `iAlpha & iFlags & 0x1F` at 3674).
* **Fix:** `E3DRenderingServer.instance_get_opaque_meshes()`; the cab takes its occluders from it.
  Test: `test_opaque_meshes_leave_out_translucent_submodels`.
* **Rule:** only an opaque submodel hides a cab control from the mouse.

## 2026-09-29 - a rebuilt cab showed the E186 screen's page button off

* **Symptom:** the E186 screen (`traxx_renderer.py`) switched to its pantograph page from the
  rightmost button under it (`universal1`), and then the button "did not work": the page stayed,
  or came back.
* **What proved it:** a headless probe on the real E186 cab
  (`MaszynaRailVehicle3D` + `show_cabin()`), printing per frame the widget's `pushed`,
  `CabinState.get_value(&"universal1")` and `PythonScreenState.compose()["universal1"]`. A click
  set all three; after `hide_cabin()`/`show_cabin()` the new widget read `pushed=false` while the
  cab and the screen still held `true`, and the next press sent `true` again - no change.
  `CabinButton._update_state()` read a control with no `state_property` as `false`, not as what
  the cab holds. Before the fix of the same day (a cab built on a running train) that `false` was
  also sent, and turned the page off at every rebuild.
* **Fix:** a button with no vehicle state behind it shows `CabinSystem.get_control()` of the
  occupied cab (`_apply_control_value()`). Test: `test_rebuilt_button_shows_what_the_cab_holds`.
* **Rule:** a control whose value lives only in the cab (`CabinState`) shows that value when it is
  built - a view rebuilt does not reset the cab.

## 2026-09-29 - the E186 screen showed no line voltage

* **Symptom:** the kV bar of the E186 diagnostic screen stayed at 0 under the wire.
* **What proved it:** the bar is `state['eimp_c1_uhv']` = `EngineVoltage` (Train.cpp:8723).
  `PythonScreenState` never filled the EIM row, and `engine_voltage` was published by
  `RailVehicleElectricSeriesEngine` alone - the Mover computes `EngineVoltage` for every engine
  (Mover.cpp:4542), and an induction motor had no key for it. `Itot` and `eimv[]` were published
  nowhere.
* **Fix:** `engine_voltage` and `total_current` on the traction motors unit (every electric
  engine and the diesel-electric), `force_max`/`force_full`/`field_current`/`motor_voltage` on
  the induction engine, and the EIM rows composed as Train.cpp:8712-8787 does.
* **Rule:** a Mover field every engine type computes is published by the part every engine type
  has, not by the first subclass that needed it.

## 2026-09-29 - MainInitTime was never loaded

* **Symptom:** found while porting `main_init`/`main_ready` for the screens: the wrapper had no
  property for FIZ `Cntrl.` `MainInitTime`.
* **What proved it:** `MainsInitTime` is set only by `LoadFIZ_Cntrl` (Mover.cpp:10910); nothing
  in the wrapper wrote it, so it kept the struct default 0 - a vehicle whose main circuit needs
  time after power returns (MainsCheck(), Mover.cpp:1591-1603; MainSwitch() waits for it,
  Mover.cpp:3360) closed its line breaker at once.
* **Fix:** `RailVehicleEngine.main_init_time`, read from `MainInitTime` and applied in
  `MoverDriveUnit::apply_configuration()`; `FIZ_PARSER_FORMAT_VERSION` 28.
* **Rule:** as the other Cntrl. entries - grep every `extract_value(..., "Key")` of LoadFIZ_Cntrl
  against the parser before calling a section ported.

## 2026-09-29 - EP07 rolled out of Markowo without power: a moment without voltage tripped it for good

* **Symptom:** krzyzowa2, the dispatcher gave the player's EX6435 no entry into Markowo Górne; the
  goods train waited at Krzyżowa as it should. The entry is given when EP07-329 (PE3435), leaving
  Markowo at 10:32, passes the third block towards Drawowo - and the EP07 stood on the line.
* **What proved it:** following EP07-329 after its departure: 28 km/h, then its line breaker open
  (`main=false`) with 3500 V back on the pantograph a moment later, the controller at 1, 0 A, and
  the driver still taking the engine for ready (`engine_missing` 0) - it rolled to a stand in six
  minutes. The breaker tripped at `markowo_grn_zwr11`, where the rear pantograph's arm lost the wire
  for one step ("Lost contact").
* **Causes:** two parts of the original not ported. A loss of the wire's voltage no longer than
  0.2 s keeps the last voltage (`NoVoltTime`, DynObj.cpp:3132-3140) - the port fed the Mover every
  step's voltage straight, so one step without contact tripped the breaker. And every update the
  driver takes the engine's readiness away when a line breaker of the consist is open or a
  converter relay tripped (`iEngineActive &= ...`, determine_consist_state(), Driver.cpp:6100-6104)
  - so `handle_engine()` gets it ready again, closing the breaker; the port kept the readiness.
* **Fix:** RailVehicleServer's step holds the vehicle's voltage through a loss of up to 0.2 s and
  feeds it apart from the pantographs' own (`set_collector_voltage()`, one writer of
  PantographVoltage); only a vehicle on a track is under a wire. The trainset reads the line
  breakers and converter relays under control, and the driver drops its readiness on them. The
  same run: no trip at zwr11, a longer loss before Drawowo closed again at once, Markowo's
  dispatcher at 2 (the player's entry) at 10:35.
* **Rule:** the vehicle's supply voltage is held through a short loss (0.2 s) as the original does,
  and a driver's readiness is tested against the consist on every update, not only while preparing.

## 2026-09-29 - the driving aid flickered: lerpf() misses its end by a rounding

* **Symptom:** approaching Markowo Górne the player's driving aid switched on nearly every update
  between "0 km/h in 0.4 km" and no next limit; the AI read the same table, so it knew about the
  red entry signal on one update and not on the next.
* **What proved it:** the table was steady (the entry signal B12 at stop, and 88 m after it
  `markowo_grn_tor2_wjazd_speedinfo`, SetVelocity 120). Printing the selection: B12's wanted
  acceleration came out `0.85000000000000009` against the preferred `0.84999999999999998`, so
  `wanted <= best` refused the stop and the 120 behind it was taken. The value is the easing into
  braking, `lerp(wanted, AccPreferred, share)` at `share == 1`. The original's `std::lerp`
  (Driver.cpp:919) returns its end exactly (C++20); Godot's `lerpf()` computes `a + (b - a) * t`
  and misses it by one ulp for some inputs.
* **Fix:** at the end of the easing the preferred acceleration is taken as it is. The aid shows the
  stop steadily down to the signal. A route test sweeps the approach speed and fails 32 times in
  3500 without it; the switch test that expected "no next speed" standing on a line of one speed
  had relied on the same rounding - the original gives VelNext as the line's speed there.
* **Rule:** a port of `std::lerp` whose result is compared exactly is taken at its end, not
  computed - `lerpf()` does not return its end exactly.

## 2026-09-29 - a goods train left past its exit signal: a track's second event2 was dropped

* **Symptom:** krzyzowa2, the goods train 3E/1-42 (TME9637) left Krzyżowa ~40 s into the scenario,
  ahead of the player's EX6435; the dispatcher expected it after the player (its exit P2 is given
  once the EP09 has passed Markowo), so the player caught it up on the line and ran into it.
* **What proved it:** the driver's table at the start held the W4 of Krzyżowa and no signal; the
  event server showed `krzyzowa_p2_sem_info` defined and passive, yet `krzyzowa_tor8end` carried
  only `Krzyżowa#p_stopinfo` on its event2 slot. The `.scn` names both on two `event2` lines; the
  track importer kept its parameters in one Dictionary and the second line overwrote the first.
  The original gathers them (`m_events2.emplace_back`, Track.cpp:818-822).
* **Fix:** a track's event names are gathered per slot (`MaszynaTrackData.events`), and every one
  is attached; the scenery cache format bumped to 28. The 3E waits at P2.
* **Rule:** a `.scn` key may repeat - check the original's loader for a list before keeping a key
  in a Dictionary.

## 2026-09-29 - a signal closing behind the train braked it hard

* **Symptom:** the 3E ran past Drawowo's entry signal B12 at stop by 184 m: at full service its
  brakes gave 0.07-0.10 m/s2 where the same train stopped from 80 km/h at 0.15-0.25 on its own.
* **What proved it:** logging the driver's braking on the way: twice before B12 the wanted
  acceleration jumped to -0.85 with the proximity negative (-245 m, -256 m) - a stop behind the
  train - then the brakes were released. Each block signal passed at proceed stayed in the table;
  when it closed behind the train, "an event behind holds only a stop" (Driver.cpp:957-963) braked
  hard. Each apply and release ran the G wagons' air down, and at B12 they had little to give.
  The original sends a signal passed at proceed as SetVelocity and lets go of it
  (`Point.Clear()`, TableUpdateEvent(), Driver.cpp:1662-1679).
* **Fix:** a signal (and, as a train, a W5) passed at proceed gives its speed and leaves the table.
  The same run stops 15.8 m short of Markowo's entry signal, no jolt on the way.
* **Rule:** an event the original clears once passed is cleared in the port - one kept holds on
  what it shows later.

## 2026-09-29 - a model rebuilt dropped the vehicle's voltage

* **Symptom:** with the cab controls fixed, entering the cab of the running 3E/1-42 still cut its
  voltage for ~3.5 s and opened its line breaker; a control run over the same spot without
  entering kept 3490 V throughout.
* **What proved it:** "Lost contact: ... pantograph 3 is not reaching the wire" on entering only.
  Entering rebuilt the vehicle's model, and `RailVehicle3D::_cache_animation_bindings()` built the
  pantograph geometry again from the model's rest pose - the arm "lowered", reached the wire again
  3.5 s later. The raise was simulation state kept in the drawing node (RC-022); every detail
  switch of any vehicle did the same.
* **Fix:** `RailVehicleServer` keeps the pantographs - geometry, raise, span - and steps them; the
  node hands over the model's arms (`vehicle_set_pantograph_geometry()`, which keeps the raise of
  one already there) and draws `vehicle_get_pantograph_raise()`. The collector position has one
  home, the server; the electric engine's copy is gone.
* **Rule:** a model rebuilt is a view change - state the simulation reads never lives in the node
  that draws it, and a rebuild of the drawing resets nothing the simulation holds.

## 2026-09-29 - a cab built on a running train lowered its pantograph

* **Symptom:** krzyzowa2, the player took the AI's 3E/1-42 at speed and handed it back: the
  pantographs came down, the line breaker opened, and the AI could not raise them again; the brake
  pipe stayed at 5 bar and the train rolled on without power. The AI's EN96 and ET22 filled the
  debugger with "Unknown command: sand".
* **What proved it:** a headless probe entering the running 3E (`PlayerServer.player_enter_vehicle`)
  and a listener on `RailVehicleServer.vehicle_command_received` printing `get_stack()`: building
  the cab sent `pantograph_valve_operate 1 0`, `sand`, the lights and `converter` -
  `MmdCabinInstancer.build_into()` -> `set_vehicle_rid()` -> `CabinButton._update_state()` set
  `pushed` from the vehicle's state, and the setter's `pushed_changed` handler acted
  (`CabinSystem.act()`). The debug HUD's `DebugSwitch` did the same through `button_pressed`, whose
  `toggled` signal sent the command back (`converter true`, then `false`). The EN96 has no
  `RailVehicleSwitches`, so no `sand` command; the driver sent it on every update.
* **Fix:** only the hand acts - `CabinButton.press()`/`release()`; showing the state sets `pushed`
  and nothing else. `DebugSwitch` shows the state with `set_pressed_no_signal()`. The driver sends
  every command through `MaszynaLegacyDriverHints.send()`, which skips one the vehicle does not
  have (`RailVehicleServer.vehicle_has_command()`), as the original's Mover call does nothing
  without the device (`Sandbox()`, Mover.cpp:3070).
* **Rule:** a control showing the vehicle's state never acts on it - one road to an effect, the
  hand's; and the driver never sends a command the vehicle does not have.

## 2026-09-29 - a train held by a Tm at stop it had passed

* **Symptom:** on krzyzowa2 the goods train 3E/1-42 (TME9637) left Krzyżowa as a train at 90 km/h
  and, about 30 s later, braked to a stand in open country for good - `stop_reason` 4, `velocity_next`
  0 at `proximity_distance` -314 m. The player's driving aid, which reads the same table
  (`DriverSystem.driver_get_state()`), was reported showing "stop" often.
* **What proved it:** a headless probe of the real scenery, the driver's table read each second: at
  the stop the only entry behind the front with a speed of 0 was `krzyzowa_tm6_sem_info`
  (`ShuntVelocity 0 0`), passed 314 m earlier; nothing ahead limited the train.
* **Cause:** the original ignores a Tm at stop for a train whether it is ahead or passed
  (`Velocity = -1`, and a passed one is let go of - `Point.Clear()`, TableUpdateEvent(),
  Driver.cpp:1618-1626). The port did it only for one ahead (`distance > 0`); a passed one fell
  through to "an event behind holds only a stop" (Driver.cpp:957-963), asked for -2 m/s2 and held the
  train there.
* **Fix:** a passed Tm at stop is skipped by a train and taken out of the table. The first attempt
  moved the "ignore" before the branch that sends `ShuntVelocity`, which in the original is its
  `else`: a Tm at stop ahead then turned the train into a shunting movement (`ShuntVelocity -1 -1`
  at order 128), and it stood before Tm6 for good - found by logging the driver's commands
  (`GameLog.log_updated`) in the probe.
* **Rule:** a signal a train ignores is ignored behind it as much as ahead; and an `if ... else if`
  of the original stays exclusive in the port - hoisting its first branch changes what the second
  sees.

## 2026-09-29 - td.scn's second track dead: a chain that touches no powered span is fed across the overlap

* **Symptom:** on td.scn, past switch `zwr01`, the diverging track has catenary but a raised
  pantograph reads 0 V.
* **Measured first:** the wire over that track is 19 spans (td.scn lines 1793-1958), supply
  `pwr01`, which is declared only as a `section`. Joined at 0.025 m per axis they form a chain of
  their own: none of its ends meets an end of the 105-span main line, where the only feed (`pwr1`,
  an autogenerated substation) is. The chain starts in the middle of a main-line span.
* **Cause:** `traction_table::InitTraction()` ends with a pass over section ends
  (Traction.cpp:858-895): every open end takes the power of the nearest powered span of the same
  section (`basic_cell::find()`, scene.cpp:579) and runs `ResistanceCalc()` along its own chain.
  `TractionServer::network_build()` did not have it, so the chain had no `power_near` and
  `wire_get_voltage()` returned 0.
* **Fix:** `TractionServer::_connect_section_ends()`, the pass ported, run last in
  `network_build()`.
* **Rule:** a wire chain that shares no end with a powered one is not dead in the original - it is
  fed across the overlap. Before debugging contact, check which chain the span is in and whether
  anything feeds it.

## 2026-09-28 - street lamps shadowed their own pool: a server light starts with no shadow blur

* **Symptom:** in stary_jawor_noc every street lamp (`linia053/lamp-5`) threw curved dark spokes
  and bands across its own pool, still after the lamp's geometry was moved to
  `SCENERY_LIGHT_OWNER_LAYER` alone (2026-09-27).
* **Proof:** a scratch scene rendered off screen (`xvfb-run`) showed that Godot honours
  `shadow_caster_mask` for a RenderingServer light and a node light alike - neither cast from the
  excluded layer - but the RenderingServer spot darkened the ground straight under itself.
  A one-lamp test scenery (`td_latarnia.scn`) through the real `SceneryInstancer` path gave the
  same dark pool in economy mode and diagonal acne across the whole pool in high quality. Godot:
  `_light_initialize()` sets `LIGHT_PARAM_SHADOW_BLUR = 0` (`light_storage.cpp:161`),
  `Light3D`'s constructor sets 1.0 (`light_3d.cpp:505`), and a spot's depth bias is multiplied by
  it (`light_storage.cpp:1232`). Every scenery spot had a depth bias of 0 - the tuned 0.06 never
  applied - and its `size` never softened anything (PCSS is off at blur 0).
* **Fix:** `_light_build()` sets `LIGHT_PARAM_SHADOW_BLUR` to 1.0 and gives omni lights
  `LIGHT_OMNI_SHADOW_CUBE`, the rest of what the two constructors set. Both modes render a clean
  pool.
* **Rule:** porting a light from a node to a RID, diff the whole constructor
  (`Light3D::Light3D(type)` and the subclass) against `_light_initialize()`, not only the
  parameters that look related to the symptom. A parameter that scales another one hides as
  "the bias is too small".

## 2026-09-28 - the player could not start the ST45 the AI drove: FuelStart was never read

* **Symptom:** in zwierzyniec_transport the player could not get ST45-03 (st45_v2, 301dd.fiz) to
  move; the AI driver started and drove it.
* **Proof:** a headless probe on the scenery took the vehicle from the AI and sent the cab's
  commands step by step. Held main switch alone: main_switch_enabled stayed false, the fuel pump
  never ran. The AI's own command log showed it sends `fuel_pump(true)` before the main switch.
* **Cause:** `fiz_train_engine_common.gd` read `OilStart` but neither `FuelStart` nor `WaterStart`,
  so every diesel's fuel pump stayed manual. 301dd.fiz says `FuelStart=Automatic` and its cab has
  no fuel pump switch - the player had no way to feed the engine.
* **Fix:** both keys read as the original does (`Mover.cpp:10948-10962`, manual when missing);
  `FIZ_PARSER_FORMAT_VERSION` bumped, or the stale cached import keeps the old mode.
* **Rule:** "the AI can, the player cannot" - log the AI's commands to the vehicle and replay them
  against the player's path; the command the AI sends and the cab cannot is the missing piece.

## 2026-09-27 - thin station objects lost their sun shadows: a normal bias of whole metres

* **Symptom:** on Stary Jawor, from the cab, the shadows of semaphores, switch indicators and a
  figure by the track were cut short near the cab and missing further out; rails cast almost none.
* **Cause:** the lookup moves along the normal by `shadow_normal_bias` texels of the cascade it
  lands in. The cab view spent two of four cascades on 0-3 m (splits 0.01/0.02/0.2 of 150 m), so
  everything outside the window fell into 3-30 m (2.8 cm texel) and 30-150 m (13.7 cm texel); at a
  normal bias of 5 that is 14 cm and 68 cm - thicker than a mast or a person. The exterior used a
  normal bias of 10 (0.3-2.6 m). Rails: the rail profile is an open strip, and with the shadow
  pass culling front faces (`reverse_cull_face`) a ray from above enters through a culled face
  and leaves through the open bottom.
* **Proof:** texel = bounding sphere of the cascade slice / 2048 px (4096 atlas, four splits),
  computed per cascade for the 45 degree cab camera - no in-game guessing.
* **What the original does:** one layout for the cab and the exterior - cascades ending at
  range/32, range/8 and range, range 250 m (`opengl33renderer.cpp:1106`, `Globals.h:153`), no
  normal bias, the cab only in the nearest cascade (`:1135`), and tracks drawn with culling
  disabled in the shadow pass (`:3609`).
* **Fix:** the world's sun takes the original's layout (1/32, 1/8, 1/2 of 250 m) with Godot's
  default normal bias 1.0; the cab gets a sun of its own lighting only its render layer
  (`maszyna/cabin/improve_shadows_quality`), with two splits over 8 m; track materials are a
  `cull_disabled` variant. Two shadowed directional lights halve Godot's directional atlas
  (`light_storage.cpp` `_get_directional_shadow_rect`), so the setting raises it to 8192.
* **Also found:** the E3D data is consistently CCW (17753 of 18135 scenery, 253235 of 253643
  vehicle submodels agree with their normals), and the importer flipped only indexed triangles
  and read only VNT2 - 21 VNT0 files (non-indexed, among them `czestochowa_dworzec_osobowy`) had
  no geometry at all. Both fixed.
* **`reverse_cull_face` was hiding acne, not fixing it.** With it on, a flat single-sided ground
  never entered a lamp's shadow map; with it off (now the default, as in Godot) the ground under
  a street lamp showed stable stripes. Measured from Godot's `light_storage.cpp:1162` and
  `scene_forward_lights_inc.glsl:813`: the merged lamp cone reaches ~60 degrees off the axis and
  gets a 128-256 px map, the normal bias vanishes under the lamp (x `1 - |N.L|`) and a node's
  spot bias 0.03 covers only the axis. Street lamps now use spot bias 0.06 and normal bias 3.0.
* **The lamp's own arms threw spokes across its pool** - the exclusion by
  `SCENERY_LIGHT_OWNER_LAYER` never worked: the layer was OR-ed onto layer 1, and Godot casts
  when `layer_mask & shadow_caster_mask` is non-zero (`renderer_scene_cull.cpp:2427`), so layer 1
  still matched `~OWNER`. A light-owning model now sits on the owner layer alone.
* **Thin casters looked flat even in clear weather:** the sun's `light_angular_distance` (Skydome
  drives it from the clouds, `Skydome.gd:775`) was 1.0 degree clear and 4.0 overcast in
  `project.godot`. The umbra is the caster's width minus the penumbra (distance to the ground x
  tan(angle)): a 0.2 m mast top 12 m from the ground along the ray had no umbra left at 1.0 degree.
  Both are now 0.53 degree, the real sun; the original has no penumbra at all (a fixed 1-texel PCF,
  `light_common.glsl:58`) and lets overcast only lighten the shadow (`opengl33renderer.cpp:2073`).
* **Rule:** excluding something from a mask means *every* bit it has must be outside the mask -
  adding an "excluded" layer beside the default one excludes nothing.
* **Rule:** a shadow bias is texel x normal_bias per cascade - compute it against the thinnest
  caster before tuning, and do not spend cascades on what a view does not look at. A culling mode
  that makes acne disappear also makes open geometry stop casting - fix the bias instead.

## 2026-09-27 - both pantographs of every vehicle sampled the wire at the vehicle's origin

* **Symptom:** none reported. Found while moving the pantograph's power path out of the drawing
  node, by asking what `_pantograph_wire_voltage()` is handed as the contact point.
* **Cause:** `RailVehicle3D` took the point from `pantograph_front_offset` /
  `pantograph_rear_offset`, two exported `Vector3` properties. The only assignments to them in the
  whole repository were inside their own setters: no scene declares a `RailVehicle3D`, the
  instancer never set them, nothing in GDScript did. So both were `(0, 0, 0)` for every vehicle in
  the game, and `contact_point = frame.transform.xform(offset)` put the front and the rear
  pantograph at the same place - the vehicle's origin.
* **What the original does:** it reads the pantograph's zero point off the submodel's own matrix -
  `vPos.z = m[3][0]` sideways, `vPos.y = m[3][1]` up, `vPos.x = m[3][2]` along the length
  (`TAnimPant::vPos`, `vehicle/DynObj.cpp:5508-5549`), with the comment "odczytane z modelu".
* **Fix:** the position is read from the arm submodel the node already resolves (the same nodes the
  arm lengths are measured from) and published to `RailVehicleElectricEngine` as
  `power_current_collector_first_position` / `..._second_position`, because it is the vehicle's
  geometry and the vehicle is what samples the wire. The two exported offsets are gone.
* **The fix's own ordering defect, which a test caught:** publishing needs two inputs that land
  independently - the model's arm nodes (when the arm paths change) and the electric engine (when
  the vehicle's parts are adopted). `_process_impl()` caches the animation bindings **before** it
  processes `dirty`, so on the first tick the geometry was known and the engine was not; the dirty
  flag was already consumed, so nothing published it later. `_publish_collector_positions()` is now
  called by both events, and whichever runs second completes it.
* **Rule:** a geometric value that nobody publishes does not read as missing - it reads as zero,
  and zero silently makes two things identical. The same shape as the bogie pivot spacing of 0
  (2026-09-23): grep for *assignments* to an exported property before trusting that anything fills
  it, and treat "both halves report the same number" as the signature.
* **Rule:** when a value is composed from two inputs that land in either order, every event that
  changes an input publishes it. One event plus a consumed dirty flag loses the race, and it loses
  it silently - the value stays at its default.
* **Trap met on the way:** removing an exported property needs a grep for *reads*, not only for
  assignments. `demo/hud/track_traction_panel.gd` read both offsets off the node and started
  erroring per frame.
* **Latent, not fixed:** `RailVehicle3D` binds its tick as `ClassDB::bind_method(D_METHOD("_process",
  ...))`, i.e. it registers a method under a virtual's name. No GDScript subclass defines `_process`
  today, so the trap of 2026-09-23 (a script replacing a native virtual) is not active - but it is
  one subclass away. Gone 2026-09-30: `RailVehicle3D` has no tick and binds no `_process`.

## 2026-09-27 - the SM42 stood braked: "zero speed" took its controller into the braking positions

* **Symptom:** on Stary Jawor the goods train SM42-1273 (6Dg) got its order from the scenario
  (`roz_uruchom_tow`, SetVelocity 70) and never moved: 70 km/h wanted, `trainset.ready` false for
  good, the master controller at 0 and the locomotive's cylinder at 6.2 bar.
* **What proved it:** the brake inputs per vehicle (only the locomotive braked, pipe 5.0), then a
  trace of the controller and a listener on `RailVehicleServer.vehicle_command_received` printing
  `get_stack()`: in one driver update `_decrease_eim()` stepped the controller up to 3 and
  `_control_releaser()` -> `MaszynaLegacyDriverHints.set_zero_speed()` stepped it back to 0.
* **Cause:** the 6Dg's universal controller works its local brake (`UCList: IntegratedLocBrake=yes`,
  `EIMCtrlType=3`): the Mover takes the local brake from `eimic_real` (`Mover.cpp:4503-4506`), and
  positions 0-2 brake, 3 is the no-power position. The original's `ZeroSpeed()` steps the master
  controller down only by `MainCtrlPowerPos()` (`Driver.cpp:3712`) - to the no-power position;
  `set_zero_speed()` stepped it down to 0.
* **Fix:** `set_zero_speed()` steps down by `controller_main_position -
  controller_main_no_power_position`. The SM42 releases, powers and runs through Roztocze.
* **Rule:** "zero" of a controller is its no-power position, not position 0 - on a universal
  controller the positions below it brake.

## 2026-09-27 - every rebuilt state dump stayed in memory: godot-cpp's Dictionary move

* **Symptom:** on Stary Jawor at x10/x20 memory grew by about 55 MB a second, paused as much as
  running (2.35 MB a frame), up to 10 GB and the OOM killer. The object count stayed flat, and
  Godot reported no leak at exit.
* **What proved it:** a jemalloc heap profile (`LD_PRELOAD=libjemalloc.so.2`,
  `MALLOC_CONF=prof:true`, two dumps diffed with `jeprof --base`): 99.7 % of the memory held was
  allocated in `VehicleController::compose_state()` under `RailVehicleServer::vehicle_dump_state()`.
  Each case in its own process: commands alone 0 KB, cached reads alone 0 KB, a command then a read
  about 40 KB (one whole dump of 169 keys) every time; clearing each dump after reading it cut that
  by nine tenths - the old dumps themselves were kept.
* **Cause:** godot-cpp generated `Dictionary::operator=(Dictionary &&)` as a copy construction over
  `opaque` without releasing what it held (`needs_copy_instead_of_move()`), so
  `placement->state_dump = controller->compose_state()` dropped every previous dump's reference
  without freeing it. Upstream: godotengine/godot-cpp#2048, fixed by #2055 on 2026-09-08. The line
  leaked since the cache (3246c7348); 5c95bc989 routed every `controller.state` reader through it,
  the sound system's per-frame reads included, which made it grow by the frame.
* **Fix:** godot-cpp raised to `507ed9d`, with what it breaks: `extension_api-4-7.json`, the native
  `Mutex`/`MutexLock` in `templates/mutex.hpp` (the engine class is `CoreBind::Mutex`), `Math::PI`
  for `Math_PI`, `CharString::get_data()` for a `std::string`. Stary Jawor paused holds 694 MB.
* **Rule:** memory that grows while the object count stays flat and nothing is reported at exit is
  a container still referenced - profile the heap (jemalloc) and diff two dumps before reading code;
  suspect the binding's value types too, not only our code.

## 2026-09-27 - the AI stood at a clear signal: a stop behind it, and a takeover that remembered

* **Symptom:** on Stary Jawor the player drove the SU46 to the signal before the station, left the
  cab, and when the SM42 and the eszelon had come and the signal cleared, the SU46 did not move.
  Headless the same: the AI's SU46 turned on n176 and stood on n174 for good - shunting, 40
  allowed, the signal ahead at 40, the engine ready, but 0 km/h wanted.
* **What proved it:** the driver's route dumped where it stood: `velocity_limit = 0` from
  `signal_velocity_last = 0` - the speed of the last signal passed, a stop it had passed before
  turning, now behind it; the shunting signal ahead read 40.
* **Causes:** two parts of the original not ported. Turning, the original clears its speed table
  and lets go of a stop of the signal passed ("don't allow potential red light overrun keep us
  from reversing", TableCheck(), Driver.cpp:510-526). Taking the vehicle over from a player, it
  forgets the way it drove ("kierunek jazdy trzeba dopiero zgadnąć") and clears the table, "the
  player may have driven against the signals" (TakeControl(), Driver.cpp:5700-5712), then guesses
  the way from the active cab or the movement (PrepareDirection(), Driver.cpp:5088-5116).
  `leave_cabin()` only switched the AI back on.
* **Fix:** the route reads the tracks afresh when the driver's way changes - the events ahead not
  taken as passed, no stop point done, the stop of a signal passed let go - and `forget()` makes
  the next reading a fresh one. `DriverSystem.vehicle_set_control_active()` tells the delegate
  (`_control_taken()`), which guesses the way again and forgets the route.
* **Follow-up, same day:** the first takeover fix guessed the way but left the reverser where the
  player had put it - the player put it backwards and left, the AI read the clear signal ahead,
  gave power, and ran backwards into the passenger coaches behind it, unseen by a scan that looked
  ahead. `PrepareDirection()` does both halves: the way guessed, then the master controller at zero
  and the reverser put that way (Driver.cpp:5116-5121); and `TakeControl()` switches the cab on
  first (`CabActivisation(true)`, Driver.cpp:5705) - without it a cab left inactive multiplies the
  reverser by zero. Both ported (`_prepare_direction()`, the `CAB_ACTIVATION` hint).
* **Rule:** whatever the AI remembers of the tracks belongs to one way of driving; a turn or a
  takeover starts it afresh.
* **Follow-up, same day (x10 on Stary Jawor):** the stop came back after the turn - the route took
  as passed every event gone from its reading, and the reading shrinks from 1500 m to ~750 m as
  the train moves off: the H2 signal at stop, 880 m ahead, was "passed", its stop became the stop
  of the signal passed. Passed is now only what would still be read were it ahead (its last
  distance inside the reach). And the eszelon, told to uncouple, turned its reverser towards a
  `direction_order` that was never set (0), took neutral for its way, thought itself standing and
  coasted into the SU46 without braking: set to the way it faces when the driver is made, as
  `iDirectionOrder = CabActive` (Driver.cpp:1872). A `ShuntVelocity` from a shunting signal read
  in the middle of uncoupling reset the count of vehicles and cancelled it: the route gives the
  signals' speeds only to a driving order, as check_route_ahead() does (Driver.cpp:8321-8335).

## 2026-09-27 - the cab's relays ran on real time: the AI's line breaker never closed at x5

* **Symptom:** an AI EU07 on a synthetic track with a live catenary kept its line breaker open;
  `main_switch_closable` was true.
* **Proof:** `LegacyCabinMainSwitch` closes after `main_on_bt` is held for `InitialCtrlDelay`,
  counted in `CabinSystem._process(delta)` - real seconds. The driver holds the button from one
  update to the next, `PREPARE_TIME` of simulation time: at x5 that is 0.4 s of real time, short of
  the delay. The original counts it in `TTrain::Update(dt)` with the scaled time.
* **Fix:** `CabinSystem` runs the cabs' processes on `SimulationServer.simulation_advanced`.
* **Rule:** every timer of the simulated train - the cab's relays included - runs on the
  simulation clock, never on the frame.

## 2026-09-27 - the series motor's automatic start had no thresholds (Imin, Imax 0)

* **Symptom:** the AI EU07 ready, wanting to go, its controller at 0: `IncSpeed()` steps on only
  while `Im < Imin`, and `circuit_imin` read 0.
* **Proof:** the backend copied `IminLo`/`IminHi`/`ImaxLo`/`ImaxHi`, but never set `Imin` and
  `Imax` themselves, which `LoadFIZ_Circuit` starts at the low ones (`Mover.cpp:11424-11425`).
  `compute_movement_()` moves `Imax` only where `ImaxHi > ImaxLo` (`Mover.cpp:1554`), so a vehicle
  with one threshold kept 0 for good, and `Imin` stayed 0 on every vehicle - the automatic start
  relay and the overload relay of the player's vehicle as much as the AI's.
* **Fix:** `MoverElectricEngineBackend` sets `Imin = IminLo`, `Imax = ImaxLo` after them. The EU07
  then starts: position 28 and the shunt at 32 km/h, 40 km/h in half a minute with eight wagons.
* **Rule:** a loader's derived fields are part of the port - the ones it sets from the ones read.

## 2026-09-27 - SA134 without a gearbox: the plain diesel's FIZ never reached the Mover

* **Symptom:** found while porting the AI's diesel traction. A dump of the model built of
  `sa134_v1/214m.fiz` showed `mechanical_min_rpm`, the clutch, the torque converter and the
  retarder at their defaults, and an empty gearbox.
* **Proof:** `FizTrainEngineParser` built a plain `DieselEngine` in a stub branch that applied only
  the keys common to every engine; `Engine:`'s own keys (`nmin`, `nmax`, `AIM`, `EUS`/`EDS`,
  `IsTC`, `TC_*`, `IsRetarder`, `R_*`, `ShuntMode`, `MaxVelANS`...) were never read. The
  `MotorParamTable:` section - a diesel's gears - is registered to the diesel-electric parser,
  whose `_get_node()` cast the engine to `VehicleDieselElectricEngine`: null for a plain diesel,
  so every row went nowhere. Its header, where the original reads the clutch
  (`LoadFIZ_MotorParamTable`, `Mover.cpp:11394`), was thrown away. `nmax` stayed 0, and
  `EngineRPMRatio()` divides by it (`Mover.cpp:1159`). Nothing sent `MotorParam[].AutoSwitch` to
  the Mover for any engine. `VehicleUniversalController` defaulted `integrated_brake` and
  `integrated_brake_pn` to true where the Mover has false.
* **Fix:** `FizTrainDieselEngineParser.apply_engine_fields()` for the whole `DieselEngine` case of
  `LoadFIZ_Engine`, in the Mover's units and with its defaults; `apply_clutch()` from the
  `MotorParamTable:` header; the rows read by `parse_diesel_gear_row()` (`readMPTDieselEngine()`)
  for a plain diesel; `AutoSwitch` applied; the universal controller's defaults the Mover's.
  After it: 214m has `nmin` 16.7 1/s, the converter and the retarder, and three gears.
* **Rules:**
  * A section is parsed for every `EngineType` that has it, and a parser never reaches its node by
    a cast to one engine class.
  * A property's default is the Mover's.

## 2026-09-27 - the driver's update hung on a refused controller

* **Symptom:** on Stary Jawor at x20 the probe stopped printing at t=91 s; the process ran on
  until the timeout at 100 % CPU.
* **Proof:** `MaszynaLegacyDriverDieselElectricTraction.decrease()` took the second controller to zero and
  returned true whenever it stood above zero. `DecScndCtrl()` refuses a diesel-electric engine
  with its automatic relay on (`Mover.cpp:2815`), so the position stayed, and `zero()`'s
  `while decrease()` never ended.
* **Fix:** the controller setters return whether the controller moved, and every step of power
  or brake returns that.
* **Rule:** a loop that steps a control ends on "did not move", never on "is not there yet".

## 2026-09-27 - couplers stiffened by a long frame

* **Symptom:** once the physics ran at the simulation speed (the one clock, below), the eszelon
  at speed 5 headless would not pull away: full power (391 kN, master controller 15), brakes
  released, 0.18 m/s for minutes, the trainset's acceleration jumping +-18 m/s2. At speed 1 the
  same start pulled away cleanly. Earlier "dead stops" of the eszelon at speed 5 were the same.
* **Wrong turns:** the physics catch-up jump (no warning logged), another train in the way (none
  within 150 m), a 0.2 s cap on the frame (still locked up), a driver fault (it drove the same in
  both runs).
* **What proved it:** the eszelon loaded, settled for 60 frames, then the clock advanced by hand
  (`SimulationServer.simulation_advance()` at a crawling speed, so only the hand advances counted) with 0.03 s
  and with 0.17 s frames - same scenery, same driver, only the frame length different: 14 m/s
  after 80 s against 0.18 m/s. Then the locations and neighbour distances refreshed before every
  sub-step instead of once a frame: 14.06 against 14.09 m/s.
* **Cause:** `TMoverParameters::CouplerForce()` (Mover.cpp:4779-4784, the original's own code)
  measures a coupler as the distance set by the last refresh plus ten times the relative
  `dMoveLen` since. The original refreshes once a frame (DynObj.cpp:8691-8699), so a coupler's
  load depends on the frame length; at the original's usual 60 fps it does not show.
* **Fix:** `RailVehicleServer::stepping_advance()` refreshes `update_location()` and `_update_neighbours()`
  before every sub-step (a location only for a vehicle that moved); the position is still
  announced once a frame. The Mover is untouched.
* **Rule:** whatever the Mover measures from "since the last refresh" is refreshed every
  sub-step, not every frame - otherwise its behaviour depends on the frame rate.

## 2026-09-27 - three clocks: the physics ran at real time, events and drivers at the speed set

* **Symptom:** headless probes at simulation speed 5 behaved oddly - trains reached places long
  after the events and the drivers expected them, and the analysis of a stop leaned on times
  that did not match.
* **What proved it:** reading who advances time. `RailVehicleStepper` handed `step_frame()` the
  raw frame delta; `ScenarioEventServer` and `DriverSystem` each added `delta *
  simulation_speed`, capped at 1 s, on `process_frame`; the sky backends counted the time of day
  themselves and pushed it to `SimulationServer` once a second. At any speed but 1 the physics ran
  at a fifth (or a tenth) of the pace of the events, the drivers and the clock of the day.
* **Fix:** one clock in `SimulationServer` (Timer::UpdateTimers(), Timer.cpp:79-87): a
  `SimulationClock` node, processed first, advances it by the frame's delta times the speed, at
  most 1 s, adds that to the simulation time and the time of day and emits
  `simulation_advanced(seconds)`. The physics integrates exactly those seconds in steps of at most
  0.01 s (drivermode.cpp:193-206) - no debt, no catch-up jump; a machine that cannot keep up runs
  the simulation slower. The events and the drivers read `simulation_get_time()`; the sky reads
  the time of day, and the environment only sets it (a jump). Whoever needs time holds the clock
  (`clock_hold()`/`clock_release()`); paused, it stands.
* **Rule:** anything that measures simulated time reads `SimulationServer` - never its own
  `delta * simulation_speed`.
* **Follow-up:** one `simulation_advanced` per frame left the drivers reacting once a frame: at
  1 s frames (x20) the eszelon's driver stepped its controller every second instead of every
  0.5 s and pulled away at 1.21 m/s against 3.24 m/s at 0.03 s frames (t = 20 s). The clock now
  hands a frame out in equal slices of at most 0.1 s (`SimulationServer::MAX_SLICE_TIME`, the
  quickest driver reaction), physics, events and drivers each slice in turn: 2.61 / 9.03 / 14.31
  m/s at 20 / 40 / 80 s against 3.24 / 9.24 / 14.09 at short frames.
* **Second follow-up:** with the couplers per step and the slices, a long frame simulates the same,
  so the original's cap of 1 s of simulation a frame (Timer.cpp:84, there because its Mover broke
  on long frames) only kept the speed from being honoured - x100 ran at about x60 at 60 fps. The
  cap is now on the real time a frame counts (`MAX_FRAME_DELTA`, 0.25 s), times the speed: x100
  is x100 down to 4 fps, and a hitch is not multiplied into a spiral. The speed goes up to 100.

## 2026-09-26 - FV4a handle left at lap: the train brake never released

* **Symptom:** Stary Jawor's eszelon (ST44, 20 wagons) would not start, or crawled as if braked
  and stalled: with the FV4a handle at running the brake pipe stayed at 4.2-4.9 bar or slowly
  fell, and the wagons' brakes held. SU46 and SM42-099 (FV4a) the same; SM42-1273 (MHZ_K8P) held
  5.0 bar.
* **Ruled out, by measurement:** the feed pipe (7 bar), `LockPipe`, the compressor, `BCPN`.
* **Proof:** the driver's valve flow (`dpMainValve`) of every FV4a was exactly 0. A temporary
  state key showed `fBrakeCtrlPos = 0`, `BrakeCtrlPos = 0` but `BrakeCtrlPosR = -2` (lap) already
  after loading, before any driver acted. At lap FV4aM's flow is `PF(..., S = 0)` = 0.
* **Cause:** the wrapper sets a vehicle up more than once (`apply_configuration()` itself runs
  `CheckLocomotiveParameters()` and `initialize_mover_state()`). `CheckLocomotiveParameters()`
  moves `BrakeCtrlPos` and `BrakeCtrlPosR` but not `fBrakeCtrlPos`; `initialize_mover_state()`
  then asks `BrakeLevelSet()` for the position `fBrakeCtrlPos` already holds, which returns at
  once - `BrakeCtrlPosR` stays where `CheckLocomotiveParameters()` put it. On FV4a lap is -2 while
  running is 0; on MHZ_K8P both are 0, which is why it worked.
* **Fix:** `initialize_mover_state()` aligns `fBrakeCtrlPos` with `BrakeCtrlPosR` before
  `BrakeLevelSet()`. Found on the way, and also against the original: `BrakeOpModes` defaulted to
  `PNEPMED` where the original has 0 (`Mover.cpp:10746`) - with `bom_PS` the Mover works the
  handle only from an occupied cab.
* **Rule:** the Mover's handle has three positions (`fBrakeCtrlPos`, `BrakeCtrlPos`,
  `BrakeCtrlPosR`) and only `BrakeLevelSet()` moves them together, comparing with the first one;
  after anything that sets them apart, re-align before setting. Read `dpMainValve` before theorising
  about the pipe.

## 2026-09-26 - headless test crashes at teardown: the dummy renderer is not thread safe

* **Symptom:** `test_zzz_ep07_cabin_main_switch` crashed in about half of the runs, in
  `MaszynaInclude._free_owned_rids()` -> `E3DRenderingServer::instance_free()` ->
  `E3DOptimizedBackend::clear()` -> `RenderingServer::free_rid()`, SIGSEGV. Other runs logged
  `mesh_add_surface: Parameter "m" is null` from `E3DModelManager.load_model()` while the cab
  loaded, "unimplemented base type encountered in renderer scene cull", or aborted with glibc
  `double free or corruption (!prev)` right after those errors.
* **Proof:** gdb on the crash: a built optimized instance, its model still referenced, its three
  render instances freed one by one. The crash is at the commit before too (2 of 4). No crash in 6
  runs with the scenery freed before the player, nor in 6 with streaming stopped first, but one
  of those aborted with the heap corrupted during the cab model load - so the teardown is only
  where the damage shows. In the engine source (4.7.2) the dummy renderer's mesh storage is
  `RID_Owner<DummyMesh>`, not thread safe, while the real one is `RID_Owner<Mesh, true>`.
* **Cause:** the streaming worker preloads E3D models, which creates meshes, while the main thread
  loads the cab's model (`MmdCabinInstancer.build_into()` -> `E3DModelManager.load_model()`). The
  RenderingServer allows mesh creation from any thread; the headless dummy renderer does not
  honour it, and two threads allocating in its mesh owner corrupt the heap.
* **Fix:** none yet (TODO.md, "Tests").
* **Rule:** a headless crash or heap corruption in rendering code that meshes are created in from
  two threads is the dummy renderer before it is our code - check whether a worker was loading
  models at the same time.

## 2026-09-26 - semaphore lost its model

* **Symptom:** `SignalHeadNode.model` in `demo_3d.tscn` was empty in the editor, and after the
  operator moved the semaphore model and saved, the `model = NodePath(...)` line was gone.
* **Proof:** packing a scene with the property set from code wrote
  `node_paths=PackedStringArray("model")` into the node's header; the hand-written entry lacked it.
  With the header added, the scene loads the node into the property.
* **Cause:** a node-typed (`PROPERTY_HINT_NODE_TYPE`) property is stored as a `NodePath` and
  resolved on instancing only for the names the header lists in `node_paths`.
* **Fix:** the header lists `node_paths=PackedStringArray("model")`.
* **Rule:** when writing a node-typed property into a `.tscn` by hand, add it to `node_paths` - or
  set it once in the editor and let the editor write the scene.

## 2026-09-25 - catalogue swapped, UI unchanged

* **Symptom:** a UI translated through Godot's i18n kept its old texts after the game directory
  changed. The language stayed the same, so only the game's catalogue was replaced.
* **What proved it:** a headless probe with a `Node` counting `NOTIFICATION_TRANSLATION_CHANGED`
  (Godot 4.7). `set_locale()` to a new locale: one notification. `set_locale()` to the same
  locale, `add_translation()` and `remove_translation()`: none. A node entering the tree gets one
  of its own.
* **Fix:** `MaszynaTranslationServer::translation_load()` sends
  `MainLoop::NOTIFICATION_TRANSLATION_CHANGED` itself when the locale did not change, and calls
  `set_locale()` only when it did, so the tree is told once either way.
* **Rule:** whoever swaps a translation in an unchanged locale notifies the main loop; a composed
  text is built in `_notification(NOTIFICATION_TRANSLATION_CHANGED)`, which also arrives on
  entering the tree.

## 2026-09-25 - cab clicks cut each other off: the controls bypassed gnd-sfx

* **Symptom:** with several cab controls moved in quick succession, the sounds cut each other
  off and ignored the cab's bus.
* **What proved it:** `CabinButton`, `CabinSwitch`, `CabinKnob` and `CabinSpotLight3D` each created
  their own `AudioStreamPlayer3D` and swapped its `stream` on every click, so each control had a
  single voice on `Master`. None of their sounds showed up in a bank dump.
  The vehicle's `CabinSfxPlayer3D` holds only the internaldata sounds. Also, every widget sits at the
  origin of the generated cab, so all the clicks came from one point.
* **Fix:** `MmdCabinInstancer` builds one `SfxBank` per cab with one `SfxPlayer3D`
  (`CabinControlsSfxPlayer3D`, bus `Cabin`, 16 voices). Each control sound is a polyphonic event
  whose `spatial_config.position` is the control's submodel, as the original places it
  (`Gauge.cpp:75-95`). The widgets hold that player and their event names.
* **Rule:** a cab control gets its sound as an event in the cab's bank, placed at its submodel,
  never as an `AudioStream` on the widget.

## 2026-09-25 - pantographs raised only with the master valve forced

* **Symptom:** after the cab's pantograph switches were ported as in the original, `P` alone no
  longer raised the E186's pantograph. `MoverElectricEngineBackend::pantograph()` had opened the
  pantographs' master valve on every raise since `1c0c044`, and taking that out broke it.
* **What proved it:** `LoadFIZ_Cntrl` sets the master valve (`PantEPValveStart`) to automatic
  by default and each pantograph's own valve (`PantValveStart`) to manual
  (`Mover.cpp:10927-10946`). The struct default is manual (`MOVER.h:875`). The E186 FIZ declares
  none of these keys. `grep` missed that at first, because the file is cp1250. So in the original
  the master valve opens by itself when there is low voltage.
* **Cause:** the wrapper never ported the five valve keys, so the Mover kept the struct default,
  and the workaround covered the missing default.
* **Fix:** `VehicleElectricEngine` carries the five keys as our own `StartMode`/bools, the FIZ
  parser reads them, `MoverElectricEngineBackend` writes them into the Mover, and the workaround is
  gone. The cab's switches send our `ValveOperation`, which is mapped to `operation_t` only in
  the backend.
* **Also found:** the original reads a legacy sound's files with `,` as a delimiter
  (`audio/sound.cpp:105-111`), so `small-compressor: a.wav,b.wav,c.wav` is begin, main and end,
  not a data error.
* **Rule:** a workaround in a backend call is a sign that a FIZ key is not ported yet. Read the
  key's default in `LoadFIZ_*` before keeping the workaround.

## 2026-09-25 - the E186 cab half built: three data quirks and a missing gauge feature

* **Symptom:** after the E186 controls were added to the catalog, the cab still did not react:
  radiostop_sw, universal*, battery_sw, pantalloff_sw and more had no widget, the light selector
  had no presets, and none of the three reverser lamps ever lit.
* **What proved it:** a headless probe entering the cab on td_e186.scn and listing every widget
  by control id, then MmdCabinInstancer.parse() on p160dc.mmd listing every descriptor - from
  line 210 of base.mmd.inc on, each `label: { ... }` block came out as an instrument called
  `soundinc`.
* **Causes:**
  * `radiocall3_sw { radio_3 ... }` has lost its colon. The original reacts only to labels it
    knows and walks over every other token; the wrapper's parser takes any `x:` token for a label,
    so it took the block's `soundinc:` for one and read every following block from the wrong end.
  * `LightsList:` in p160dc.fiz has no `endL` and runs straight into `WiperList:`. The FIZ builder
    ended an open table only when the next header opened none, so the WiperList header replaced
    the light table without its end_table(), and all twelve presets were lost.
  * TGauge takes `<name>_on` as the lit state of a control, shown instead of it while a flag is set
    (Gauge.cpp:204-210, 386-392; the flags are bound in Train.cpp:11995-12040, the reverser
    buttons to the sign of DirActive). The wrapper had no such thing, so kierunek_*_on stayed
    hidden.
* **Fix:** a block without a label is skipped whole; every FIZ section header ends the table
  before it; the MMD factory builds a CabinIndicator3D on `<name>_on` for a catalog entry with
  `state_light`.
* **Rule:** a parser of the original's data mirrors the original's tolerance, not the format's
  grammar - the data is full of lines only the original's "skip what you do not know" accepts.
* **Rule:** a table section may end at the next header rather than at its end marker; every
  header closes the open table.

## 2026-09-25 - SU46 would not release its train: the converter never started

* **Symptom:** some trains, passenger and freight, could hardly be released. SU46-054 with four
  coaches on `zwierzyniec_osob.scn` stayed braked. The trainset refactor (#184) was the first
  suspect.
* **What proved it:** one `get SU46-054` dump. The main reservoir was at 3.40 bar, the brake pipe
  at 3.35, and `compressor_allowed` was false. An FV4a cannot charge the pipe above the main
  reservoir, and the coaches' distributors hold their cylinders until the pipe comes back to
  around 5 bar. The refactor was cleared by comparing every moved coupling and movement function
  with its pre-refactor body.
* **Cause:** SU46 declares `CompressorPower=Converter` and `Cntrl. ConverterStart=Automatic`. For
  that compressor the Mover takes `CompressorAllow = ConverterAllow` (Mover.cpp:3886), and
  `ConverterAllow = Mains` only when the converter start is automatic (Mover.cpp:1885). No parser
  read `ConverterStart`, and the property existed only on `VehicleElectricEngine`, which a
  diesel-electric does not have. So the Mover kept `start_t::manual`, and SU46's cab has no
  converter switch to make up for it. A vehicle starting at velocity 0 begins with its main
  reservoir at `0.55 * MinCP` (Mover.cpp:8932) and relies on the compressor from there.
* **Fix:** `ConverterStart` and `ConverterStartDelay` are `VehicleController` properties, parsed
  with `BatteryStart` and applied next to it, as `LoadFIZ_Cntrl` does (Mover.cpp:10909). After the
  fix the operator's train pulled away.
* **Found on the way:** `BrakeValveParams` is never set, so every ESt distributor is built as an
  ESt4 (TODO.md). The test fixture's `W_Lu_L` valve has no distributor in the Mover (the factory
  falls through to a plain `TBrake`), so the fixture shows pipe pressure but never a cylinder.
* **Rule:** a `Cntrl.` key belongs to the vehicle. A property placed on one engine class silently
  does not exist for the other engine types that read the same key.

## 2026-09-24 - Python cab screens: what the original's scripts actually need

Porting `pyscreen:` meant running the original's own Python 2 scripts. Four things were only
visible that way.

* **A missing key blanks the screen.** `state['hours']` on a missing key raises KeyError, so
  `render()` fails and nothing is drawn. A harness fed `{}` and added each key a script asked for:
  `timetable` needs 2, `etcs_180kmh` 8, and all 145 scripts together read ~250 of
  `GetTrainState()`'s keys. `PythonScreenState` therefore always hands over the original's whole
  key set, with its value types, and fills in only what the wrapper has.
* **The parser ate the labels after a screen.** `pyscreen:` went through the generic instrument
  branch, which reads five value tokens, but a screen has two (legacy form) or a `{...}` block. A
  test now puts an ordinary instrument right after each form of screen.
* **Scripts assume the game dir is the cwd** (`./fonts/`, `from scripts import`), and importing
  writes a `.pyc`. The interpreter `chdir`s to the game dir and runs with `dont_write_bytecode`.
* **A leaked screen with a GDScript lambda callback crashes the exit** ("corrupted size vs.
  prev_size", `~Callable` in `PythonScreenServer`'s destructor at module de-init). Freeing the
  screens and using a method callback exits cleanly, and `CabinPythonScreen` does both.
* **Test trap:** the headless dummy renderer keeps no texture data, so an `ImageTexture` replaced
  by `set_image()` reads back as the 1x1 placeholder. The test checks the size, and the fixture
  script echoes the state it received.
* **Rule:** when porting a data contract consumed by scripts nobody here maintains, run the real
  scripts against it before designing it.

## 2026-09-24 - the E186 line breaker dropped at 17 km/h: the wire was a hundred times too resistive

* **Symptom:** the E186 pulled away and the line breaker opened after ~120 m, at 17 km/h.
* **Proof:** a per-frame probe on `td_e186.scn`. At 250-270 A the pantograph voltage slid
  2267 -> 1889 V over 1 m of travel, and the breaker opened the frame it passed `MinV` (1900 V).
* **Cause:** a scenery gives resistivity in Ohm/km (`traction pwr01 3500 4500 0.01`), and the
  original converts it to Ohm/m (`fResistivity *= 0.001`, Traction.cpp:112, with 0.01 read as the
  default 0.075). `TractionServer` multiplied the raw value by metres. Only a vehicle drawing
  real current shows it.
* **Fix:** `wire_set_params()` takes Ohm/km and stores Ohm/m. The same run now reaches 44 km/h at
  3547 V.
* **Rule:** a value copied from a scenery token carries the unit the original's loader gives it
  right after parsing. Read the lines after `>>`, not just the `>>`.

## 2026-09-24 - every brake handle froze under 1 bar of difference, because of C's abs()

* **Symptom:** the E186 brake pipe stopped at 4.0 bar in running position. The pipe lock
  (`LPOn=3.0 LPOff=4.5`) never released, so the loco had no power.
* **Proof:** the handle fields were all correct and the control reservoir still froze. The formula
  is `CP += 9 * min(abs(LimCP - CP), 0.05) * PR(...) * dt`. A scratch program built with
  `hamulce.cpp`'s includes printed `abs(0.89) = 0.000000`: it had resolved to C's `int abs(int)`.
* **Cause:** the original's `stdafx.h` includes `<stdlib.h>`, which puts the `std::abs` overloads
  in the global namespace. The vendored files have no `stdafx.h`. This hits the control reservoir
  of `MHZ_EN57`, `MHZ_K5P`, `MHZ_6P`, `M394` and `St113`, and the EP step of `TEStEP1` (11 calls
  in `hamulce.cpp`). `FV4a` does not use it.
* **Fix:** CMake force-includes `stdlib.h` into `src/legacy/maszyna-mover/*.cpp` (non-MSVC). The vendored files
  stay untouched.
* **Rule:** the vendored engine was written against its own precompiled header. A name that
  resolves differently without it compiles silently (vendored files build with warnings off), so
  check overload-sensitive calls (`abs`, `min`, `max`) when something numeric just stops.

## 2026-09-24 - an induction motor never pulled: nobody turned the controller into power

* **Symptom:** E186 with brakes released, direction set and the controller on T+: `Ft = 0`.
* **Cause:** the integrated controller's setpoint (`CheckEIMIC`, `CheckSpeedCtrl`, `eimic_real`)
  is computed by `TDynamicObject::Update` (DynObj.cpp:3246-3283), which is not vendored.
  `TractionForce` read `eimic_real = 0`.
* **Fix:** `VehicleEngine::_do_process_component` does it for a vehicle with a driver. The ED/PN
  brake split that follows in DynObj.cpp is not ported (TODO.md).
* **Rule:** a Mover method that nothing in `Mover.cpp` calls is called from DynObj.cpp or
  Train.cpp. Grep the original for its callers before assuming the backend drives itself.

## 2026-09-24 - every force of the E186 turned NaN once a direction was set

* **Symptom:** `velocity`, `Ft`, `brake_unit_force` and the wheel angles went `nan` once the
  reverser left neutral with the breaker closed.
* **Cause:** the EIM step divides by `InvertersNo` (`InvertersRatio`, Mover.cpp:5627).
  `LoadFIZ_Engine` gives a powered EIM without `InvNo` one inverter and sizes `Inverters`
  (Mover.cpp:11302). The wrapper did neither, so the step computed 0/0. The same block defaults
  `fcfuH` to `fcfu` (Mover.cpp:11290) and reads `Volt`, `abed`, `edep`, `eimclf`,
  `InvCtrCplFlag` and `Flat`. All of it is ported now.
* **Rule:** porting a `LoadFIZ_*` block means porting what it does after the `extract_value`
  lines too: derived counts, container sizes, fallbacks to another key.

## 2026-09-24 - the E186 line breaker opened the moment it closed

* **Symptom:** with the pantographs up, the main switch would not stay on. `main_switch` returned
  true and was false again three frames later.
* **Cause:** an induction motor opens the breaker above `CollectorParameters.MaxV + 200`
  (Mover.cpp:5706). The original reads FIZ `MaxVoltage` into both
  `EnginePowerSource.MaxVoltage` and `CollectorParameters.MaxV` (Mover.cpp:11622). The wrapper
  wrote only the first, so `MaxV = 0`. Series-motor vehicles use `MaxV` only with `OverVoltProt`,
  which is why they never showed it.
* **Test trap:** the knock-out lives in `TractionForce()`, which runs only with `Power > 0` and
  only for a driven vehicle. Two versions of the test passed with and without the fix. The kept
  test was checked against a build with the fix commented out.
* **Rule:** one FIZ key can feed several Mover fields. Grep every `extract_value(..., "Key", ...)`,
  not the first one found.

## 2026-09-24 - every spring brake started shut off, because the struct defaults were kept

* **Symptom:** on the E186 the spring brake seemed to do nothing, and "Enable" shut it off.
* **Proof:** a probe with e186's `SpringBrake:` values showed `shut_off=true` and `is_ready=false`
  from the first frame. The cylinder filled only through the bypass, because `UpdateSpringBrake`
  takes `MSP = ShuttOff ? 0 : MaxSetPressure` (Mover.cpp:4836).
* **Cause:** `LoadFIZ_SpringBrake` ends with `ShuttOff = false; Activate = false; IsReady = true;`
  (Mover.cpp:11028). The struct defaults (`ShuttOff{true}`, `IsReady{false}`) describe a vehicle
  *without* a spring brake, and the port dropped those lines. Setting them in `if (!Cylinder)`
  cannot work: `CheckLocomotiveParameters()` creates a fallback cylinder first (Mover.cpp:8881).
* **Found on the way:** `set_spring_brake_enabled(true)` called `SpringBrakeShutOff(true)`, and a
  test locked that inversion in. The original swaps `ValveOnArea`/`ValveOffArea`
  (Mover.cpp:11025), which the port read straight. `MTC` defaulted to 0 instead of 127.
  `i-springbrakeactive` showed `Activate` where the original shows `IsActive` (Train.cpp:9195).
* **Rule:** port the whole loader function, including the state it sets at the end. A struct
  default is what a vehicle *without* that section gets.

## 2026-09-24 - the Linux release would not start on most machines, because of the build host's glibc

* **Proof:** `objdump -T <file> | grep -oE 'GLIBC_[0-9.]+' | sort -Vu | tail -1`.
  `libmaszyna.64.so` needed GLIBC_2.43 and the `reloaded` template needed 2.44, both built on
  Manjaro. Ubuntu 24.04 has 2.39, Debian 12 2.36, Ubuntu 22.04 2.35. libstdc++ is static and was
  not the problem.
* **Trap:** the CI image `jezsonic/build-tools:4.7.stable` is Ubuntu 26.04 with glibc 2.43.
  Rebuilding only the library is not enough, because the template needs the newer glibc.
* **Fix:** `make release-linux` builds both in `ci/docker/linux-sdk`, Godot's buildroot SDK
  (`godot-2026.05.x-1`, GCC 15.2, glibc 2.34). The template is built once from `4.7.2-stable`.
* **Rule:** glibc is only forward compatible. Check the highest `GLIBC_` of every shipped binary,
  and build on a sysroot as old as the oldest target distribution.

## 2026-09-24 - switch blades in a scenery never moved, because only a node listened

* **Symptom:** throwing a switch changes the route, but the blades stay put. `TrackSwitch3D` in
  `demo_3d` still animates.
* **Cause:** `TrackServer::_process_switches()` emits `switch_offset_changed`, and the only
  listener was `TrackSwitch3D`. Since `923b293` a scenery builds tracks through
  `TrackRenderingServer`'s RID API with no nodes, so the blades were posed once in
  `_stream_build()` and never again.
* **Fix:** `TrackRenderingServer` subscribes to `switch_offset_changed` itself. The node's copy is
  removed.
* **Rule:** as with the scenery lights (2026-09-21), a feature parked on a node vanishes when an
  instancer without nodes appears. The server that owns the visuals subscribes to the manager.

## 2026-09-24 - what a "load" turns out to be in this engine

Porting `loadcount`/`loadtype` from a `.scn` `dynamic` line.

* **A load is not always cargo.** `AssignLoad(name, amount)` branches on the name.
  `"pantstate"` (Mover.cpp:7649) reads the amount as a bitmask that raises pantographs and picks
  the direction. The load therefore reaches the backend as name + amount together, through that
  one call.
* **A count with no name is not a load.** The name follows only a non-zero count, and the
  original zeroes both when it is missing ("idiotoodporność",
  `simulationstateserializer.cpp:1031`). Reading the next token unconditionally eats `enddynamic`.
* **Passengers are cargo.** The MMD `loads:` block maps cargo to a model
  (`logs: loads/eaos_vrz-99_logs`), and `passengers` is one entry of it. 235 vehicles declare the
  block. The wrapper used to read only `passengers`.
* **A cargo without a model is normal.** The lookup is: the vehicle's own override, then
  `<vehicle type>_<cargo>`, then the cargo name. Finding none is accepted (DynObj.cpp:7195).
  `dynamic/zssk/lgs_v1` has no `loads:` block and uses the third rule.
* **The load's height depends on how full the vehicle is.** `LoadOffset` lerps from `offset_min`
  to 0 (DynObj.cpp:3079). Both inputs are vehicle configuration, so the height is set when the
  configuration lands, on the same event the bogie spacing waits for (2026-09-23).
* **Rule:** before porting a field that looks like data, read what the backend does with its
  *name*.

## 2026-09-24 - a trainset ringing like metal, and the original naming the bug in a comment

* **Symptom:** from outside, a moving trainset sounds metallic, like comb filtering ("podwójne
  dźwięki").
* **Ruled out:** a probe over the banks found no event registered or played twice (EP07 5/3/30,
  E186 7/4/9).
* **Cause:** every wagon plays the same running-noise loop, all started at the same moment. The
  original names the effect (`audiorenderer.cpp:99`) and starts it at
  `Random(0.0, 80.0) * 0.01` of the sample (DynObj.cpp:6511): a *fraction* of the sample, drawn
  *once per vehicle*.
* **Fix:** `TrainSoundSystem` draws `randf_range(0.0, 0.8)` per bank runtime and starts a looping
  running sound at that fraction. `SfxPlayer.play()` already takes an offset.
* **Not the clatter:** `RunningSoundModel._wheel_clatter()` already phases each axle by position
  (DynObj.cpp:3671-3730). A one-shot per joint does not comb; a shared loop does.
* **Rule:** when many copies of one sound play at once, the defect is phase, not level. Look for a
  start offset before touching a gain.
* **The fix did not work, twice.** The offset was first applied only to a clip picked without an
  automation (000fe9b moved it into gnd-sfx). The second time it was added in
  `_resolve_voice_start_position` and overwritten one line later: both voice-start paths then called
  `_resolve_phase_locked_automation_start_position` for every automation, and that returned a bare
  `clip.stream_offset` when `phase_locked` was false (the default). A probe that played an automation
  event with `start_fraction` 0.5 printed a start of 0.0 at the first start and after a chunk swap.
  With the phase-lock override limited to phase-locked automations, it prints 0.5 in both places
  (`demo/tests/test_sfx_start_fraction.gd`).
* **Rule:** prove a fix to a value by printing the value where it is used, not where it is set.
* **It did not work a third time (2026-10-01).** Rolling wagons still rang metallic from outside.
  `SfxPlaybackRuntime._resolve_voice_start_position()` applied the shift to an automation clip and
  to a `TRIGGER_SUSTAIN` clip only; a plain looping timeline clip started at its stream offset. A
  single-sample `outernoise: { soundmain: ... }` is exactly that (the EP07 bank dump: `outer_noise_0/1`,
  one clip, no automation), and the test covered only the automation path - a plain looping clip
  with `start_fraction` 0.5 started at 0.0. A looping timeline clip takes the shift now. The
  bogie and motor copies of one vehicle (`outer_noise_0/1`, `traction_motor_0/1`) also shared one
  drawn fraction and played one recording in step a few metres apart. `play()` was not restarting
  the loop: the instance of a looping clip outlives its length (`test_sfx_start_fraction.gd`).
* **And the "cause" above misread the original.** `Random(0.0, 80.0)` once per vehicle
  (DynObj.cpp:6518) is the `#else` branch; `DynObj.h:27` defines `EU07_SOUND_BOGIESOUNDS`, so the
  original plays an `outernoise` copy per bogie and starts an even one at 50-80 % of the sample,
  an odd one at 0-30 % (DynObj.cpp:6505-6514) - two neighbours never start close. Located traction
  motors start anywhere (`LocalRandom(0.0, 1.0)`, DynObj.cpp:6085), a lone one at 0; other loops
  have no offset. A first port drew 0-80 % per copy independently, and two bogies of one wagon
  could land close together - heard as a doubled sound. `TrainSoundSystem.register_bank()` draws
  exactly the original's ranges now.
* **Rule:** a fix to how a clip starts is proven on every path a clip can start by - automation,
  timeline, sustain - not on the one the first bug report went through. A ported constant is read
  with the preprocessor: check which branch of an `#ifdef` the original builds.

## 2026-09-24 - the pantograph lost the wire where the original keeps it, in four different ways

* **Symptom:** on `zwierzyniec_tlk` an EP08 loses line voltage a few times per run and trips the
  main switch. After two fixes it still died exactly at the exit of one switch, at any speed.
* **Measured first:** 1335 spans / 2670 ends. 2414 ends have one neighbour, 116 are line ends,
  126 have three (four spans over a switch), and only 2 have a gap over the tolerance. All 1387
  joined pairs differ in height by 0.000 m. The wiring has no holes; the problem is choosing a span.
* **Divergences from the original:**
  * No guide horn. The original accepts a wire up to `fWidthExtra` = 0.381 m beyond the slider and
    counts it as higher (`scene.cpp:105-112`, DynObj.cpp:93).
  * The slider width never came from data. `pantograph_collector_width` sat at 0.5 for every
    vehicle. EP08 has `CSW=1.4`, so the original searches 1.081 m per side.
  * No chain walk. The original steps along `hvNext` (DynObj.cpp:8742). The wrapper re-searched
    and covered the gap with a 0.25 m span-end tolerance.
  * `iLast`, the one that killed it at the switch. A span that ends a section, or whose neighbour
    does (`TTraction::WhereIs()`, Traction.cpp:392), does not follow the chain at all
    (DynObj.cpp:8747). The wrapper followed a first-come `next[]` onto the wrong span.
  * Join tolerance: 0.25 m euclidean against the original's 0.025 m per axis
    (`TTraction::TestPoint`, Traction.cpp:355). At 25 ends that produced three candidates instead
    of one.
  * `parallel` and `section` were parsed and dropped. `section` is not a substation: power reaches
    a span along the wires (`TTraction::PowerSet()`, Traction.cpp:460). `TTraction::VoltageGet` is
    now ported whole.
* **Data trap:** every supply is declared twice under one name, once as `section` and once as a
  substation. The original's name table keeps the last (`Names.h:38`), which is the substation.
* **Rules:**
  * Measure the data before reading the code. Three of four hypotheses died to one pass.
  * A tolerance that papers over data becomes load-bearing once something trusts the structure
    under it.
  * 0 V at a raised pantograph has three causes: no wire in reach, a dead wire, and no contact
    (`PantDiff >= 0.01`, DynObj.cpp:3866). Report them separately, with the track and the offset.

## 2026-09-24 - a parked vehicle jumping, because two writers disagreed about where it stands

* **Symptom:** a stopped vehicle jumps slightly, only on curves and most on one switch. The
  position readout is stable.
* **Cause:** `apply_track_placement()` wrote the body transform twice. The first write came from
  `vehicle_get_transform()` (the track under the centre). The second, on `moved ||
  force_detail_refresh`, came from the chord between the bogie pivots. The two agree on straight
  track only.
* **Fix:** `RailVehicleServer` composes the body from the two pivots, cached against the
  placement. `RailVehicle3D` takes that one answer and only places the bogie nodes (today
  `RailVehicleRenderingServer` does, on `vehicle_placement_changed`).
* **Rule:** one piece of state, one writer. Two writers that both look correct disagree only where
  the geometry shows it.
* **Trap:** a vehicle with no mass integrates to NaN, and NaN never equals itself, so "did it move"
  is true forever.

## 2026-09-24 - a .fiz in the project stopped importing, silently

* **Symptom:** `--import` prints `Error importing 'res://tests/fixtures/test_vehicle.fiz'`, and the
  `.import` gets `valid=false`.
* **Cause:** `.fiz` now produces a `VehicleModel` (a `Resource`), but
  `FIZImportPlugin._get_save_extension()` still said `"scn"`. `ResourceSaver.save()` of a plain
  Resource to `.scn` returns 15 (`ERR_FILE_UNRECOGNIZED`); to `.res` it returns 0.
* **Trap:** Godot reimports on md5, not mtime. Delete the artifact under `.godot/imported/` to
  retest.
* **Rule:** an `EditorImportPlugin`'s save extension changes together with
  `_get_resource_type()`.

## 2026-09-23 - the cab acted one keypress late, because the dump was cached per step

* **Symptom:** a key plays its sound at once, but the operation happens only on the next key.
* **Cause:** `RailVehicleServer::vehicle_dump_state()` cached one Dictionary per physics step on
  the premise that only a step changes state. `TrainSystem::send_command()` changes it
  synchronously, and `CabinSwitch._on_command_received()` -> `_update_state()` read the pre-command
  snapshot.
* **Fix:** the cache is keyed on the step and on a command serial.
  `VehicleController::command_executed()` bumps the serial, updates the state and announces it.
  It is not keyed on `update_state()` alone, because a parked vehicle does not run it.
* **Rules:**
  * A cache keyed on a tick is correct only while the tick is the only writer. Write that premise
    where the cache lives.
  * "One action late" is a read of a snapshot taken before the write. Look for the cache first.

## 2026-09-23 - the release re-parsed every scenery because its game dir was "."

* **Symptom:** in the shipped build every scenery is parsed on each launch. The editor caches fine.
* **Wrong first guess:** the build stamp clearing caches. In fact `clear_cache()` only emits
  `cache_clear_requested`, and the scenery cache does not listen to it.
* **Proof:** running `_is_cache_valid()`'s checks over `user://cache/scenery_compiled/*.res`. The
  editor entries carry absolute `src` and resolve. The release entries carry
  `src=scenery/baltyk/mod/drogi.scm`, with 102 of 102 dependencies missing.
* **Cause:** `get_maszyna_game_dir()` returned `"."` in an export (`UserSettings.cpp:119`, since
  `d3e7d52`), and a relative path given to `FileAccess` resolves against `res://`, which is the
  pack. The parser reaches files by another route, which hid it.
* **Fix:** in a release the game dir is `OS::get_executable_path().get_base_dir()`. The editor and
  the release now share cache keys.
* **Rules:**
  * A path handed to `FileAccess` is absolute, or it is silently a `res://` path.
  * When a cache "does not work", load an entry and run its own validity check before suspecting
    invalidation.

## 2026-09-24 - the shipped library had no symbols, and the crash was in the parser

* **Symptom:** closing the game during loading segfaults on a non-main thread (`#0 0x0`).
* **Cost:** the shipped `.so` was stripped, and two cores were misdiagnosed by stack shape. The
  `-s` comes from godot-cpp's `DEBUG_SYMBOLS` generator expression, propagated through
  `TARGET_LINK_LIBRARIES`, so editing our own `LINK_OPTIONS` changes nothing.
  `make release-linux-symbols` builds `RelWithDebInfo` with `template_release`.
* **Cause (with symbols):** `MaszynaParser::parse_chunk` (`maszyna_parser.cpp:270`) ->
  `Callable::call` -> freed code. The parse runs as a `SceneryLoadingTaskQueue` task, and nothing
  stopped the queue before the scripts went away. Its destructor runs during teardown, too late.
  `callback.is_valid()` does not help.
* **Fix:** `SceneryLoadingTaskQueue::drain()`. `SceneryInstancer` keeps `_active_queues`, and
  `cancel_loading()` drains them, called from `maszyna_include.gd::_exit_tree()`.
* **Regression 1:** draining waits for a multi-second parse, so quitting hung instead.
  `MaszynaParser` now has a static `cancelled` checked by the token loop and by
  `_count_includes`.
* **Regression 2:** `drain()` dropped queued tasks that another task `wait()`s on, which
  deadlocked `wait_to_finish()`. A core from `kill -ABRT` showed threads in
  `SceneryLoadingTaskQueue::wait` (`:90`) from `_run` (`:146`). `wait()`/`is_done()` now give up
  while draining.
* **Diagnosis tip:** `/proc/<pid>/task/*/wchan` needs no privileges, and `kill -ABRT` turns a hang
  into a symbolised core.
* **No test:** a mid-parse teardown test passed with and without the fix (headless parses finish
  too fast), so it was deleted.
* **Rules:**
  * A shipped build keeps its symbol table.
  * A `Callable` across a thread is only as valid as its script. The thread's owner stops it
    before the scripts go.
  * Every worker needs an owner that stops it at teardown. A destructor runs too late.

## 2026-09-24 - the simulation stepped after everything that reads it

* **Symptom:** vehicles judder, worst from the external view, even a single loco.
* **Wrong turns (reverted):** a fixed step, which was worse without interpolation, and the cabin
  shake. `4ec5490` had already decided both: a variable delta as in the original
  (`vehicle_table::update`, DynObj.cpp:8181) and `process_priority = -100`.
* **Cause:** the C++ port stepped from `SceneTree.process_frame`, which is emitted after all
  nodes are processed. It pushed placement onto `RailVehicle3D` afterwards, but other readers
  (`ExternalCamera._process()`) saw the previous frame.
* **Fix:** `RailVehicleStepper` (`process_priority = -100`) calls
  `RailVehicleServer::step_frame()` - since 2026-09-27 `SimulationClock`, the same priority,
  ticking `SimulationServer`'s clock.
* **Follow-up (superseded 2026-09-27, "three clocks"):** past 0.2 s, `sub_step = delta /
  MAX_PHYSICS_ITERATIONS` exceeded `PHYSICS_STEP`, so `step_frame()` owed the excess to the next
  frames and past `maszyna/physics/catch_up_limit` took it in one jump. Gone: the frame is capped
  at 1 s and integrated whole, as the original does.
* **Test trap:** `test_process_movement_with_invalid_controller_reference_is_noop` relied on the
  old order. `controller = null` does not detach, so the test now detaches the controller.
* **Rules:**
  * ~~`process_frame` is the end of a frame.~~ Wrong, see 2026-09-30: measured on Godot 4.7.2,
    `process_frame` is emitted before every node's `_process`.
  * When a fix is being reinvented, `git log -S` the moved code and read the original commit
    first.

## 2026-09-23 - a GDScript subclass silently replaced the native _ready()

* **Symptom:** after `Cabin3D` moved to C++, the camera stopped entering the cab, silently.
* **Cause:** C++ `_ready()` emitted `cabin_ready`, but `MaszynaDynamicTrainCabin` (GDScript) defines
  `_ready()`, which **replaces** a native virtual. `super._ready()` is refused for native
  virtuals.
* **Fix:** `_notification()` with `NOTIFICATION_READY` / `NOTIFICATION_PROCESS`, which reaches
  the native class and the script both.
* **Second half:** the script's override of `set_train_id()` (which rebuilt the interior) is
  bypassed by the typed call from `RailVehicle3D`. A script shadows a native method only for
  `call()` callers. `Cabin3D` now emits `train_id_changed` and the subclass reacts.
* **Rules:**
  * A C++ class under an existing GDScript subclass puts its lifecycle in `_notification()`, never
    in the `_ready()`/`_process()` virtuals.
  * A C++ base does not offer "override this method" unless it is a registered virtual. It
    announces with a signal instead.

## 2026-09-23 - the loco that would not move had nobody in the cab

* **Symptom:** `test_sm42_startup_sequence::test_successful_moving_on` - "Speed should be > 0" -
  was red for weeks since `87d5f8d`.
* **Cause:** `ComputeTotalForce()` (Mover.cpp:4485) keeps physics active only when
  `CabActive != 0 || Vel > 0.0001 || |AccS| > 0.0001 || LastSwitchingTime < 5 || EZT || DMU`. The
  test used `cabin_number = 0`, so `CabActivisation()` never ran (Driver.cpp:2126). After 5 s the
  Mover stopped integrating.
* **Fix:** the test occupies the cab (`cabin_number = 1`).
* **Rules:**
  * A vehicle that is not driven is not simulated. Check `CabActive`/`PhysicActivation` before
    treating "it does not move" as a bug.
  * A test that stays red across many unrelated commits stops being evidence of the commit that
    turned it red.

## 2026-09-23 - a parked vehicle never had its bogies placed, and four guesses before one print

* **Symptom:** both bogies on a curve had the same tangent. A 1 mm nudge fixed it.
* **Cost:** four hypotheses read off the code (two of them real bugs, fixed on the way) before one
  `print` of every guard in `apply_track_placement()` named the branch.
* **Cause:** components added after `VehiclePhysicsNode::_build()`'s `initialize()` configure
  nothing until the next tick dirties the Mover. `MoverVehicleWheels` reads `mover->BDist`, so the
  only placement saw spacing 0, and `moved` stays false while the vehicle stands still.
* **Fix:** `RailVehicle3D` reacts to `mover_config_changed`. Today the drawing is
  `RailVehicleRenderingServer`'s, and it reacts to `VehicleServer.vehicle_config_changed`.
* **Rules:**
  * A per-frame path gated on "moved" never picks up a late value. Recompute config-derived values
    on the config event, not with a retry flag.
  * After two hypotheses read off the code have failed, stop reading and print.

## 2026-09-23 - every vehicle ran with a bogie pivot spacing of zero

* **Cause:** `MoverVehicleWheels::_fill_config_dictionary()` (and
  `MoverVehicleUniversalController`) published keys named after methods, parentheses included:
  `p_config["get_bogie_pivot_spacing()"]`, 11 keys in 2 files. `RailVehicle3D.cpp:1106` asks for
  `bogie_pivot_spacing` and got the default 0.0 for every vehicle.
* **Fix:** the keys carry the value's name.
* **Rules:**
  * A state or config key is data, not a method name. Grep fills for `["get_`.
  * `Dictionary.get(key, default)` hides a typo forever. Test that the key is present.

## 2026-09-23 - the cab's instrument backlight blinking, once per frame, from the transform

* **Symptom:** in the EP07 cab the desk backlight and the ceiling lamp blink when switched on.
* **Proof:** the state dump was stable (`11 11 11`). With the cab entered, `podswietlenie_on` was
  visible one frame in fifteen and `_off` was its complement: one writer at 10 Hz, one every
  frame.
* **Cause:** `E3DRenderingServer::instance_set_transform()` ended in `_update_if_built()`, which
  in `E3DNodesBackend` re-applies `lights_state` to the `_on`/`_off` submodels. `9ca6b9f` made the
  transform notification unconditional (for smoke), so every node-instanced model re-applied its
  lights every frame.
* **Fix:** `E3DInstanceBackend::apply_transform()`. It does nothing for nodes and does
  `instance_set_transform` per RID for optimized instances. This also dropped a per-frame
  re-resolve of the optimized instances. The czuwak/SHP blinker was broken by the same thing.
* **Rule:** a setter applies what it is named after. Routing every `instance_set_*` through "apply
  everything" makes a move overwrite state owned by someone else.
* **Still open:** the light submodels have two managers (TODO.md).

## 2026-09-22 - a teardown abort that is RID allocator corruption, not a double free

* **Symptom:** `test_zzz_ep07_cabin_main_switch` aborts during scenery teardown in about half the
  runs.
* **Instruments:** a print per group in `_free_owned_rids()` found it dying in group 5
  (`E3DRenderingServer.instance_free`, 324 instances). `coredumpctl debug` showed
  `E3DOptimizedBackend::clear` -> RenderingServer -> **SIGABRT**. The errors were "Initializing
  already initialized RID", "Attempting to initialize the wrong RID" and "unimplemented base type
  encountered in renderer scene cull", which means allocator corruption, not a double free.
* **Mechanism:** `_stream_preload()` runs on `SceneryStreamingServer`'s worker and calls the
  GDScript `model_loader` (`e3d_model_manager.gd::load_model`), which runs `load()` and creates
  renderer resources while the main thread frees them. A cold `rail_vehicle`/`fiz` cache
  reproduces it.
* **Fixed on the way, not the cause:** a double free across an await in `_free_owned_rids()`, 14
  unguarded `get_instance()->` calls, and a HashMap iterator held across re-entry in
  `instance_free()`.
* **Half fixed 2026-09-24:** `SceneryStreamingServer::streaming_drain()` is called from
  `maszyna_include.gd::_exit_tree()` before `_free_owned_rids()`. The reload path has done this
  since `8d02b43`. The race while a stream is running remains open (TODO.md).
* **Not reproducible headless:** a `--script` run has no autoloads (so no `model_loader`), and the
  dummy renderer creates nothing.
* **Rules:**
  * An abort: read the engine's error lines first. "Already initialized RID" means concurrency;
    "invalid RID" means a double free.
  * Godot's crash dump is not the stack. Use `coredumpctl debug`.
  * Read a worker-thread `Callable` all the way down. `load_model` is `ResourceLoader.load()`.

## 2026-09-22 - "the C++ port made it 4x slower" was a GPU that never woke up

* **Symptom:** `td.scn` ran at ~30 fps instead of ~200 right after `TrackServer`/`SpatialIndex`
  moved to C++.
* **Cause:** the discrete GPU stayed in powersave, so the integrated RX 780M rendered. The GPU
  frame time was over 40 ms from the first measurement.
* **Rules:**
  * Split frame time into CPU and GPU before any hypothesis. If the GPU time moved, check the
    adapter (`--verbose`).
  * Confirm the environment (adapter, power profile, build flags) before blaming the code.

## 2026-09-22 - a config property and a state key of the same name are not the same value

* **Symptom:** `test_train_battery::test_successful_battery_voltage_drop_after_two_seconds` went
  red.
* **Cause:** the state was pointed at the `battery_voltage` config getter. The authored value is
  the nominal voltage (written to `BatteryVoltage` and `NominalBatteryVoltage`), and the backend
  changes `BatteryVoltage` at run time (Mover.cpp:946).
* **Fix:** `get_live_battery_voltage()` for the state. `RPowerCable.SteamPressure`, `PowerTrans`
  and `RAccumulator.RechargeSource` were checked, and the backend never writes them.
* **Rule:** grep the backend for an assignment before publishing a state value through a config
  getter. A value the simulation writes is state.

## 2026-09-22 - an uninitialised pointer that only a property read could reach

* **Symptom:** SIGSEGV in `PackedScene.pack()` (`fiz_train_controller_instancer.gd:218`). The
  bisect gave inconsistent answers.
* **Proof:** `addr2line` on the three `libmaszyna` frames:
  `VehicleDoors::get_locked()` -> `VehicleController::get_mover()` via `MethodBind::bind_call`,
  that is, a property read.
* **Cause:** `VehicleComponent::train_controller_node` had no initialiser. It is set in
  `ENTER_TREE`, and `pack()` read the new typed properties of a never-parented component.
* **Fix:** `= nullptr`.
* **Rules:**
  * Exposed properties make getters reachable before `_ready()`, before `ENTER_TREE`, during
    `pack()` and from the inspector. Everything they touch is valid from the constructor.
  * A raw pointer member gets `= nullptr` at its declaration, always.
  * Run `addr2line -f -C -e <.so> <offsets>` on the extension's hex frames before reading the tail
    of the dump.

## 2026-09-22 - a regex that deleted 588 lines, and the linker that caught it

* **Symptom:** the build succeeded, then Godot refused the extension with
  `undefined symbol: RailVehicleServer::vehicle_move(RID const&, double)`, and every script naming
  a wrapper class failed to parse, which looked like a broken class cache.
* **Cause:** a removal regex with an optional `(    /\*.*?\*/\n)?` prefix under `re.S` matched
  across hundreds of lines and deleted 588 of 856. A missing definition only fails at link time,
  and GDExtension links lazily.
* **Fix:** remove by walking lines from a signature to its matching `    }`. The result was checked
  by diffing `grep -oP "Class::\K\w+"` against `HEAD`.
* **Rules:**
  * No span deletion with a regex whose optional prefix can match across lines. Verify scripted
    source edits structurally (symbol list, line count).
  * `undefined symbol` from a GDExtension means a bound method has no definition. Look for a
    deleted definition first.

## 2026-09-22 - an unguarded singleton dereference only crashes at teardown

* **Symptom:** signal 11 in `_free_owned_rids` -> `TrackServer.track_free` in
  `test_zzz_ep07_cabin_main_switch`.
* **Cause:** `track_free()` emits `tracks_changed`, and `RailVehicle3D`'s handler called
  `TrackServer::get_instance()->...` with no null check after the singletons were unregistered.
  The untyped `->call()` it replaced had only warned.
* **Fix:** every `get_instance()` result in `RailVehicle3D` goes into a local and is checked.
* **Rules:**
  * The typed form removes the string, not the null. The guard becomes more necessary.
  * A signal emitted from a teardown path runs handlers against a half-dismantled world.

## 2026-09-22 - what porting an autoload to C++ actually costs, and the crash it hides

`TrackServer` (1023 lines) and `SpatialIndex` (55) became C++ singletons (#184 stage 2). What
GDExtension cannot carry over:

* Enums flatten: `TrackServer.TrackType.TRACK_NORMAL` -> `TrackServer.TRACK_NORMAL`, 370 call
  sites.
* No inner classes (`EndpointRef` -> `TrackEndpointRef`), and no constructor arguments, so
  `X.new(a, b)` becomes `X.new()` plus assignments.
* No float/RID constants, so they became read-only properties (`TrackServer.rail_height`), and
  `UNDEFINED_TRACK` -> `RID()`.
* `Array[Vector3]` -> `PackedVector3Array`, and GUT will not compare packed arrays with literals.
* **The crash:** `RailVehicle3D::_apply_start_track()` used
  `get_node_or_null("TrackServer")->call(...)`, which returned nullptr once the autoload was gone.
  This is why reaching a singleton by path and calling by name are prohibited.
* **Trap:** the first headless run after a rebuild re-imports and can take minutes. Run `--import`
  alone first.

## 2026-09-22 - reading the vehicle state was changing it, in four places

Found by reading every `_do_fetch_state_from_mover()` in #184 stage 1:

* `TrainController::_consume_coupler_sounds()` cleared `TCoupling::sounds` on the Mover, so the
  first reader ate the events.
* `TrainBrake` advanced a filter with `get_process_delta_time()`, so the fall/rise rates depended
  on how often the state was read.
* `TrainEngine` (`engine_start`/`engine_stop`) and `TrainSecuritySystem`
  (`blinking_changed`/`beeping_changed`) emitted from the fetch, comparing against values from the
  dictionary being filled.
* The `state_dirty` guard (one fetch per tick) hid all four.
* **Fix:** the work moved to `_do_process_mover()` / `_handle_mover_update()`, and the fetches
  only read.
* The twelve coupler counters were sound bookkeeping living in the vehicle. The vehicle now emits
  `coupler_attached`/`coupler_detached` (carrying a `CouplingElement`, since 2026-09-30 a
  `RailVehicleController.CouplingFlags` flag), and `TrainSoundSystem`
  counts per vehicle RID.
* `power_source` was written by `TrainElectricEngine` and `TrainLighting`, and the last to merge
  (FIZ section order) won. Lighting now publishes `light_power_source`.
* **Rules:** a getter never changes state; state lives in the only layer that needs it (see
  `CODE_STYLE.md`).

## 2026-09-22 - the sound system's per-frame cost was not where the loop was

* **Measured** (300 vehicles, 600 banks, 297 in range): 11.8 ms/frame. `controller.state` took
  5.0, `_update_triggers` 4.8, `_update_brake_sounds` 1.5 and `_ensure_brake_events` 0.23. The
  walk over all banks took 0.35 ms.
* **Cause:** untyped trigger Dictionaries re-read and converted per tick, and
  `MaszynaBrakeSfxEventFactory` tables plus `_primary_source()` walked per event per tick.
* **Fix:** `Trigger`/`BrakeEvent` records resolved once. The frame went 11.8 -> 2.35 ms, and the
  bulk left is `TrainController.state` (~17 us per controller).
* **Rule:** measure by phase before restructuring. The loop the eye finds was 3% of the cost.
* **Trap:** a `RailVehicle3D` without a track has a NaN transform, `NaN > culling_distance` is
  false, and so it is never culled.

## 2026-09-22 - the sfx playback tick moved off the main thread, and what the numbers showed

* **Measured** (200 players with 4 voices each, main thread): 24.7 ms before, 19.7 ms after
  (wait 12.2, flush+observe 2.2, tick on the worker 12.1).
* **Trap:** headless, the main thread has nothing to overlap with, so the total barely moves. The
  split is the real result.
* `modulate()` re-applied every voice synchronously, and the tick recomputes them anyway (7.6 of
  27.9 ms). It now only stores the values and raises `automation_refresh_pending`.
* **Rules:**
  * Work the tick redoes anyway does not belong in the synchronous API path as well.
  * The scene tree is not thread safe, while global-scope servers are. The worker writes
    `SfxVoiceSlot` values, and only the main thread touches `AudioStreamPlayer(3D)`. The tick is
    posted at `process_frame` and awaited at the start of the next frame, so no lock is needed.
* **Trap:** running against a `git worktree` of another commit leaves
  `global_script_class_cache.cfg` stale. Run `--import`.

## 2026-09-22 - a vehicle of the previous scenery left in the strip when the search found nothing

* **Symptom:** an empty search left the previous scenery's vehicles in the strip.
* **Proof:** the screen run as a scene logged `Invalid type in function 'set_tiles' ... does not
  have the same element type as the expected typed array argument`.
* **Cause:** `%TrainsetGrid.set_tiles([])`. A bare `[]` is refused by an `Array[TileGrid.Tile]`
  parameter declared in another script, and the call never runs. An isolated probe passed because
  the class lived in the calling script.
* **Fix:** a typed `var tiles: Array[TileGrid.Tile] = []` with one exit.
  `set_rows(PackedStringArray(), PackedStringArray())`.
* **Rules:**
  * Never pass a bare `[]`/`{}` to a typed collection parameter.
  * Probe a screen by running it as a scene, not with `--script`, which has no autoloads.

## 2026-09-21 - smoke emitters spawned at the origin of the world

* **Symptom:** sm42, st44 and su45 do not smoke, although `smoke_get_statistics()` reports the
  emitter as built.
* **Cause:** the scene cull overwrites `particles_set_emission_transform()` from the instance
  transform, which was left at identity.
* **Fix:** `instance_set_transform()`, with `particles_set_custom_aabb()` in local space.
* **Rule:** a RenderingServer particle system is placed through its instance. Configure a server
  effect the way the equivalent node does.

### The whole plume cut off in one frame on a notch change

* **Cause:** `particles_set_amount_ratio()` deactivates live particles with index >=
  `amount * ratio`. A ratio of 0 on a notch change killed the whole plume.
* **Fix:** emitting is off. `E3DRenderingServer::process_smoke()` (ticked by `SmokeSourceLibrary`)
  accumulates `spawn_rate * intensity * delta` and calls `particles_emit()`: the original's
  `m_spawncount` (`particles.cpp:157-212`).
* **Opacity, twice:** `dizel_fill` in `color` alpha, and then in `color_initial_ramp`, both reach
  live particles, so the plume stepped when the Mover floored it at 0.05 (Mover.cpp:5508). The
  initial opacity ramp is now written once at build time, and `dizel_fill` is folded into the
  spawn rate (`RailVehicle3D::_update_smoke()`). This is a deliberate divergence from
  `particles.cpp:330`.
* **Rule:** `color`, `color_initial_ramp` and `amount_ratio` all reach particles already in the
  air. Only the emission itself affects just the new ones.

### A state key published by one engine part only

* **Cause:** `diesel_max_rpm` was on `TrainDieselElectricEngine`, so a plain diesel got 0 and never
  smoked.
* **Fix:** it moved to `TrainDieselEngine`, via `TMoverParameters::EngineMaxRPM()` (Mover.cpp:1099),
  which covers both variants.
* **Rule:** put a state key on the part that owns the concept, and look for a Mover accessor that
  covers every subclass.

## 2026-09-20 - regressions after the frame-time optimisation night

37 commits (`8bd5c9a`..`ed5ee09`) judged by frame time only. Nobody checked the cabin, the
lighting or the trainset.

### The modelled cabin covered by the low-poly interior, its light always on

* **Proof:** the cached template under `user://cache/rail_vehicle/.../303e-ep-tv_*.res` still had
  `LowPolyInterior instancer`, and its `.hash` matched the current `structure-v11` tag.
* **Cause:** `ebecb4b` set `OPTIMIZED` and `8139f8b` removed it, and neither bumped
  `structure-vN`. OPTIMIZED creates no nodes, so there was no `cabN` to hide and no material to
  dim. "Clear cache" missed `rail_vehicle`, `fiz` and `vehicle_profiles`, and `user://cache`
  survives a checkout, so every bisect step was "bad".
* **Fix:** `structure-v12`. "Clear cache" covers every `ResourceCache` (`MaszynaVehicleProfileManager`
  became `@tool`). The low-poly interior switches instancer with distance
  (`_update_model_detail()`) and restores `cabN` visibility on `e3d_loaded`.
* **Rule:** code whose output is cached on disk bumps the cache tag in the same commit, and a new
  `ResourceCache` joins "Clear cache". When code has no effect, read the cache file (`strings`,
  mtime, `.hash`) first.

### Coupled wagons drifting apart

* **Cause:** `72d3b33` skipped `update_neighbour(end, null, -1, 0.0)` as a no-op. For a coupled
  end it recomputes `Neighbours[end].distance` from `CouplerDist()` (`TrainController.cpp:423-430`,
  DynObj.cpp:7144-7154, called from DynObj.cpp:8193), which `CouplerForce()` starts from
  (Mover.cpp:4781).
* **Fix:** coupled ends refresh every frame. The saving stays for free ends. (The "nothing found"
  call is `clear_neighbour(end)` since 2026-09-30.)
* **Rule:** before caching a call as redundant, open the callee and the original line cited above
  it.

### Blotchy, then black ground

* **Measurements:** *Unshaded* was uniform (so lighting). *Normal Buffer* varied (so the material,
  on flat terrain). `grass_normal.dds` R,G is 0.500 at mip 0 and 0.530-0.532 from mip 2 (DXT).
* **Cause:** `maszyna_material_factory.gd` set `normal_scale = -5.0` (since `f4138c4`, #74), while the
  original applies the map as is (`mat_normalmap.frag:46-48`). The mip bias became a 24 degree
  tilt. `detail_normalmap.gdshader` (registered in `dc25b6f`) applied it to the combined normal of
  `grass.mat`'s detail map (`param_detail_scale: 0.00125`, `param_detail_height_scale: 0.45`,
  `mat_detail_normalmap.frag:53-59`), which made it 5x stronger and reversed.
* **Not the cause:** the three `WorldEnvironment`s (own worlds), the skydome's clouds shadow, the
  terrain normals.
* **Fix:** the override is removed, `NORMAL_MAP_DEPTH = 1.0`, and the material cache key carries
  `MaterialManager.CACHE_VERSION`.
* **Rule:** a tuning factor with no counterpart in the original scales the data's errors along
  with the data.

### Project setting shown as 0/1/2 instead of a named list

* **Cause:** `add_custom_project_setting()` in `libmaszyna.gd` returned early for an existing
  setting. `project.godot` stores only values, so the hint was lost on every restart.
* **Fix:** the value is set only when missing, and the hint and initial value are always
  registered.

## 2026-09-20 - scenery environment, fog and Skydome

### Huge terrain triangles missing under the camera

* **Cause:** `SceneryTrianglesBuilder` stored a whole triangle in its centroid's 1 km cell, and
  streaming goes by the distance to that cell (`SceneryStreamingServer.cpp:53-56`).
* **Fix:** triangles are clipped along the grid (Sutherland-Hodgman in XZ), with each cut computed
  from the lower edge end so no crack opens.
* **Rule:** whatever is streamed or culled by a cell must not reach outside it.

### A winter afternoon turning the fog into orange milk

* **Cause:** Skydome's hard-coded windows put full day above 17.5 degrees of sun elevation and
  sunset colours up to 23.6. A winter sun at 50 N peaks at 16-19 degrees, so the day took night
  fog (24x density) and a sunset tint. The sky shader copies both.
* **Fix:** `day_full_elevation` (6), `sunset_fade_start/end_elevation` (4, 10) as uniforms and
  `gnd_skydome/*` settings.
* **Rule:** check sun-altitude thresholds against a winter day.

### Two fog layers that did not agree

* The original's fog is `1 - exp(-(z / range)^2)`, with `range = fFogEnd / max(1, Overcast * 2)`
  (`apply_fog.glsl:16`, `opengl33renderer.cpp:4685`). A depth fog complete at 1.5x the range with
  curve 1.5 fits it best.
* Volumetric fog is extinction per metre. Its density follows the distance inversely and the
  length stays fixed. Scaling the length made milk, and Skydome's shortening made it pulse.
* The volumetric fog affects the sky (`volumetric_fog_sky_affect` 1.0). The depth fog reaches the
  sky only up to `maszyna/rendering/fog_sky_height`, and rain reaches it fully.
* `fog_aerial_perspective` 1.0 left fogged objects dark at dusk, so it is off by default.
* The `PROPERTY_HINT_EXP_EASING` editor invites values like 0.01, so the value is floored in code.

### A weather change freezing the game

* **Cause:** `MaterialManager._refresh_managed_material()` wrote every material (0.7-3 MB each)
  to disk on the main thread, even without variants (169 of 805 `.mat` files have a rain one).
* **Fix:** no write on refresh, and materials without variants are skipped.

### A new `class_name` unknown to the running game

* `.godot/global_script_class_cache.cfg` is updated only by the editor's scan. Run
  `godot-double --headless --import`.

### Duplicated HUD in the demo scenes

* `TopBar`, `ControlWindows` and the menu were pasted into `demo_3d` and `demo_scenery_loading`.
  They are one scene now (`demo/hud/game_hud.tscn`), and a scene adds its own `Button` under
  `MenuActions`.

## 2026-09-20 - material shaders missing from the wrapper

### "Shader is not supported: Default_1 / reflmap"

* 26 shaders in the original (`mat_*.frag`) against 12 mapped. Missing and in use: `reflmap` (69),
  `detail_parallax_specgloss` (24), `reflmap_specgloss` (17), `default_1` (6),
  `rain_windscreen` (4), `default_detail` (3), `colored` (1).
* Names are case insensitive (`opengl33renderer.cpp:2018`, a Windows FS), so they are lowercased
  in the parser.
* **Rule:** survey the data before trusting a "supported" list. Check the age of `~/src/maszyna`
  against the game's `shaders/`.

### `texture2:` is not always the normal map

* `textureN:` binds shader slot N-1 (`material.cpp:76-81`), and the slot order differs per shader
  (`reflmap`: diffuse, reflmap; `default_detail`: diffuse, detailnormalmap; `water`: normalmap,
  dudvmap, diffuse; `detail_parallax_specgloss`: specgloss before detailnormalmap).
* Without `shader:`, `default_0/1/2` is chosen by texture count (`material.cpp:117-134`), and
  `mat_default_2.frag` is reflmap. Its second texture, also when written as `texture_normalmap:`
  (`material.cpp:60-65`), is read through alpha. About 2500 of 8633 shaderless materials bind one.
* **Fix:** `TextureMap.slots` per shader, resolved by `_texture_path()`, plus a
  `CACHE_VERSION` bump.

### `parallax_specgloss` never received its specgloss texture

* `_apply_parallax()` never set `specgloss_texture` (187 materials), so it read as white.

### Raindrops on the windscreen black as soot

* The atlas is a white rim over black, and the original does not light it
  (`dropTex.rgb * dynBright`, `mat_rain_windscreen.frag`). The port put it in `ALBEDO`.
* **Fix:** `EMISSION`, with the blurred screen luminance standing in for the ambient.
* **Rule:** check what the original multiplies by before moving an unlit term into `ALBEDO`.

### Wipers: data traps

* Only `e186_v2` (and the Vectron cab) has both the `rain_windscreen` glass and wipers, so test
  wiping there.
* `e186_v2/eu47.fiz` ends `WiperList:` with `endL`. The original bounds it by `Size=`
  (Train.cpp:2643), and so does the parser now.
* A script cannot read the shader `TIME`. Pass the elapsed time, not a moment.

### Wiped edge running away from the wiper blade

* A probe read `szyby_wipermask` under the blade: 0.22 ... 1.0 along an sRGB curve. The original
  declares the mask `sRGB_A`, and the port lacked `source_color`.
* The arms are eased (`smoothInterpolate`), and the wrapper feeds the eased value to the shader.
* **Rule:** port every sampler's `#texture (name, index, FORMAT)`. Measure the data under a moving
  part before tuning the motion.

### A whole layer of droplets popping in after a wipe

* **Cause (read, not measured):** the original's `GetMixFactor()` drops the wiper once the factor
  reaches 1, `side` returns to 0, and `side` seeds the droplets, so they are all re-dealt at once.
* **Fix:** the first wiper is kept at factor 1, and returning droplets fade in.

## 2026-09-20 - E186 (dynamic/pkp/e186_v2) not starting up

### Ctrl+J did nothing

* The E186 MMD has no `cabactivation_sw:`, and keys are polled by cab widgets. The original runs
  `OnCommand_cabactivationtoggle` regardless (Train.cpp:3077).
* **Fix:** `LegacyCabinCabActivation`, added by `LegacyCabinLogicDelegate`, like
  `LegacyCabinBattery`.
* **Rule:** every `OnCommand_*` works without its gauge. A control mapped only in
  `MmdSemanticCatalog` is dead in a cab that does not model it.
* **Correction (2026-10-05):** not every one - about thirty handlers return early when the cab has
  no gauge for them (`ggX.SubModel == nullptr`: the horns, sanding, the cab light dimmer, the
  instrument/dashboard/timetable lights, train heating, lowering all pantographs, line contactors,
  doors; Train.cpp:2284, 7934, ...), and a few only log it with the `return` commented out
  (`fuse_bt` 5154, `converterfuse_bt` 4523). Such entries are `requires_gauge` in the catalog; a cab
  without the gauge takes no key for them.

### Main tank empty within a minute and a half

* **Proof:** pipe 4.4 -> 3.0 bar in 30 s (EP07: 0.03), and `brake_emergency_valve_flow` 0.34. The
  unacknowledged SHP braked, and the handle refilled the pipe from the main tank (`bPantKurek3`).
* **Cause:** `EmergencyCutsOffHandle = false; //@TODO` in `TrainBrake.cpp`, while the FIZ says
  `Yes` (Mover.cpp:10508, `lock_new` at Mover.cpp:4534).
* **Fix:** `TrainBrake.main_pipe_emergency_cuts_off_handle`.
* **Trap:** the parsed FIZ is cached. Bump `FIZ_PARSER_FORMAT_VERSION` with every FIZ parser
  change.

### Pantographs raised but standing still

* E186 has single-arm pantographs with no `ramiegorne2`. The original skips a missing element
  (DynObj.cpp:5414). Only lower arm 1, upper arm 1 and the slider are needed.

### M, D and R dead in the E186 cab

* E186 models `main_sw:`, three `dir*_bt:` buttons and `shp_reset_bt:` (`SeparateAcknowledge`),
  and none of them was in the catalog.
* **Fix:** the labels are mapped (`LegacyCabinMainSwitch`, `LegacyCabinReverser`), with new
  commands `security_cabsignal_acknowledge` and `pantographs_drop_all`.
  `LegacyCabinUnmodelledControls` registers every catalog control whose key no modelled control
  has taken.

## 2026-09-20 - Scenery streaming started from the menu camera

* **Symptom:** the loading screen stayed up to 30 s after 100%, or the terrain was missing without
  it.
* **Cause:** the camera was registered at the demo position `(30, 3, 615)` before moving to the
  vehicle. A worker pass preloaded in `HashMap` order and published only at the end. Plans had no
  camera revision, and `passes > 0 && pending_builds == 0` could report completion mid-preload.
* **Fix:** streaming pauses until the final camera is known. Plans carry a camera revision, and
  preload is published nearest-first. Startup waits only for the camera's own chunk; waiting for
  its 8 neighbours meant 1000+ builds and ~15 s.
* **Rule:** readiness describes built content for a specific camera revision. An empty handoff
  queue is not proof that planning has finished.

### Global transform requested while an E3D node leaves the tree

* **Cause:** `E3DModelInstance` forwarded `global_transform` on the notification while its RID was
  valid, but the node was already out of the tree.
* **Fix:** it forwards only while the node is in the tree.
* **Rule:** a valid RID does not imply that its `Node3D` has a global transform.

## 2026-09-20 - Skydome clouds behind alpha-blended cabin windows

* **Symptom:** visible cloud cover in a cabin with alpha-blended windows pushes a 60 FPS frame
  past V-Sync to 30 FPS.
* **Cause:** the cost of `light_angular_distance` under PSSM, even with the medium filter.
* **Fix:** the fastest filter. The option to disable it is in TODO.md.

## 2026-09-20 - double slips impassable and painted with the missing-texture checker

* **Symptom:** a train stops dead at a double slip, which renders a fan of `missing_texture.png`
  quads.
* **Proof:** a double slip is four `track switch` nodes `_a/_b/_c/_d` (`TTrack::DoubleSlip()`,
  Track.cpp:2593), not `track cross`, which is a road (Track.cpp:419). Of 4819 switches, 1494
  (31%) have branch ends 0.19-0.21 m apart. `_ENDPOINT_EPSILON` was 0.25 m.
* **Cause:** the switch's own ends merged, and `_merge_endpoint_nodes()` chain-merged all 8 into
  one node. `_get_motion_connection()` returned `null` as ambiguous, `_move_vehicle_state()`
  `break`s silently, and `rebuild_track_stitches()` built the fan. The original uses 2 cm per axis
  (`Equal()`, Track.cpp:2121).
* **Checker:** the `.scn` sentinel `none` was kept as a material name. The original uses a null
  handle (Track.cpp:491) and borrows the trackbed from a neighbour
  (`copy_adjacent_trackbed_material()`, Track.cpp:3326).
* **Fix:** a 2 cm per-axis tolerance with its own hash cell, `none` -> empty, and
  `copy_adjacent_trackbed_material()` ported. Over 26960 endpoints only 2 fell in the 2-25 cm
  band.
* **Rules:**
  * A ported tolerance carries the original's value. A "safer" round number merges geometry the
    data placed deliberately.
  * A movement step that cannot resolve the next track must say so.

## 2026-09-21 - the main brake hiss has no interior/exterior distinction to key off

* **Proof:** only 1 of 1344 `airsound*` declarations sets `placement:`. The rest default to
  `general`, which `TrainSoundSystem._soundproofing()` short-circuits to 1.0.
* **Fix:** `pipe_hiss`, `local_brake_hiss` and `emergency_brake_hiss` get a
  `listener_inside` -> GAIN modulation baked by `MaszynaBrakeSfxEventFactory`, fed 0/1 from
  `_inside_vehicle()`.
* **Rule:** check what the MMD data declares before modulating a sound with a parameter.

## 2026-09-21 - a +38 dB SfxTrack under the cab hiss, and five rounds of guessing instead of one dump

* **Symptom:** the main brake release hiss is deafening in the cab of every FV4a vehicle, and no
  gain change was audible.
* **Found by:** the Remote tree. `pipe_hiss` (`airsound2`) had `track.volume_db = 38`, while every
  other track was ≤ 0.
* **Cause:** `_signed_flow_automation()` computed
  `maximum_gain = output_scale * (offset + factor * gain_signal_max)` as the curve divisor and
  then also applied it as `volume_db`. For su45: factor 40000, `maximum_gain` 79.98, 38.06 dB.
* **Fix:** `_track_gain()` caps the track at `minf(maximum_gain, 1.0)`, so `maximum_gain` works as
  a divisor only.
* **Rules:**
  * Dump the whole built bank (every event, each clip's `track.volume_db`) before touching a sound
    constant.
  * A gain derived as a normalisation divisor must never also be applied as a gain.

## 2026-09-21 - distant buildings cut out of the fogged sky, whatever the fog distance

* **Proof:** the sky pixels were exactly the fog colour `(149, 113, 95)`, and distant objects came
  out brighter `(170, 133, 113)`. That is only possible with a fog amount above 1.
* **Cause:** `fog_density` was applied twice (scaling `day/night_fog_density` and as
  `storm_fog_intensity`), and Skydome sums the two (`Skydome.gd:1265`, clamp 1.5). Godot does not
  clamp `fog_amount`. Separately, `fog_sky_affect` ignored the density.
* **Fix:** the base density stays Skydome's haze (0.005/0.02), and the storm carries
  `fog_density - base_density`. `sky_affect` is multiplied by the opacity.
* **Rules:**
  * An opacity summed from two sources is summed where it is set. An unclamped value turns a blend
    into an extrapolation.
  * The sky and the geometry are fogged by different parameters (`fog_sky_affect` vs
    `fog_density`), and they agree only when the sky carries the depth fog's opacity.

## 2026-09-21 - the whole scenery unlit, day and night, since the OPTIMIZED instancer

* **Data:** model-node `lights` modes across 975 files: `ls_Dark` 3180, `ls_Off` 1797, `ls_On`
  733, `ls_Home` 607, `ls_Blink` 15. `stary_jawor_noc` alone has 1001 light-bearing models, 1430
  groups and 452 `FREE_SPOTLIGHT`.
* **Cause:**
  * `e3d_parser.cpp` hides `light_on*`, and `lights_state` lived on the node, while scenery models
    are node-less RIDs since `2125898`.
  * The importer dropped `lights`/`lightcolors` (`obj.lights` commented out since `923b293`).
* **Fix:** the light state lives in `E3DRenderingServer`, resolved against the time of day and
  the light level from `MaszynaEnvironmentNode`. Real lights are server RIDs streamed with their
  own range. The `light_onNN`/`light_offNN` pairing is `E3DLightFactory::discover()`.
* **Rule:** state an instancer must honour belongs to the server. A light that is never switched
  on looks exactly like one never implemented.

### Godot's spot cone stops at 90 degrees, the data's does not

* 6 of 871 `FREE_SPOTLIGHT` are wider: `elektryczne/lampa_parkowa01` at 117 degrees,
  `nastawnie/nastawnia_laziska_huta_lh1` at 150, and 4 more. They become omni lights.
* **Rule:** check the data's value range (a histogram) before mapping a parameter one to one.

### A street lamp that lights nothing

* Nine `latarnia*` models have no `FREE_SPOTLIGHT`, and in the original they only drew a glare
  (`opengl33renderer.cpp:4646`). Their halo billboard (`elektryczne/poswiata`) and ground quad
  (`elektryczne/light1|2`) are found by material, never by name.
* The halo carries the lamp colour: mercury `(0.61, 0.59, 1.0)` for `betdziur`, sodium
  `(1.0, 0.66, 0.18)` for `lbc`/`str`, warm `(0.90, 0.84, 0.64)` for `drew`/`hs`.
* **Rule:** read the geometry drawn for a glow before adding a tuning constant.

### The lit patch says how wide the cone is, not where the light ends

* The double-armed `latarniay_*` have two halos (z ±0.76 on `str`, ±0.99 on `betdziur`), and the
  quirk took the first. That affected 26 of 124 lamps in `stary_jawor_noc`.
* The patch is 15.0 m across in all nine models, while its other axis covers the arms
  (15.1-22.0 m), so `max(x, z)` was wrong.
* The patch edge is not the range. Declared street lamps use 40 m (`lampa_parkowa01`, mounted at
  4.9 m) or 80 m (`linia053/lamp-y`, `lamp-5`, `lamp-i`).
* **Rules:**
  * The patch's width is data; its edge is not a falloff radius.
  * A constant identical across a whole family is the one the author meant - for that family; it is
    read from the data, never generalised to models outside it.

### A RenderingServer light is not a Light3D - it inherits none of the node's defaults

* **Symptom:** shadow acne bands radiating from the lamp.
* **Cause:** `spot_light_create()` starts with the server's defaults: reverse cull face on, and
  biases other than the node's (spot 0.03 / omni 0.1, normal bias 1.0). The project setting
  `lights_shadow_reverse_cull_face=false` was read but never written into the light.
* **Fix:** every shadow parameter is set explicitly after creation.
* **Rule:** porting from a node to a RID carries nothing over. Set each parameter the node's
  constructor sets.

## 2026-09-21 - "the release runs old GDScript" - the release was never unpacked into the game dir

* **Proof:** the zip's `.so` matched `demo/bin/...` and the pck's `build_number.txt` was fresh. The
  game dir's `reloaded` had mtime 23:11 and md5 `5aa4a837...` against a fresh `4fa78d7a...`, and
  the repo root held the unpacked files.
* **Cause:** `upgrade-*.sh` ran `cd <repo> && ... && unzip -o <zip>`, which unpacked into the repo.
* **Fix:** `make -C "$REPO"` and `unzip -o "$REPO/bin/linux/<zip>" -d "$GAME"`.
* **Rules:**
  * When a build "has no effect", prove the binary being run is the one built (mtime, md5).
  * A one-liner that `cd`s and uses a relative destination has two working directories. Name the
    destination absolutely.

## 2026-09-21 - no fog in the exported release, perfect fog in the editor

* **Symptom:** no fog at 80 m and a white-out at 4160 m in the release. The editor is fine.
* **Ruled out:** a stale binary. The symlinked addons (`gnd_skydome`, `gnd_weather`, `gnd_sfx`,
  `libmaszyna`) are exported with their content.
* **Instrument traps:** `grep -c` on a binary counts lines. `FileAccess.file_exists()` on a `.gd`
  in a pck is false (`.gdc` + `.gd.remap`), so inspect the pck with `load_resource_pack()` +
  `DirAccess`.
* **Cause:** `Script.get_property_default_value()` returns null for every property of a GDScript
  compiled into a pck. The control was `maszyna_model_data.gd`. `SkydomeSettings.get_value()` falls
  back to it, and the `gnd_skydome/*` settings do not exist in a release, so every value became 0.
* **Fix:** `GndSkydomeMaszynaEnvironment._skydome_value()` falls back to the live Skydome node's
  value, cached on first read because five call sites write it back scaled.
* **Rules:**
  * A default that exists only in the script's source does not survive export. Fall back to a
    live object.
  * A setting registered by an EditorPlugin does not exist in an export unless it is in
    `project.godot`, and values equal to the default are exactly the ones kept out.
  * Test the invariant, not the intermediate (density × length, not density).

## 2026-09-27 - EN57's motor car refused its line breaker: no master controller positions

* **Symptom:** EN57 `s` had its pantographs up at 3300 V and reported `main_switch_closable`, but
  `main_switch` was refused.
* **Proof:** `s`'s dump read `MainCtrlPosNo` 0. `MainSwitch_()` and `DirectionForward()` do
  nothing when it is 0 (Mover.cpp). The FIZ gives `MCPN=3` in `Cntrl.`, which EN57 keeps in the
  brake include that comes *after* `Engine:`.
* **Cause:** `FizTrainCntrlParser` only stored the section for `Engine:` to apply when it created
  the engine node; with the order reversed nobody applied it.
* **Fix:** `Cntrl.` applies its engine subset to an engine that already exists.
* **Rule:** a FIZ section is applied whatever the order the file gives it in.

## 2026-09-27 - EN57 vented its pipe from the rear cab and did not drive

* **Symptom:** the AI prepared EN57 from `ra`: pantographs up, line breaker closed, but the brake
  pipe fell to 0.2 bar and the driver never became ready.
* **Proof:** `rb` (the rear cab car) had `CabActive` 0 and its emergency valve open -
  `(0 == CabActive) && (InactiveCabFlag & emergencybrake)` (Mover.cpp:4585). A `cab_activation`
  sent to `ra` after load reached `s` and `rb`; the AI's own one, at t=0.09 s, did not. A frame
  trace showed three causes in a row:
  1. `MoverVehicleController` switched the cab on while the vehicle was created, before it was
     coupled; the AI's hint then found it on and did nothing, so `CabActivisation()` never went
     along the unit. The original starts with `CabActive = 0` (MOVER.h:2090).
  2. `TrainSet3D` coupled in its own `_process` once the controllers existed - a frame after the
     drivers were attached, and coupling earlier (at the instancer) found the vehicles not yet on
     their tracks: every coupler of `ra` stayed stretched and broke after 4 s, pulling the alarm
     chain.
  3. With `rb` active, its alerter (enabled for every vehicle by the component's `enabled`,
     `MoverVehicleSecuritySystem::_apply_configuration()`) saw the cab activated, started the cab
     signalling nobody acknowledged and braked. The original enables the alerter only in
     `CabActivisation()` of the master cab (Mover.cpp:2905).
* **Fix:** no cab activation at creation; `SceneryInstancer._wait_for_vehicles()` waits until the
  vehicles stand on their tracks and `_build_drivers()` couples the trainsets (`TrainSet3D.couple()`)
  before any driver exists; the alerter is left to the cab's activation. Since 2026-09-30
  `RailVehicleServer.trainset_place()` stands and couples a trainset's vehicles, and
  `_wait_for_vehicles()` awaits each vehicle's `vehicle_built`.
* **Rule:** a command sent along the couplers is sent once the trainset is coupled and placed; a
  configuration never sets what the original switches at run time.

## 2026-09-27 - a control car had no controller: MCPN lived on the engine

* **Symptom:** EN57 `ra` (no engine) could not turn its reverser; `DirectionForward()` and
  `MainSwitch_()` refuse with `MainCtrlPosNo == 0`.
* **Cause:** MCPN/SCPN, the controller delays and `CoupledCtrl` were `VehicleEngine` properties,
  though `LoadFIZ_Cntrl` reads them for every vehicle (Mover.cpp:10837-10869). The diesel-electric
  engine also derived `MainCtrlPosNo` from its WWList, which the original never does (every FIZ
  checked has MCPN equal to it).
* **Fix:** `VehicleMasterController` component, created from every `Cntrl.` section.
* **Rule:** a field goes where the original's loader reads it, not where its first consumer is.


## 2026-09-27 - the AI drove the cab the player started the scenery in

* **Symptom:** a scenery started with the player in a trainset's cab; the AI driver kept operating
  its controls. Leaving the cab and entering again gave it to the player.
* **Cause:** the player's `_process` enters the start vehicle as soon as it has a controller -
  while `SceneryInstancer._wait_for_vehicles()` still awaits frames, before `_build_drivers()`.
  `RailVehicle3D.enter_cabin()` then called `DriverSystem.vehicle_set_control_active(rid, false)`
  on a vehicle without a driver, which returned without recording anything; the driver created
  next started with its own `control_active = true`.
* **Fix:** `DriverSystem` keeps the vehicles a player drives per vehicle
  (`player_controlled_vehicles`), recorded whether the vehicle has a driver yet or not;
  `vehicle_is_control_active()` reads it, so a driver attached later starts not driving.
* **Rule:** a fact about a vehicle is kept per vehicle, never on an object that may not exist yet
  when the fact is set - otherwise the order of creation decides whether it is lost.


## 2026-09-27 - the AI stood still in the cab the player left

* **Symptom:** Stary Jawor, the SU46 the player starts in: once the player left the cab, its AI
  driver did not move it at all.
* **Cause:** the player enters before `SceneryInstancer._build_drivers()`. The cab
  (`MaszynaDynamicTrainCabin._on_vehicle_rid_changed()`) found no cab logic and attached its own,
  keeping only the vehicle's RID; `_build_drivers()` then replaced it with the driver's. The cab
  leaving the tree detached "the vehicle's" logic - the driver's - so every `CabinSystem.act()` of
  the AI found no handler ("Unknown cabin control") and it never got past `PREPARE_ENGINE`.
* **Fix:** the cab keeps the logic it attached and detaches it only while it is still the one
  attached (`test_a_cab_leaving_keeps_the_logic_that_replaced_its_own`).
* **Rule:** whoever attaches something shared per vehicle takes away only what it attached -
  another owner may have replaced it meanwhile.


## 2026-09-28 - cab Python screens could not open their images

* **Symptom:** the EN57 cab screens (`rozklad_aksel.py`, `screen_en57al.py`) raised in the Python
  worker: `image not found: "./dynamic/pkp/en71aks_v1/cab/ekran"` (reported as `NameError:
  FileNotFoundError` - a Python 3 name in a Python 2 script) and `IOError ... WS_gotowosc.png`.
* **Cause:** two traps in the data. `akl_ra.mmd:247` passes `parameters: tex="./dynamic/..."`:
  the original's `cParser::findQuotes()` (parser.cpp:248, 479) glues quoted text to the token and
  drops the quotes, `MaszynaParser` had no quote handling and kept them in the path. And the
  scripts name `WS_gotowosc.png` for `ws_gotowosc.png` on disk - fine on Windows, whose file
  names ignore case, not on Linux.
* **Fix:** `MaszynaParser::_read_token()` reads quoted text as `readQuotes()` does (no stops or
  comments inside, backslash escapes); `PythonScreenServer`'s worker resolves a missing path
  letter case aside, one directory at a time, for `open()` and `os.path.isfile()`.
* **Rule:** the game data is written against Windows and `cParser` - quoted text is one token
  without quotes, and a file name matches letter case aside.


## 2026-09-28 - the ED72 could not be started, and the fix did not take

* **Symptom:** zwierzyniec_ed72.scn, ED72 cab: no pantographs, no main switch, no lights; after the
  fix a headless probe still read `get_power_flag()=0` on the driving car.
* **Cause:** the ED72's battery is only in 5bs-rb (the other cars say `BatteryStart=Disabled`), its
  24V reaches the unit over the couplers (`PowerCouplersCheck()`, Mover.cpp:1872), which pass it
  only with `PowerFlag & power24v`. The original's default is `power110v | power24v`
  (MOVER.h:1215); `RailVehicleBuffCoupl.power_flag` defaulted to 0, and no ED72 FIZ sets the key.
  The probe then read a FIZ cache entry written at the same minute by the operator's game, which
  still had the old library loaded but already the new scripts - the old default saved under the
  bumped `FIZ_PARSER_FORMAT_VERSION`.
* **Fix:** `power_flag` defaults to `POWER_24V | POWER_110V`; `FIZ_PARSER_FORMAT_VERSION` bumped
  once more past the poisoned entries. Measured: battery -> 24V on every car, pantographs 3600 V on
  the motor car, main switch, converter, 110V, lights.
* **Rule:** a property default is the original's default for the absent key; and a cache bump only
  holds when no process with the old library can write under the new version.


## 2026-09-28 - the ED72's controller did not reach its motor cars

* **Symptom:** ED72 in zwierzyniec_ed72.scn: the cab's master controller moved, but the motor car
  (sa) never took a position; `get ED72-010sa` showed `"cabin": 0` while the driving car had 1.
* **Cause:** `IncMainCtrl()` refuses on a vehicle with `CabActive == 0` (Mover.cpp). The motor
  cars get it from the driving car's `CabActivisation()`, which sends it along the couplers
  (`SendCtrlToNext`, Mover.cpp:2905, 12425). The player entered the cab - and activated it
  (`RailVehicle3D::enter_cabin()` -> `cab_activation_auto()`) - while the scenery was still
  loading: `demo_scenery_loading.gd` set `start_train_id` before `load()`, and the player's
  `_process` took the vehicle as soon as it had a controller, before `_build_drivers()` coupled the
  trainsets. The activation went out over couplers not connected yet.
* **Fix:** the scene hands the player its train on `scenery_loaded`; `MaszynaPlayer.auto_start`
  (off in that scene) keeps it from taking the first vehicle on its own meanwhile.
* **Rule:** anything that sends along the couplers happens only once the trainset is coupled -
  the player takes a vehicle only after the scenery says it is loaded.


## 2026-09-28 - every rear coupler held 1 kN

* **Symptom:** the ED72 pulled away and tore apart at once: sb `coupler_stretched`, `train_damage`
  8 (dtrain_coupling), the alarm chain flag, rb left behind with its own pipe and battery - the
  brake pipe and the 24V of the rest lost.
* **Cause:** `FizVehicleBuilder.build_model_at()` saved the parsed vehicle one component per type
  (`root.get_component(type)`), so of `BuffCoupl1.`/`BuffCoupl2.` only the first reached the model
  and the vehicle. The rear coupler kept the Mover's defaults (Mover.cpp:487-488: SpringKC 1,
  FmaxC 1000 N) - `vehicle_dump_config()` showed `coupler_max_force=[780000.0, 1000.0]` on every
  car. Any pull stretched it past FmaxC and, after the Mover's one second of leeway, broke it
  (Mover.cpp:4843-4857). Two coupler components also both registered the same (stub) commands.
* **Fix:** `VehicleController.find_components(type)`; the builder captures every component of a
  type; the stub `buffer_couple`/`buffer_decouple` commands removed. Measured: 780 kN at both ends
  of every ED72 car. Test: `test_two_coupler_sections_reach_both_ends`.
* **Rule:** a vehicle may carry several components of one type - never reduce them to one per type
  when saving, copying or listing them.


## 2026-09-28 - the ED72 never went into the field shunt

* **Symptom:** the ED72 ran up to 36-43 km/h on the master controller's top position and no
  further; the shunt keys did nothing.
* **Cause:** with `CoupledCtrl=Yes` the original refuses `IncScndCtrl()` (Mover.cpp:2531) - the
  master controller's shaft goes on into the shunt past its last main position (Mover.cpp:2335),
  and the cab counts its range as `MainCtrlPosNo + ScndCtrlPosNo` and its position as `MainCtrlPos
  + ScndCtrlPos` (Train.cpp:985, 1133, 9410). The catalog's `mainctrl` capped the cab widget at
  `main_controller_position_max` (3), so the fourth press never went out.
* **Fix:** `master_controller_position_max` and `master_controller_position` from the master
  controller component, main + shunt when coupled; `mainctrl` uses them. Measured: position 6
  (main 3, shunt 3), camshaft to 14, past 50 km/h and still accelerating.
* **Rule:** a coupled controller's cab range is main + shunt - check `CoupledCtrl` before taking a
  controller's range from one table.


## 2026-09-28 - the cant tilted the rails the other way than the vehicles (#292)

* **Symptom:** on every canted curve the body leaned the opposite way to the track and the wheels
  of one side hung above the rail.
* **Cause:** `RailVehicleServer._placement_transform()` rolls the vehicle as `DynObj.cpp:2694`
  does (the right side of the track direction down on a positive roll). The track renderer copied
  the original's rail and trackbed cross-sections (`Track.cpp:2652`), but its loft frame
  (`_build_curve_frame`) maps the profile's x to the *left* of the track direction, where the
  original's `RenderLoft` maps it to the right (`Segment.cpp:398`, `parallel = (-dir.z, 0,
  dir.x)`) - the profile was mirrored and the cant with it. A probe on a straight track along +X
  with a 5 deg roll: the vehicle's wheel on +Z at 0.0028 m, the rendered rail on +Z at 0.128 m -
  the high side. The switch trackbed (`_build_transition_loft_strip_chunks`) used `parallel` and
  tilted like the vehicle, the plain trackbed the other way.
* **Fix:** the rail and trackbed sections turn the cant the other way to match the mirrored
  frame; the switch trackbed maps x to the left like every other loft. Measured: rail at +Z
  0.0028 m, at -Z 0.128 m, the vehicle's wheel on +Z at 0.0028 m.
* **Rule:** a ported formula carries the original's frame with it - compare world coordinates of
  both sides, not the formulas.

## 2026-09-28 - the trackbed hung over the terrain: its slopes sampled the texture's transparent edge

* **Symptom:** after the self-shadowing fix below, tracks still stood on a gap above the terrain
  (elektrocieplownia at the `test_zwr07` lever, Glinojeck); the lever and the vehicles sat right
  on the rails, and the original showed the ballast running flat out under the lever.
* **Proof:** the built vertices were right (bed 0.000..0.210, terrain 0.000 under and beside it,
  read from the real renderer's mesh arrays), but an orthographic cross-section drew the slopes
  only halfway down. The autumn ballast texture (`1435mm/tpd-stone4-old3_autumn`, DXT5) is fully
  transparent for u < 0.13 and u > 0.87. Our bed mapped u with the track's `tex_length` 4, so
  the slopes reached u -0.08..1.08 and most of each was cut away; the original takes the length
  from the material's `size: 6 6` (`texture_length()`, Track.cpp:2481-2493), u 0.111..0.889, and
  keeps a fixed old mapping for exactly 4 m (Track.cpp:2855-2926).
* **Fix:** `MaszynaMaterial.size` is the original's float with -1 for none; the trackbed resolves
  its texture length with its material and uses it for the section mapping, the old mapping at
  4 m, and the V along plain beds, switch beds and stitches. Measured: u 0.111..0.889, the slope
  drawn down to the terrain.
* **Rule:** a gap that the vertices do not show is a cut-out: look at the texture's alpha where
  the UVs land. A value the original takes from the material (`size:`) is not the scenery's
  number of the same name.

## 2026-09-28 - a gap between the trackbed and the terrain: the bed shaded its own slopes

* **Symptom:** on elektrocieplownia (near the SM42, `tor_53`) the ballast bed seemed to stand
  above the terrain with a dark gap between them; the original shows the same bed flush.
* **Proof:** the data puts every track point 0.2 m over terrain at 0.0 with `tex_height 0.2`, and
  the built bed ends exactly at y 0.000 - geometry as in the original. An off-screen render
  (`xvfb-run`) beside the track, grass hidden: the slope facing the camera at 0.21 against the
  terrain's 0.33. A camera on the slope aimed at the sun saw only sky - no caster. Turning off
  shadow casting for the rails or the stitches changed nothing, for the bed alone gave 0.34; a
  back-culled bed material changed nothing, and only a sun bias of 5 cleared it. The slope
  (0.2 m over 1.1 m) is lit at N.L 0.25 by a 22.7 deg sun: acne of the bed on itself.
* **Fix:** the trackbed and its stitches cast no sun shadow (`create_track()`); the rails still
  do. Measured: slope 0.285 against terrain 0.294, 0.293 with no shadows at all.
* **Rule:** a surface lying on the ground and lit at a grazing angle shadows itself; do not
  raise the light's bias for it (09-27 thin objects), take the surface out of the casters. Before
  blaming the geometry, render the spot with its shadow off.

## 2026-09-28 - switch trackbed dark after the cant fix, and ballast wings at every switch joint

* **Symptom:** on td.scn the ballast of both switches was much darker than plain ballast (before
  `02c54f6a1` made tracks `cull_disabled` it was not drawn at all), and at each switch end thin
  ballast triangles stuck out several metres to both sides.
* **Proof:** a headless probe over td.scn's tracks compared each trackbed triangle's front face
  (Godot: clockwise) with its vertex normals: plain beds 112/112 agreeing, switch beds 0/192 with
  the face pointing down (-0.99). The same probe paired the two sections of every stitch at a
  switch: 0.6 m apart in the middle, 3.7 m at the outer edges - the switch side was about 6 m
  wide. `zwr01` declares `tex_width 2.75 tex_slope 2.5`, its neighbours `0.5 1.1`; the switch bed
  takes the neighbour's profile (`Track.cpp:2753-2809`), the stitch took the switch's own.
* **Fix:** the switch strip is wound like `_append_loft_strip_indices()` now that it maps the
  profile's x to the left (the reversed winding belonged to the right-hand mapping the cant fix
  removed), and the stitch section at a switch endpoint comes from
  `_build_switch_trackbed_sections()`, the bed's own. Measured: switch beds 192/192 facing up,
  every stitch pair 0.57-0.62 m apart.
* **Rule:** mirroring a loft's cross vector flips its triangle winding too - with culling off
  the face does not vanish, it is lit from below. And geometry that joins two meshes takes each
  side's section from the code that built that mesh, not from the raw track data.


## 2026-09-28 - no vehicle without DoorPermitList could permit its doors from the cab

* **Symptom:** the new door permit switches (`LegacyCabinDoorPermits`) did nothing on the Impuls
  36WEa, whose FIZ has `DoorNeedPermit=Yes` and no `DoorPermitList`.
* **Cause:** `RailVehicleDoors.permit_list` defaulted to `[0, 0, 0]`, so
  `MoverRailVehicleDoors::_apply_configuration()` gave every such vehicle three permit presets. The
  original has none unless `DoorPermitList` names them (Mover.cpp:10559), and its permit switches
  step aside for the presets (Train.cpp:7203). The dump showed `doors_permit_preset_count=3`.
* **Fix:** the default is an empty list; `FIZ_PARSER_FORMAT_VERSION` 26 drops the cached
  vehicles built with it. Measured: `doors_permit_preset_count=0`, the switch permits the left
  doors and the left mirror unfolds.
* **Rule:** a component's default is what a vehicle without the key gets - it must be the
  original's default.


## 2026-09-28 - a diesel that is off puffed smoke the first time the camera saw it

* **Symptom:** a parked diesel with its engine off gave one puff of smoke the first time the camera
  looked at it.
* **Cause:** read from the code, not measured. `E3DRenderingServer::SmokeObject::intensity`
  defaulted to `1.0`, and the emitters are built with the model. The engine's rate (0 when off)
  came only from `RailVehicle3D::_update_smoke()` in its 0.25 s block, and on the first load even
  later: the model builds in its own `_ready()`, before the vehicle wires `model_node`, and the
  first `process_frame` after the loading frame has a long delta. Every detail switch
  (`reload()`, as the camera comes close) built new emitters at `1.0` again. Probably (not checked
  in the Godot source) Godot processes a culled particle system only once it is in view, so the
  queued `particles_emit()` requests all appear when the vehicle first enters the view.
* **Fix:** the instance owns `smoke_intensity`; an `INSTANCE_KIND_DYNAMIC` instance starts at 0
  and a rebuild keeps the last value. The vehicle sets the rate when the model loads and when it
  wires the model, and a vehicle without a diesel sets `1.0` itself.
* **Rule:** an emitter whose rate its owner drives spawns nothing until the owner has set it.

## 2026-09-28 - trackbed stitches flickered at every track and switch joint

* **Symptom:** the ballast over each joint between two tracks, or between a switch and a track,
  flickered like two coplanar surfaces fighting in depth.
* **Proof:** a headless probe over td.scn cast a vertical ray from every stitch vertex and
  triangle centre onto the beds of the tracks it joined. Every plain-joint sample that had a bed
  below it (2398 of 2398) sat 8-12 mm above that bed. The stitch took its sections 0.5 m inside
  each curve and lifted them 1 cm, so it lay as a second, nearly coplanar bed over the last
  0.5 m of both neighbours: 215 m2 of overlap at plain joints and 30 m2 at switches. With the
  sections moved to the endpoints, the stitches covered only 2.2 m2 and 0.3 m2, and the two
  beds' end sections were at most 4.9 cm apart (most at 2.2 cm, the original's endpoint tolerance,
  `Equal()`, Track.cpp:2121). The original has no stitch geometry at all.
* **Fix:** the stitches are removed: their mesh and instance, `rebuild_track_stitches()` and its
  helpers. The beds meet the way they do in the original.
* **Rule:** do not lay a surface over another with a small lift to hide a seam, because the two
  fight in depth. Before you keep geometry the original does not have, measure what it covers once
  the bugs it was added to hide have been fixed.

## 2026-09-28 - the EP07 trip test never moved: a driver held it and no cab was active

* **Symptom:** `test_zzz_ep07_main_switch_trip_diagnostic.gd` failed with the vehicle at 0 m/s
  after five `main_controller_increase` commands, `controller_main_position` staying 0 - on the
  real td.scn since `9d9bff094` and on its fixture cut alike.
* **Proof:** `DriverSystem.vehicle_is_control_active()` was true (td.scn gives EP07-424 a
  `headdriver`), and after taking the control the position still stayed 0: `IncMainCtrl()` refuses
  every step while `CabActive == 0` (Mover.cpp:2226), and nothing had activated a cab.
* **Fix:** the test takes the vehicle from its driver, as a player does
  (`vehicle_set_control_active(false)`, drivermode.cpp:266), and sends `cab_activation` after the
  battery; it then reaches 35 km/h on notch 6.
* **Rule:** a test that drives a scenery vehicle by commands takes it from its driver and activates
  a cab first - neither happens without a player entering the cab.

## 2026-09-28 - engine silent after the camera came back, or restarted from ignition

* **Symptom:** after the camera watched a vehicle farther than 1 km away, the player's own running
  engine (and compressor, pumps) stayed silent when the camera came back. When the camera reached
  a vehicle whose engine had been running for a while, the engine started with its ignition clip.
* **Proof:** read off `TrainSoundSystem._refresh_active_banks()`: culling stopped the bank's
  player and nothing else, so each `Trigger.activated` stayed `true` and `_update_triggers()` never
  called `play()` again. Brake and running sounds came back because they check
  `player.is_playing()`. Every trigger start was `play()` at event time 0, and the engine event puts
  its ignition bookend at 0. The original clears `m_playbeginning` when a sound is due out of range
  (sound.cpp:360-367), and only a simulation-side `stop()` sets it again (sound.cpp:498).
* **Fix:** `_silence_bank()` stops the player and resets each trigger (`activated`,
  `play_beginning`, `last_value`). Culling and the cabin-only bank use it. A trigger starts at
  `beginning_length` when `play_beginning` is false. The system culls no farther than
  `gnd_sfx/hard_cut_distance`, beyond which `SfxPlayer3D.play()` drops the call.
* **Second cause:** the in-game check showed that the ignition still played, but only the first
  time a vehicle was heard. Starting the event at `beginning_length` asked the ignition clip to
  start at its own end. Before its first play, a `MaszynaAudioStream` reports length 0, so gnd-sfx
  could not clamp or end the voice. Godot's Ogg playback turns a seek at or past the length into
  0 (`audio_stream_ogg_vorbis.cpp:286`), so the whole ignition played. From the second play the
  length was known and the voice ended at once.
* **Second fix:** gnd-sfx does not start a timeline clip whose span has passed when an event starts
  at a position (FMOD's `setTimelinePosition`). `MmdSoundEventBuilder` gives the ignition clip its
  measured length, so the span is known before the stream is loaded.
* **Third miss:** the in-game check still heard the ignition. A headless reproduction with the
  real `6d1.mmd` (camera away, AI prepares the engine to ~500 rpm, camera back) showed the event
  starting at 10.38 s with the ignition clip active. `play()` skipped the passed clip but did not
  mark it as triggered, so the first tick found it due and started it from 0. The gnd-sfx test
  checked right after `play()`, with no tick, and with a WAV that knows its length.
* **Rule:** code that stops a player also resets what its triggers remember about playing. A sound
  that is due while out of earshot resumes past its opening bookend. Never skip a clip by starting
  it at its end: an Ogg playback starts that at 0. A skipped clip is marked as done, or the next
  tick starts it. A playback test ticks at least once, with a stream that does not know its length
  before it is loaded.

## 2026-09-28 - a sound bank counted its vehicle's coupler events under a handle nothing read

* **Symptom:** a test that registered a bank on a `RailVehicle3D` whose `controller_path` was
  already set failed with "Signal 'coupler_attached' is already connected".
* **Proof:** `RailVehicle3D.get_controller()` resolves the path at once, but `get_rid()` keeps the
  node's own handle until `_on_controller_changed` adopts the controller's
  (RailVehicle3D.cpp:451-462). `_resolve_controller()` connected under the first handle. On the
  announcement it tried again under the second, and Godot refused, because a bound callable is
  the same connection whatever its bound arguments are. Coupler and pantograph events went on
  being counted under a handle that no trigger read. The game's order (bank before controller)
  never showed it.
* **Fix:** when the vehicle's handle has changed, `_resolve_controller()` stops counting under the
  old one before it connects under the new one.
* **Rule:** a connection keyed by a bound argument is one connection per method. When the key
  changes, disconnect it before connecting again.

## 2026-09-28 - a vehicle lamp's glare blinked on and off with the viewing angle

* **Symptom:** the glare of a locomotive's headlight (SM42) did not fade as the camera went round
  it: at some angle it was either there or gone. Smoothing the cone's linear ramp changed nothing.
* **Proof:** a dump of the free spotlights (`TP_FREESPOTLIGHT`) in `dynamic/pkp/sm42_v1/*.e3d`:
  every `fspot` declares falloff 22.5 deg and hotspot 21.5 deg. The original's angle factor
  (opengl33renderer.cpp:4665) ramps between the two, so the whole fade fits into 1 deg.
* **Fix:** the glare fades over its own band, from `railway_lights_glare_fade_start` of the falloff
  angle out to the falloff, unless the hotspot starts it earlier. The point keeps the original's
  band.
* **Rule:** do not trust a model's hotspot-to-falloff band to be a fade: vehicle lamps declare a
  1 deg one. Dump the angles before tuning anything that uses the cone.

## 2026-09-29 - a thrown switch ahead held the train at a clear signal

* **Symptom:** on td.scn the EP07 stood before signal A with the engine ready; Shift+2 (keyctrl02:
  switch `zwr01-` and A at S13, 40 km/h) cleared A, the driver took `SetVelocity 40`, and still
  wanted 0 km/h until it had passed A. The driving aid showed "Stop ahead".
* **What proved it:** a headless probe of the real scenery, the driver's route read each 0.1 s:
  `velocity_limit = 0` from `signal_velocity_last = 0`, set on the update after keyctrl02. The
  events ahead before it: `stacja1_k_sem_info@1179.9` (signal K, stop); after it:
  `stacja1_j_sem_info@1180.1` - the switch had put signal J on the route instead of K, and K,
  gone from the reading inside the reach, was taken as passed.
* **Cause:** "passed" was "gone from the reading while its last distance was inside the reach" -
  a guard against the shrinking reach (09-27), blind to the route changing ahead. The original
  moves its table by the distance driven and passes a point only when its distance goes below 0;
  a thrown switch re-traces the table from it (TableCheck()).
* **Fix:** first an event was passed only once the distance driven (DistCounter) reached it;
  then, the same day, the route got the original's table: kept between updates, moved as the
  trainset drives, traced again from a switch thrown ahead (TrackRouteSegment
  `branch_from_setting`), an event passed when its distance along the table reaches the front.
  Checked on the same run: after keyctrl02 the aid shows 40 with a stop 1.2 km ahead, and driving
  on, A is passed once at 0.48 m.
* **Rule:** a point of the route is passed when the train has driven up to it, never because it
  is no longer read - the reach and the route both change without the train moving.

## 2026-09-29 - a freight train early at a station was shown 7 min late

* **Symptom:** Stary Jawor (night), ROS66383 at 21:17: Roztocze (dep. 21:20) already passed, and
  the timetable panel said "+7 min" in red - late for a station not yet due.
* **What proved it:** the original sets `LastStationLatency = CompareTime(now, departure)`, the
  departure less the arrival (mtable.cpp:122), and `UpdateDelayFlag()` takes a value below 0 as
  late (Driver.cpp:5605). The port computed the same, but its comment and all three displays
  (timetable panel, vehicle card, selector row) took a positive value as late. A freight train
  does not wait for the departure (Driver.cpp:1275), so it left Roztocze at ~21:13: 7 min early.
* **Fix:** `latency` keeps the original's value and sign; the displays show its negative.
* **Rule:** `LastStationLatency` is the departure less the arrival - positive is early; a shown
  delay is its negative.

## 2026-09-29 - shunting signal dwarfs never lit in the scenery

* **Symptom:** Stary Jawor (and other sceneries): the Tm dwarfs (`ms2nbk.inc`,
  `sem/karzelki/ktmnb`) stood by the tracks dark whatever their aspect; the main signal next to
  them switched. `mini_tm.scn` (Tm heads and a main signal on Shift+1..4) showed the same.
* **What proved it:** a headless dump of the model: both lamps sit under a transform named `_on`
  (`karzel2/_on/light_on00`), and each lamp's lens is a mesh `light_on00` nested under the
  `light_on00` transform. The parser hid every `*_on`/`*_xon` submodel and every `light_on*`
  one, so `_on` hid both lamps and the nested lens stayed hidden under a lamp that was switched.
  The original hides `*_on` only in a dynamic (vehicle) model (Model3d.cpp:275, 2221), and
  switches only the first `Light_On00` it finds (GetFromName(), AnimModel.cpp:306).
* **Fix:** the parser marks a `*_on`/`*_xon` submodel `dynamic_hidden`, and the backends hide it
  only in an `INSTANCE_KIND_DYNAMIC` instance (vehicle parts, the cab, the vehicle preview); a
  copy of a light's name nested under the bound one stays visible. Checked on `mini_tm.scn`.
* **Rule:** what a model hides by default depends on who loads it: a vehicle hides its `_on`
  controls, a scenery model does not. Read the model's tree before assuming a light is missing.

## 2026-09-29 - style-fix broke the build

* **Symptom:** PR #286 `style-check` red with 397 clang-format violations in 127 files. After
  `make style-fix` the build failed: `RailVehicle3D.hpp:91 'RailVehicleController' does not name
  a type`, followed by `incomplete type GetTypeInfo<int*>` in `binder_common.hpp`.
* **What proved it:** `scripts/style-fix` ran `clang-tidy --fix --fix-errors` file by file with
  `-line-filter` limited to that file, so `readability-identifier-naming` renamed declarations
  (`PythonApi::PyErr_Occurred` -> `py_err_occurred`, `r_point` -> `p_r_point`) without their uses
  in other files. The style-check database is configured `LIBMASZYNA_STYLE_CHECK` (single
  precision godot-cpp: `float Light3D::get_param()`), while the build is double
  (`double get_param()`), so `readability-redundant-casting` removed `static_cast<float>` casts
  the build needs. clang-format's include sorting put `RailVehicle3D.hpp` first in its `.cpp`,
  and the header compiled only thanks to an earlier include.
* **Fix:** `style-fix` runs clang-format only; `extension_api.json` (double) is versioned and
  style-check binds it like the build (`LIBMASZYNA_STYLE_CHECK` removed); the removed casts are
  restored; `RailVehicle3D.hpp` forward-declares `RailVehicleController`; the Python C API names
  are fenced with `NOLINTBEGIN/END(readability-identifier-naming)`.
* **Rule:** clang-tidy findings are fixed by hand, across the tree; every header includes or
  forward-declares what it names; every touched C++ file is formatted before commit; the style
  check sees the same precision as the build.

## 2026-09-29 - after the double extension_api.json, the debug library would not load

* **Symptom:** after pulling `70ee1b4a` (godot-cpp bound to the versioned double
  `extension_api.json`), `make compile-debug` succeeded, but Godot could not open
  `libmaszyna.debug.64.so` - "undefined symbol: _ZN5godot8Resource21_setup_local_to_sceneEv" - so
  every C++ class was missing: the tests hung, `release-linux` failed while exporting (the headless
  editor loads the debug library, not the release one).
* **What proved it:** `nm` on `build-debug/bin/libgodot-cpp...template_debug.double.x86_64.a`
  showed `Resource::_setup_local_to_scene()` only as undefined references, while the regenerated
  `gen/src/classes/resource.cpp` defines it: the archive held objects of the previous generation.
  The library was not even relinked on a second build.
* **Fix:** remove `build-debug/godot-cpp/CMakeFiles/godot-cpp.dir` and the godot-cpp archive in
  `build-debug/bin`, build again; `nm -D -C --undefined-only` on the library shows no `godot::`
  symbol afterwards. The release build dir, configured afresh, was not affected.
* **Rule:** when the API godot-cpp is generated from changes (`extension_api.json`, precision,
  `GODOT_VERSION`), rebuild godot-cpp from clean objects in every build dir; check the library with
  `nm -D -C --undefined-only ... | grep godot::` before trusting a green build.

## 2026-09-29 - an extension class named like an engine class never registered

* **Symptom:** the new `CameraServer` singleton built and linked without a warning, but none of its
  methods or constants existed in GDScript; `player.gd` failed to parse on every
  `CameraServer.camera_*` call.
* **What proved it:** `godot-double --headless --import` printed "Attempt to register extension
  class 'CameraServer', which appears to be already registered" and then "Attempt to register
  extension method ... for unexisting class" for every binding - Godot has its own `CameraServer`
  (camera feeds), and `GDREGISTER_CLASS` of the same name is refused at run time, not at build time.
* **Fix:** the class renamed `PlayerCameraServer` (singleton of the same name); Lua keeps
  `maszyna.camera`.
* **Rule:** before naming a new extension class or singleton, check that Godot has no class of that
  name (`ClassDB.class_exists()` in the editor, or the class reference); after adding one, read
  the `--import` log for "already registered".

## 2026-09-29 - every vehicle's cab ran each step

* **Symptom:** a clear fps drop in game (38 fps, `Process` 33 ms). The editor profiler's Script
  Functions put `CabinSystem._on_simulation_advanced` -> `LegacyCabinMainSwitch._process` at
  165-259 calls a frame, with `CabinState.vehicle_of` (39k of 63k self time) and `vehicle_state`
  (15k) under it.
* **What proved it:** the call count of `LegacyCabinMainSwitch._process` matched the number of
  vehicles with an MMD, not the few driven ones. `git log -S"LegacyCabinLogic.from_mmd(data_path"`
  led to 495c3538, where `MaszynaRailVehicle3D._on_controller_changed()` attached a cab logic to
  every vehicle; before, only the AI's vehicles (`SceneryInstancer._build_drivers()`) and the
  player's 3D cab had one. Each cab's `_process` behaviours walk the trainset
  (`vehicle_find_powered`) and rebuild the vehicle's state dump every step.
* **Fix:** `DriverSystem` announces `vehicle_driven_changed(vehicle, driven)` (a driver or a
  player) and answers `vehicle_is_driven()`; `MaszynaRailVehicle3D` attaches the cab logic when its
  vehicle becomes driven and detaches it when it stops. Same session: `set_end()` of the AI's lights
  skips a vehicle without `RailVehicleLighting` (the original masks lamps by `iInventory`,
  DynObj.cpp:7293) instead of flooding the log with "Unknown command: light", and reads the lamps
  with `RailVehicleLighting.light_is_enabled()` instead of the state dump; `E3DModelInstance`
  processes only in the editor and follows its transform only where the server needs it; scenery
  sounds became one streamed `SfxPlayer3D` (`ScenerySoundServer`).
* **Rule:** the original keeps a TTrain only for a driven train - a cab at work on every vehicle is
  N per-step processes. Before attaching per-vehicle logic, ask who drives the vehicle; read the
  profiler's call counts before its times.

## 2026-09-29 - submodels without meshes

* **Symptom:** in `test_t3d_parser.gd` every submodel taken as
  `E3DModelManager.load_model(...).get_node(...)` had `mesh == null`, while a probe iterating
  `model.submodels` of the same file saw its meshes.
* **What proved it:** the only difference was the model held in a variable. `~E3DModel()` calls
  `E3DModel::clear()`, which clears every submodel (`E3DSubModel::clear()` unrefs its mesh); the
  temporary model is released at the end of the expression, the submodel survives it empty.
* **Fix:** the test keeps the model in a variable while it reads the submodels.
* **Rule:** hold an `E3DModel` for as long as its submodels are used.

## 2026-09-29 - the vehicle card missing in the release build

* **Symptom:** after the card moved to the bottom left corner (13b92be0) it opened in the editor
  but not in the exported release; its row in the trainset list still lit up.
* **What proved it:** the release has `print`/`push_warning` switched off
  (`run/disable_stdout.release`, `run/disable_stderr.release`), so the probe wrote a file past the
  logger: `VehicleCards` 0x0 with anchors `[0, 0, 0, 0]` and `layout_mode=0`, the card at
  y = -566. The pack read with `godot-double --main-pack` held an override
  `GameHud/VehicleCards: layout_mode=0` in `demo_scenery_loading.scn` - converted at 10:20 from
  the old `game_hud` (`layout_mode=3`) and never again, since `demo_scenery_loading.tscn` itself
  did not change (`.godot/exported/<id>/file_cache` is keyed by the scene's own md5).
  `_set_layout_mode(POSITION)` resets the anchors to top-left.
* **Fix:** every release target (`release-linux`, `-windows`, `-android`, `-linux-symbols`)
  depends on `release-clear-godot-cache`, which deletes `demo/.godot/exported`.
* **Rule:** a release is exported from an empty export cache; a layout that differs between the
  editor and the release - dump the exported pack's `SceneState` before reading engine code.

## 2026-09-30 - Krzyżowa 2: the timetable panel never left Krzyżowa

* **Symptom:** EX6435 left Krzyżowa on time and passed Markowo_Górne; the panel stayed on
  Krzyżowa throughout. The original on the same scenery shows Krzyżowa as left (green) at 10:35.
* **What proved it:** a headless run of krzyzowa2.scn, EP09-025 driven by its AI at x10: order
  `OBEY_TRAIN`, 10 km driven, `station_index` 0 and `at_passenger_stop` false all the way. A grep
  of the scenery's own files found no W4 - they are included two levels down
  (`krzyzowa2/sc2.scm`: `include ip/pkp/w4n.inc Krzyżowa#tor6end ...`). The scenery parser
  appended each byte past ASCII as a signed char (`MaszynaParser::_to_token()`), so the cp1250
  `ż` (0xBF) became U+FFFD: the W4 read `PassengerStopPoint:Krzy\uFFFDowa`, the timetable
  `Krzyzowa`. The original cuts a W4's name at `#` and makes it plain ASCII (Event.cpp:715-719),
  as it does the timetable's (mtable.cpp:430). After the fix the same run stops at Krzyżowa
  (3 min early), leaves at 10:32 and moves on to Markowo_Górne.
* **Fix:** the parser decodes bytes past ASCII as cp1250 (the scenery, fiz and material caches'
  versions bumped); the event factory makes a W4's station plain ASCII.
  `demo/tests/test_driver_timetable_run.gd` runs EX6435 past W4 read in cp1250 by the parser.
* **Rule:** a timetable that does not move - compare the W4's station with the timetable's, byte
  for byte; and search a scenery through all its include levels.

## 2026-09-30 - the station shown did not catch up in a test run

* **Symptom:** the timetable's `station_start` (StationStart) stayed on the station left while the
  test moved the train kilometres on.
* **What proved it:** `total_distance` (Mover `DistCounter`) read 0.0 after every
  `trainset_move()`; the Mover counts it only inside `ComputeMovement()` (Mover.cpp:1393, 1460).
* **Fix:** the way driven since the stop (`fLastStopExpDist`, Driver.cpp:1131, 1281) is counted
  along the driver's route table (`_front_along`) - the same thresholds, a source a moved train
  shows too.
* **Rule:** do not count on the Mover's odometer for anything a test moves by placement.

## 2026-09-29 - blurry cab gauges

* **Symptom:** the E186 cab (td_e186.scn) showed its gauges blurred - digits and scale marks
  smeared - where the original draws them sharp.
* **What proved it:** the cab textures (`dynamic/pkp/e186_v2/kabina_a/b/c.dds`, `fst.dds`) are
  2048x2048 DXT, and `MaterialManager.load_texture()` loaded every DDS under
  `maszyna/import/dds_max_texture_size`, default 1024 - `dds_texture_loader.gd` dropped the top
  mip, so the cab ran at half resolution. The original has two limits, `iMaxTextureSize` and
  `iMaxCabTextureSize`, both 4096 (Globals.h:164-165), and loads the cab under the second
  (Train.cpp:660-666).
* **Fix:** `maszyna/import/dds_max_cab_texture_size` (default 4096); the cab's `E3DModelInstance`
  carries it as `max_texture_size` through `E3DRenderingServer.instance_set_options()` and the
  material resolver to `MaterialOptions`. Both settings' enum hints now store the size, not the
  index of the entry.
* **Rule:** a blurry texture - compare its DDS size with the limit it loaded under before blaming
  mipmaps or filtering.

## 2026-09-30 - EP07 tests driven at x20

* **Symptom:** on CI `test_zzz_ep07_cabin_main_switch`, `test_zzz_ep07_main_switch_trip_diagnostic`
  and `test_zzz_ep07_orientation_regression` failed (the vehicle never moved:
  `controller_main_actual_position` 0, brake cylinders full), and
  `test_maszyna_environment_node` ran the clock 28.5 s short of an hour; every one passed run
  alone.
* **What proved it:** the Mover's step differed between CI and a local run
  (`main_switch_time` -0.00985 against -0.00667). `test_weather_controls.gd` moves the time scale
  slider to its end, `MaszynaEnvironmentNode.simulation_speed` writes it to
  `SimulationServer.simulation_speed` (x20, x60 before) and nothing set it back, so every later
  script ran at it; `-gpre_run_script` setting x20 reproduced all four EP07 failures run alone.
  The clock test's 28.5 s is the speed ramp (`speed_change_time` 0.4 s, from `c476726a`) from
  1 to 100 at 0.25 s frames: 99 * 0.535 * 0.25 / 0.465; its neighbour passed only because the
  ramp had already been run up by it.
* **Fix:** `test_weather_controls.gd` restores the speed in `after_each`;
  `test_maszyna_environment_node.gd` sets `maszyna/simulation/speed_change_time` to 0 for its
  tests, as `test_simulation_clock.gd` does.
* **Rule:** a test that changes a server's state restores it; a failure only in the suite is
  reproduced by setting that state before the script alone.

## 2026-09-30 - track textures stayed after a game directory change

* **Symptom:** a game directory changed in the editor, then "Reload models": the models came from
  the new directory, the track textures stayed the old ones until the whole scene was reloaded.
* **What proved it:** reading who holds what was read from the game directory. "Reload models"
  called `reload()` on the `E3DModelInstance` nodes and nothing else; every other owner kept its
  memo in memory with nothing telling it the data had changed - `TrackRenderingServer` its
  tracks' materials, `MaterialManager` its handed-out materials (a lookup by the same key hands
  the same object back), `E3DRenderingServer` its loaded models (keyed by path without the game
  directory) and resolved materials, `SmokeSourceLibrary` its templates (its `clear_cache()` was
  connected to nothing), `MaszynaAudioStream` its loaded file. Two disk caches (`materials`,
  `vehicle_profiles`) had no game directory in their key, and `ResourceCache::set()` saved a new
  resource to the path of one still held, so `get()` - `ResourceLoader` in REUSE mode - handed out
  the old object again.
* **Fix:** `GameDataServer` (the caches moved there from `SimulationServer`): `data_reload()`,
  called on every game directory change and by "Reload game data", emits
  `data_unload_requested` then `data_reload_requested`, and every owner follows them itself -
  drops its memo in the first round, builds again in the second (`SceneryStreamingServer.
  owner_rebuild()` for streamed pieces). The game directory is part of both keys;
  `ResourceCache::set()` makes the saved resource take the path over.
* **Rule:** whatever reads the game directory and keeps the result follows `GameDataServer`'s
  unload and reload; a disk cache key names the game directory.

## 2026-09-30 - the editor crashed on a game directory change

* **Symptom:** in the editor, any change of the game directory (Browse, "Reload game data")
  crashed it with SIGSEGV, no frame of the library on the stack.
* **What proved it:** a headless editor with `demo_3d.tscn` open crashed on `data_reload()`; the
  log before it had `remove_child: Required object "rp_child" is null` from
  `MaszynaRailVehicle3D._free_parts()`, once per vehicle. In the editor `TrainSoundSystem` is a
  placeholder (not `@tool`): `MmdSoundBankInstancer._build_player()` stopped at its
  `register_bank()` with a script error and returned null, so every vehicle kept a null part.
  The first `_free_parts()` in the editor - now on every data unload - stopped at it, leaving
  parts removed or freed in `_parts`, and the rebuild after freed them again.
* **Fix:** the instancer builds no sound banks and no cab in the editor, as it already skipped
  `CabinSystem`.
* **Rule:** code that runs in the editor calls no autoload that is not `@tool` - the call is a
  script error that returns null into the caller's data, not a warning.

## 2026-09-30 - coupled vehicles lost their couplers

* **Symptom:** coupling SM42-329 with b16mnopux_50512608041-3 on Stary Jawor hid the hanging
  coupler on the locomotive without showing its connected E3D submodel; the wagon appeared not
  to change.
* **What proved it:** both real models contain `coupler1/2_off` and `coupler1/2_on`. The parser
  marks every `*_on` and `*_xon` as `dynamic_hidden`, as the original does for a vehicle. After
  `RailVehicleRenderingServer` hid `_off` and called
  `instance_set_submodel_visible(..., true)` for `_on`, `E3DInstanceBackend::_is_submodel_shown()`
  still rejected `_on` solely because it was `dynamic_hidden`. The original's
  `AirCoupler::TurnOn()` shows it (`AirCoupler.h:32`, `DynObj.cpp:819-850`). The pneumatic
  fallback also returned a straight variant when `GetPneumatic()` found no connected hose,
  unlike `TDynamicObject::SetPneumatic()` (`DynObj.cpp:497-545`).
* **Fix:** an explicit per-instance visible setting overrides `dynamic_hidden`, while a model's
  own `visible=false`, an invisible parent and an explicit hide still win. Coupler and pneumatic
  layouts use named enums, and a vehicle with no connected hose geometry keeps its hanging hose.
* **Rule:** `instance_set_submodel_visible(true)` overrides a dynamic model's default-hidden
  state; merely posing or changing the material of that submodel does not.

## 2026-09-30 - a recoupled wagon kept stale coupler rendering state

* **Symptom:** the consist in `td.scn` began with correct couplers. After uncoupling and coupling
  EP07-424 to the first wagon again, the locomotive's connected submodels returned but the
  wagon's did not.
* **What proved it:** the fixture passed for the first coupling, then failed as soon as the same
  vehicles were uncoupled: the initiating controller emitted `trainset_changed`, but
  `TMoverParameters::Dettach()` had already cleared both `Connected` pointers, so
  `_consume_coupler_events()` could no longer announce the former neighbour. Its renderer kept
  the old coupled `coupler_state`; the next coupling computed the same value and correctly
  skipped what appeared to be an unchanged state. The rendering callback's own comment promised
  to redraw the neighbours whose hoses depend on this vehicle, but its implementation redrew only
  the RID carried by the signal.
* **Fix:** `MoverRailVehicleController::uncouple()` retains the neighbouring controller before
  `Dettach()`, consumes the initiating end's events, then announces the trainset change to the
  former neighbour. The interactive disconnect operation goes through that same public
  operation. A coupling or trainset event redraws the directly connected neighbours too, so
  attaching an individual hose from one end updates both models. The regression now covers
  couple, uncouple and recouple.
* **Rule:** a bilateral operation whose backend severs the relationship retains both owners long
  enough to announce the resulting state change to both; a state cache is not invalidated as a
  substitute for the missing event.

## 2026-09-30 - the departure sound played on arrival

* **Symptom:** a station's departure sound ("odjazd") played straight after the train stopped,
  while the timetable panel still counted down to the departure correctly.
* **What proved it:** read off the code, not measured in the game: the scenery importer read
  `departuredelay <s>` and dropped it. The original (`event_manager::AddToQuery`,
  Event.cpp:2431-2444) adds to such an event's launch time the seconds until the departure of the
  queueing vehicle's train (`seconds_until_departure()`, mtable.cpp:184-190, from `StationStart`),
  plus the keyword's value. Without it the event runs after its own delay, which is the moment the
  train rolls onto the stop's track event.
* **Fix:** `ScenarioEventServer.event_set_departure_delay()`; `event_queue()` adds
  `DriverSystem.vehicle_get_seconds_until_departure()` of its activator vehicle (its own driver's
  timetable, else its trainset's), not before now.
* **Rule:** a scenery keyword read and dropped is a behaviour dropped - look for one in the
  importer first when an event fires at the wrong time.

## 2026-09-30 - the timetable's delay frozen on the way

* **Symptom:** the timetable panel showed a delay that did not change between stations and was
  off by the stop's dwell; the vehicle card did not count on while the train stood.
* **What proved it:** read off the code: the panel showed `-latency` on the way, and
  `LastStationLatency` (departure less arrival, mtable.cpp:122) is written only on arriving; the
  original shows no delay figure at all (driveruipanels.cpp:298-466). "At the platform" was a
  snapshot taken on a timetable change, while the route changes it every update without a signal.
* **Fix:** the driver's timetable records `delay` - at the arrival against the arrival time, once
  the train has driven clear of the station against its departure - and owns `arrived`; the
  panel counts on from the departure while the train stands; the card refreshes its timetable on
  its timer.
* **Rule:** a value the original keeps for the AI is not a figure for the player: check what it
  measures before showing it, and show only what the owner announces when it changes.

## 2026-09-30 - the EP07 orientation test braked by its driver

* **Symptom:** `test_zzz_ep07_orientation_regression` failed on CI ("vehicle should have
  actually started moving") and passed alone; after `test_zzz_ep07_main_switch_trip_diagnostic`
  it failed in about two runs of three.
* **What proved it:** a full state dump at the end of the drive, failing against passing: the
  failing run had `brake_local_position_normalized` 1.0 and the brake cylinder at 4.4 bar. A
  listener on `vehicle_command_received` caught `local_brake_set 1.0` during the parked frames,
  before the test took the vehicle over, sent by `MaszynaLegacyAIDriver._update()` ->
  `_apply_independent_brake_only()` - the driver holds a standing locomotive by its independent
  brake (Driver.cpp:8166-8180). Whether its scheduled update fell inside the parked frames
  depended on the timing left by the scripts before.
* **Fix:** the test releases the independent brake after taking the vehicle over, as a player
  does; four runs with the AI's brake applied each time all passed.
* **Rule:** a test that takes a scenery vehicle over sets every control it drives by, not only
  the ones a fresh vehicle has wrong.

## 2026-09-30 - the load exchange that never ran

* **Symptom:** the first `test_rail_vehicle_load_exchange` test called `load_add()` on the car's
  load component and waited for the doors and the exchange - nothing happened, the exchange time
  never moved; a later assertion read the old load straight after `load_add()`.
* **What proved it:** `MaszynaMoverVehicleServer::stepping_advance()` steps only the vehicles
  `RailVehicleServer` has on a track, and the car was built without one; the component the test
  held was the `MoverRailVehicleLoad` it had added to the controller description, while the
  running vehicle's component came from `VehicleServer.vehicle_component_get()`. The state dump
  (`vehicle_dump_state()`) is built once per step, so it still showed the load before the call.
* **Fix:** `MaszynaGutTest.build_passenger_car()` stands the car on a test track; the tests take
  the component by `vehicle_component_get()` and read its getters after an operation.
* **Rule:** a test of a component's tick needs a vehicle standing on a track and the vehicle's
  own component; after an operation read the getters, not the cached dump.

## 2026-09-30 - the sound system's dump per frame

* **Symptom:** in the editor profiler at x8 simulation speed `TrainSoundSystem._process` was the
  heaviest untyped `_process` (~836 against ~95 for the next one) while the train was moving.
* **What proved it:** a GDScript profile counts a native call in its caller's self time, and the
  sound's per-frame path had two: `VehicleServer.vehicle_dump_state()` per vehicle in earshot per
  frame, whose cache is keyed on the controller's state serial - moved by every step
  (`VehicleController::process_components()`) and every command, so on the frame path it is always
  a full `compose_state()` of every component (~28 per call in the cab's own `vehicle_state` row) -
  and `VehicleServer.vehicle_dump_config()`, not cached at all (`VehicleController::get_config()`),
  called by `RunningSoundModel.update()` per moving bank per update and by the engine gain per
  trigger tick - only when moving, hence the x8 drive.
* **Fix:** the sound takes its vehicle's components once, in `_resolve_vehicle()`, and reads their
  typed getters and properties (`RunningSoundModel.attach_vehicle()`,
  `MaszynaBrakeSfxEventFactory.state_reader()`); MMD triggers, which name their value in the data,
  read the dump on the trigger tick only. The AI driver, the player and the external camera moved
  off the dump the same way; `RailVehicleEngine.get_transmission_ratio()` was bound for it.
* **Rule:** a hot path reads a component, never a dump (`CODE_STYLE.md`); the dump is for readers
  driven by a name out of the data.

## 2026-09-30 - BR285's speed NaN, from a key the FIZ gives twice

* **Symptom:** a BR285 standing in a scenery had `speed`, `velocity`, `Ft`, `Im`, the brake
  forces, the wheel angles and the diesel temperatures all `nan`.
* **What proved it:** every NaN of the dump sits downstream of the diesel-electric branch of
  `TractionForce()` (`diesel_fill` and the temperatures are computed from `Im`, `Mover.cpp:4944`).
  The config ruled out the axles and the gear ratio, `engine_rpm_ratio=1.0` the WWList row. The
  FIZ's `Engine:` says `Vadd=5.5 Cr=1 Vadd=0.0 Cr=1.0`; the original's `extract_value()` finds the
  first (`utilities/utilities.h:170`), our `FizLineUtil.read_key_values()` kept the last, so
  `Vadd = 0`. With the line contactor closed, the vehicle standing and `tempPmax` still zero, the
  hyperbola gives `1000 * 0 / (0 + 0)` (`Mover.cpp:5310`) and the NaN stays in `V` for good.
* **Fix:** `read_key_values()` keeps a key's first value. Found on the way, from the original's
  `LoadFIZ_Engine`: a diesel-electric's `AIM` (default 1.25) and `RPMDecRate` were never read -
  and `dizel_RevolutionsDecreaseRate` had a second writer, `rpm_change_rate`, fed by nothing - and
  the cooling keys of both diesels (`Heat*`, `Water*`, `Oil*Temperature`, `Heater*`,
  `NominalCoolingPower`) were not imported at all.
* **Rule:** a FIZ key given twice counts once, with its first value - read how `extract_value`
  looks a key up before reading a line into a dictionary.

## 2026-09-30 - release export without CabinSystem

* **Symptom:** `make release-linux` printed `Failed to create an autoload, script
  'uid://lx8tmya3o3dj' is not compiling`, `Identifier not found: CabinSystem` (`player.gd:209`) and
  `!info->node` from `debug_menu/plugin.gd:27` while saving the pack.
* **What proved it:** the export is `godot-double --headless --export-release`, the editor, and an
  editor build carries the `debug` feature tag: it loads `linux.debug.x86_64` from
  `libmaszyna.gdextension` even for a release export. `release-linux` built only
  `libmaszyna.64.so`; the debug library predated `vehicle_set_cab_light_level()`, so
  `cabin_system.gd:197` failed to parse and the `CabinSystem` autoload was never created.
* **Fix:** `release-linux`, `release-linux-symbols`, `release-windows` and `release-android`
  depend on `compile-debug`.
* **Rule:** the library the exporting editor loads is the debug one - build it with every export.

## 2026-10-01 - the start offset that never reached the game, and the pitch every emitter shares

* **Symptom:** after the start-offset fixes (09-24, 10-01) trainsets still drifted in and out of
  phase from outside, "the phase still overlaps somewhere".
* **What proved it:** a probe built the stream the way a vehicle does
  (`MmdSoundEventBuilder._build_stream()`) on a real 2.61 s `.ogg` and played it with
  `start_fraction` 0.5: `MaszynaAudioStream.get_length()` was 0.0 and the voice started at 0.0.
  The stream reads its file on the first playback, after gnd-sfx has placed the start
  (`length * fraction`); the tests used `AudioStreamWAV`, whose length is known. No offset had ever
  reached a vehicle's sound in the game.
* **And the original's other half:** every `sound_source` draws its own pitch factor on its first
  play, 97.5-102.5 % unless the MMD's `pitchvariation:` says otherwise (sound.cpp:207-216, 374-377,
  applied per buffer at audiorenderer.cpp:206). Two copies of one recording then run at slightly
  different speeds, so copies that start close drift apart; with one pitch for all, a close pair
  stays in phase for as long as it plays. `startoffset:` was parsed and never used either.
* **Fix:** one shape for every sound made of MaSzyna's data. `MmdSoundEventBuilder.build_stream()`
  is the only maker of a `MaszynaAudioStream` and sets its length at build;
  `MmdSoundEventBuilder.shape_emitter()` gives every `SfxEvent` - vehicle banks, brake events, cab
  controls, scenery and scenario sounds, the guard's signal - its own `start_fraction`
  (`startoffset:`, or the bogie/motor rule of DynObj.cpp:6505-6514, 6085) and `pitch_variation`.
  gnd-sfx keeps both on the `SfxEvent` (the emitter) and applies the fraction to every clip but a
  `bookend` (begin/end), one-shots included, as audiorenderer_extra.h does; it steals a releasing
  voice first, then a one-shot, a loop last - a stolen loop is never started again.
* **First done only in TrainSoundSystem**, with the cab, scenery and one-shots left to TODO: the
  same original rule covered by one system and not the others, and two more makers of
  `MaszynaAudioStream` without a length. A rule ported from the original is one operation every
  caller goes through, not a copy in the system the bug report came from.
* **Rule:** a fix to a value is proven on the object the game uses (`MaszynaAudioStream`), not on
  a stand-in the test finds easier to build.

## 2026-10-01 - the local brake hiss keyed to a parameter nobody sent

* **Symptom:** the brake sounds did not sound like the original's, the local brake worst: too
  loud, no fade-out, no opening or closing bookend.
* **What proved it:** read against `Train.cpp:8474-8641` and `DynObj.cpp:4545-4760`.
  `local_brake_hiss` was played with `brake_local_valve_flow`, while its automations listened to
  `brake_loco_pressure_fall_rate`/`rise_rate`, so gnd-sfx saw their `min_domain` instead. Its
  curve had no `* 0.05`, was divided by a `maximum_gain` and bent by a cubic bias, the release
  condition `LocBrakePress > BrakePress - 0.05` was missing, a 0.6 s ADSR stood in for the
  original's 0.1/s fade, and only `soundmain:` was played. `unbrake`, `brakeacc` and the cylinder
  and EP clicks were never built at all, and `VOLUME_FACTOR` (2.0) multiplied the brakes alone.
* **Cause:** the original computes each sound's gain at its call site - filters, hysteresis, a
  hand-made fade, extra conditions - and the curve model mapped one parameter to one curve, which
  cannot say any of that. Every gap was filled by a guessed constant.
* **Fix:** `BrakeSoundModel` ports the call sites line by line and keeps their sound-only state;
  `MmdSoundBankInstancer` builds one plain event per label (bookends, or chunks on `point`), and
  the brake publishes only physical values (the FV4a handle flows, the accelerator event).
* **Rule:** port a sound whose original computes its gain at the call site as that code, with its
  own state, not as a curve over one parameter.

## 2026-10-01 - a phaser on the Exterior bus

* **Symptom:** from outside, a passing trainset (445w_v2 coaches) swept like a phaser, after the
  start offsets and pitch factors were fixed.
* **Cause (read off the bus layout, not measured):** the Exterior bus carried a reverb, a 60 ms
  slap-back delay and a StereoEnhance with `time_pullout_ms` 12 - a Haas delay of one channel. A
  source panning across the field during a pass-by changes the mix of the direct and the delayed
  copy: a moving comb filter on every exterior sound. The original (OpenAL) has none of them.
* **Fix:** all three removed; the bus keeps the wall low-pass, air absorption, amplify and limiter.
* **Rule:** no delay-based stage (reverb, echo, Haas widening) on the bus where many copies of one
  recording play - it is a comb filter of its own.

## 2026-10-01 - Alt+Enter loaded a scenery

* **Symptom:** none seen yet - found while adding Alt+Enter (fullscreen) to the game window: on the
  scenery selector the same keypress would also have run Enter and loaded the selected scenery.
* **What proved it:** a GUT probe built `InputEventKey` Enter with `alt_pressed`:
  `is_action_pressed("ui_text_submit")` true, `is_action_pressed("ui_text_submit", false, true)`
  false. The selector's `FocusSection`/`SelectorList`/`TileGrid` tested Godot's `ui_*` actions
  with the default loose match. A handler earlier in the tree cannot stop it either: a window's
  `window_input` signal comes before `push_input()`, which resets the handled flag
  (`window.cpp` `_window_input`, `viewport.cpp` `push_input`), and `_input` reaches an autoload
  last.
* **Fix:** the starter's keys are the project's `menu_*` actions (`demo/project.godot`), each
  matched with `exact_match` true; `toggle_fullscreen` is Alt+Enter.
* **Rule:** a key is the project's input action, matched exactly - never a keycode, never a
  built-in `ui_*` action, never a loose match (`AGENTS.md`, `CODE_STYLE.md` "Input is the
  project's actions, matched exactly").

## 2026-10-01 - a scenery's whole terrain and every model it ever showed, kept in memory

* **Symptom:** a large scenery took almost 10 GB of RAM while it parsed, and again while it loaded
  from the cache, and the OOM killer ended the game - though the streaming builds only what is
  near the camera.
* **What proved it:** reading what holds the data, not what builds it. The compiled scenery
  (`MaszynaCompiledScenery.triangle_chunks`) carried an `ArrayMesh` for every terrain chunk, so
  reading the cache loaded the whole terrain into RAM and VRAM at once, and the parse built every
  mesh at once from raw triangles it kept as well. Three memos never let go:
  `E3DRenderingServer.models` (every model ever streamed), `MaterialManager._dds_cache` (strong
  refs to every texture) and `ChunkState.mesh` (every chunk mesh until the scenery unloaded).
* **Fix:** `ResourceLazyLoader` holds a resource while something built uses it and lets it go
  after; the E3D models go through it. A terrain chunk's geometry is a cache file of its own
  (`MaszynaTrianglesChunkGeometry`), written on a worker as it is built and read only while the
  chunk is in range; its mesh is made as it is built and freed as it is cleared. The texture memo
  is weak, track materials are resolved as a track is built, and a scenery sound out of reach
  lets its file go.
* **Rule:** a cache in memory that never evicts grows with the session, not with what is in view.
  Whatever is loaded for a streamed piece is held by that piece's build and let go by its clear,
  and a cache file holds data, not render resources.
* **Since 2026-10-10:** this is lazy loading, an option (`maszyna/resources/lazy_loading`,
  `--enable-lazy-loading`), off by default at the operator's decision. Off, `ResourceLazyLoader`
  loads a resource when it is registered and keeps it until its last registration is freed - the
  memory this entry measured is spent again - and the streaming only builds and clears.

## 2026-10-01 - the parse kept every include's triangles in world space

* **Symptom:** after the terrain went to a file per chunk, Galicja (Linia 107 Objazdy) still took
  10.6 GB while it was parsed and went into swap; the memory stayed at the top once the parse was
  over, through the registration of the models.
* **What proved it:** following one `triangles` node through the parse. `MaszynaTrianglesImporter`
  put every node into world space and the context kept it as an entry until the end of the parse -
  `grass.inc`, included 24 000 times, as 24 000 full copies. The project is built in double
  precision, so a vertex with its normal and UV is 64 B, plus about 1 KB of Array, String and
  packed-array wrappers per node; the chunks were cut only after the parse, so for a while both were
  held. The cutting itself needs no more than one triangle: it clips to a fixed grid and appends to
  the chunk of its texture, cell and range.
* **Fix:** the triangles go to a `SceneryTrianglesSink` as each node is read - cut into chunks at
  once, kept as floats, and past 256 MB written to disk part by part; the parse ends by writing a
  file per chunk. The parsed scenery is packed and let go of, and built from that the same way as a
  cached one.
* **Rule:** a parse whose output is in world space and repeats per include must not keep it per
  include - reduce it to what it ends up as while parsing, and bound what is held in memory.

## 2026-10-01 - a task per include, and memory the allocator kept

* **Symptom:** with the terrain chunked as it was parsed, Galicja still reached 7.7 GB in the
  parse and held ~8.7 GB loaded, ~8.5 GB with the camera off the map - the streamed part was a
  margin, and nothing came back after the parse.
* **What proved it:** the loading screen counted 294 286 parsed files - every `include` with
  parameters (`grass.inc`, `tree.inc`) was a task of the queue with a whole
  `MaszynaImporterContext`, a `PendingInclude` and a bound Callable, kept until its parent finished
  its file, and its file read from disk each time. The resident size stayed up because glibc keeps
  what each worker's arena freed.
* **Fix:** includes under 16 KB are parsed in place and read once per parse
  (`SceneryInstancer.INLINE_INCLUDE_MAX_SIZE`); `ProcessMemory.release_unused()` (`malloc_trim`)
  after the parsed scenery is packed. Galicja loaded: 2.8-2.9 GB.
* **Rule:** a unit of parallel work costs a context; an object placed thousands of times is not a
  task. After a parse on many threads, give the allocator's free memory back, and measure the
  resident size, not Godot's own count.

## 2026-10-02 - a C++ stream crashed the release build

* **Symptom:** the exported game (`./reloaded`) loaded Galicja and died with SIGSEGV as the load
  ended; the debug library in the editor ran the same code without fault.
* **What proved it:** the core's main-thread stack (`coredumpctl -r info reloaded`): libstdc++'s
  `std::istream::_M_extract<long>` and a `codecvt` frame inside `libmaszyna.64.so`, called from the
  engine - `ProcessMemory::get_resident_bytes()` reading `/proc/self/statm` with `std::ifstream`,
  through `SceneryLoadMeasurement.print_memory()`. godot-cpp links libstdc++ statically
  (`GODOTCPP_USE_STATIC_CPP`), and its stream locale broke in the release library.
* **Fix:** the file is read with `FileAccess` and parsed with `String`.
* **Rule:** no C++ iostreams in the extension - files through `FileAccess`, numbers through
  `String`; a crash of the shipped build only is read off its core, never guessed.

## 2026-10-02 - a subscene kept its triangles in memory three times over

* **Symptom:** Całkowo took 6.5 GB while its files were parsed, the loading screen at `tree.inc`.
* **What proved it:** a synthetic scenery of 40 000 `tree.inc`/`grass.inc` includes in eight
  vegetation files, parsed headless: included with a parameter (in the scenery's sink) the peak
  (`VmHWM`) was +66 MB, included as subscenes (no parameters, >= 64 KB) +160 MB. A subscene had a
  sink of its own without a directory or a limit, copied it out whole (`get_geometries()`) and
  serialised the copy into its cache entry before handing it on - three copies, on every worker at
  once. The same includes in the scenery file itself kept nothing (+48 MB for 30 000, all freed).
* **Fix:** a subscene's sink writes to a directory beside its cache entry with a 32 MB limit
  (`SUBSCENE_TRIANGLES_BUDGET_BYTES`); its cache entry keeps the chunk file paths, and the
  scenery's sink takes them file by file (`SceneryTrianglesSink.add_geometry_file()`). Peak +80 MB.
* **Rule:** every sink of parsed geometry has a directory and a limit, a subscene's too; geometry
  moves between sinks as files, never as a copy of everything.

## 2026-10-02 - a queue task for every placed object over 16 KB

* **Symptom:** Całkowo parsed with a peak of 4.5 GB resident and 2.76 GB of Godot's own memory
  (`[SceneryLoad] FILES`), yet right after the parse Godot held 0.31 GB (`[SceneryConvert] parsed`) -
  over 2 GB alive only while the files were parsed; the loading screen counted ~29 000 files at half
  way.
* **What proved it:** a synthetic scenery placing a 20 KB object 30 000 times with parameters,
  parsed headless: peak (`VmHWM`) +466 MB, against +48 MB for the same number of small includes.
  Only includes under 16 KB were parsed in place; a larger one with parameters - an object placed
  again and again - became a queue task with a whole `MaszynaImporterContext`, a `PendingInclude`
  and a bound Callable, its result kept until the parent's file ended and merged it.
* **Fix:** an include with parameters is always parsed in place and its file read once per parse;
  only a parameterless part of the scenery (16 KB and more) is a task, from 64 KB a subscene.
  Peak +28 MB, the parse no slower (16.4 s against 17.0 s).
* **Rule:** parallel tasks are for parts of the scenery, never for placed objects - a task costs a
  context kept until its parent's merge, and a scenery places objects by the tens of thousands.

## 2026-10-02 - uppercase vehicle files were reported missing

* **Symptom:** loading `braniewo_szeroki.scn` reported that
  `dynamic/pkp/st44_v2/2m62-0571-a.fiz` and `.mmd` did not exist. The same happened for the B
  section and the 2M62-0662 consist.
* **What proved it:** the scenery names `2M62-0571-A` and `2M62-0571-B`, and files with exactly
  those uppercase names exist on disk. `MaszynaNodeDynamicImporter` lowercased the data folder,
  skin and vehicle filename before any lookup, so Linux never got the valid exact-case attempt.
* **Fix:** `MaszynaDataPath.resolve()` is the one databack-path operation: it keeps the base
  directory unchanged, tries the relative path as authored, then its lowercase form, then each part
  of it against its directory's entries letter case aside (821 names on disk carry capitals, e.g.
  `scripts/kilometry/EP07P-2003_przebieg.txt`). Every data loader uses it, and parsers retain the
  spelling of filename tokens.
* **Rule:** preserve the filename from the databack and try it exactly, then lowercase, then letter
  case aside; never lowercase only the token before the first lookup.

## 2026-10-02 - SN61 drawn without its body

* **Symptom:** SN61 in `stary_jawor_retro.scn` showed holes - only parts of the vehicle were drawn.
  The case-sensitivity fixes of the same day did not change it.
* **What proved it:** a probe calling `MaszynaRailVehicle3DInstancer.read_structure()` for
  `dynamic/pkp/sn61_v2` / `SN61_v2` printed `model=none`. `sn61_v2.mmd` is only
  `include sn61.mmd.inc (p2)`, and the include's `models: (p1).t3d#`; the top-level MMD was
  tokenized without parameters, so `(p2)` became the include's default `none`.
* **Fix:** the original parses a vehicle's MMD as `include <TypeName>.mmd <name> <TypeName> <skin>
  end` (`DynObj.cpp:5260`); every reader of it takes `MmdCabinInstancer.vehicle_parameters()`, and
  `attachments:` (`DynObj.cpp:5384`), which name their models by `(p1)`/`(p3)`, are drawn with the
  exterior. Structure cache v24, profile version 7.
* **Rule:** a vehicle's own MMD takes parameters like an include; a model name that comes out as
  `none` is a missing parameter, not missing data.

## 2026-10-02 - an empty terrain chunk

* **Symptom:** loading a scenery logged `Condition "array_len == 0" is true` from
  `mesh_create_surface_data_from_arrays()` in `MaszynaSceneryChunkRenderingServer._stream_build()`.
* **What proved it:** `to_mesh_arrays()` drops nothing, so the chunk's geometry itself was empty. A
  probe adding to a `SceneryTrianglesSink` one triangle of cell (0, 0) with an edge on the border
  x = 1000 got two chunks back: (0, 0) with 9 floats and (1, 0) with none. `add_triangles()` made
  the cell's piece before testing the area of its triangles, and a piece of only slivers became a
  chunk with nothing in it, written to the cache like any other.
* **Fix:** a piece without vertices makes no chunk; scenery cache version 34.
* **Rule:** a grid cut keeps a cell only when a piece with area lands in it.

## 2026-10-02 - Infrastructure hung with parallel preloads

* **Symptom:** Stary Jawor (and other sceneries without an SBT) hung for minutes on the loading
  screen's Infrastructure stage, on one model name, after the streaming's preloads were fanned out
  over the `WorkerThreadPool`. Headless loads and every test passed.
* **What proved it:** the full game under `xvfb-run` with `--rendering-driver opengl3` reproduced it;
  `kill -ABRT` and `coredumpctl info` (ptrace is blocked here) gave the stacks: the main thread in
  an engine wait under a GDScript call, the pool's threads in `SceneryStreamingServer::_preload` ->
  `E3DRenderingServer::_stream_preload` -> `model_load` -> `ResourceCache::set` ->
  `ResourceSaver.save`, waiting on a condition. Saving a mesh needs the main thread with a real
  renderer; the main thread's own loading needed the pool, whose threads were all ours.
* **The cause in the preload:** every model cache was cold after the cache versions were bumped, so
  each preload read the model from its source and saved it (`E3DModelManager.load_model`). Saving
  an `ArrayMesh` reads its surfaces back from the RenderingServer, and off the main thread with a
  real renderer that call waits for the main thread to flush its commands; the dummy renderer does
  not, so headless never waited.
* **Fix:** preloads run in batches on the streaming's own queue (`SceneryLoadingTaskQueue`, renamed
  `WorkerTaskQueue` the same day - the entries above keep the name they had), which the main thread
  never waits for; a model read off the main thread is saved into the cache by the main thread
  (`model_loaded_uncached`, connected deferred), so no preload waits for it at all.
* **Rule:** nothing on a worker reads back from the RenderingServer, and nothing that loads or saves
  runs on the `WorkerThreadPool`; a threading change is proven on a real renderer, not headless.

## 2026-10-02 - the Vehicles stage spent its time on sound banks nobody heard

* **Symptom:** on Galicja (`linia_107_poludnie.scn`, 129 vehicles, 104 types and skins) the
  loading screen's Vehicles stage was the longest: 6.6 s headless with every cache warm (22 s
  cold), one vehicle built per frame.
* **What proved it:** a headless probe of the real load, timing every step of
  `MaszynaRailVehicle3DInstancer.build_into()` on the vehicles the scenery placed: the build
  itself 4.4 s, of it the sound bank 2.4 s (the MMD parsed four times per vehicle 0.43 s,
  1905 `SfxEvent`s built 1.5 s - the length of each of their 1535 streams by loading the whole
  Ogg file - players and registration 0.45 s), the appearance 0.5-1.1 s (headless, nothing sent
  to the GPU), the physics 0.1 s; 1.6 s were the frames themselves, as every build exceeded
  `BUILD_BUDGET_MSEC` and a frame built one; drivers and scripts 0.5 s. Every vehicle's bank was
  built at load, though `TrainSoundSystem` culls a bank beyond `maszyna/sound/culling_distance`.
* **Fix:** a vehicle's bank is built only once it is within the culling distance of the listener
  (`TrainSoundSystem.vehicle_set_bank_builder()`, the nearest first, a budget a frame), and a
  stream's length is read off its Ogg pages (`AudioStreamManager.get_stream_length()`: the
  identification header's rate, the last page's granule) instead of loading the file. Vehicles
  6.6 s -> 4.1 s headless, the build 4.4 s -> 2.1 s.
* **Rule:** what a vehicle has only for being seen or heard is built when it comes within sight
  or earshot, not at load; a file's metadata is read off its header, not by loading the file.

## 2026-10-02 - streaming hitches: materials and textures loaded on the main thread

* **Symptom:** players report Braniewo dropping from 60 to 20 fps, and streaming "cutting" despite
  `SceneryStreamingServer::BUDGET_MSEC` (4 ms) and the preload threads.
* **What proved it:** a probe driving the camera at 25-30 m/s, `streaming_get_statistics()` each
  second (main-thread time and the longest single piece per owner) on a real GPU, and the
  material resolver timed per call, then `MaterialManager.get_material()` timed step by step:
  single pieces took 85-169 ms (a track), 163 ms (a model) - the budget is checked between pieces,
  so it cannot split one. A track's geometry is at most 2.8 ms with its textures loaded; the 163 ms
  model was 160.6 ms of one material: reading the disk-cached material (up to 80 ms - its `.res`
  carried the textures embedded) and then `MaszynaMaterialFactory.apply()` loading every texture
  again (10-83 ms). The preload threads load only geometry; materials and textures are resolved
  in the build, on the main thread. The game data here is on a spinning disk.
* **Also measured:** at Braniewo station (`braniewo_szeroki.scn`) a steady ~30 fps with ~5400
  draw calls, ~8400 objects and ~11 M triangles a frame, GPU ~32 ms - the steady drop is the
  rendering load; the streaming adds the hitches on top.
* **Fix (partial):** the material disk cache is gone - a material is always built from its `.mat`
  (`MaterialManager.get_material()`). Not measured yet. Open: textures loaded on the streaming's
  preload threads (`TODO.md`).
* **Trap on the way:** `xvfb-run` does not hide Godot under Wayland - it ignores `DISPLAY` and
  opens its window on the operator's desktop; Xvfb has no DRI3, so Vulkan cannot present there at
  all. `gamescope --backend headless -- godot-double ...` (with `WAYLAND_DISPLAY` unset) renders on
  the real GPU with no window.
* **Rule:** a streamed piece's main-thread build creates only what needs the main thread - every
  file it needs (model, material, texture) is read on the preload thread; a cache that is applied
  over again saves nothing. A real-renderer run goes through `gamescope --backend headless`.

## 2026-10-03 hundreds of vehicles

* **Symptom:** Wrzosy EIC (`wrzosy_eie2620.scn`, 852 vehicles: 553 parked, 291 road cars in
  one-vehicle trainsets on roads) ran at ~80 ms a frame with the GPU at ~22 ms, wherever the
  camera was; the Vehicles stage took 59 s with a warm cache.
* **What proved it:** a probe under `gamescope --backend headless` (real GPU, no window), the
  player in the cab, the simulation running, taking one system away every 10 s and reading the
  difference (debug build, so C++ costs read high): of 131 ms a frame, the cab logic 47 ms, the
  WeatherNode 24 ms, the road cars' physics 15 ms and their drivers 3 ms, the stepping of the
  other ~560 vehicles 22 ms, the GPU 17.5 ms.
* **The causes:**
  * The cab logic of every driven vehicle (322, the road cars among them) read
    `vehicle_dump_state()` on every simulation slice (`LegacyCabinCabLights`,
    `LegacyCabinMainSwitch`) - and the dump is invalidated by every step, so it was composed anew
    for each.
  * Every vehicle carried a `RainVolume`; `WeatherServer` walks all of them every frame.
  * Roads are not built, so the road cars could never be placed - yet each was a whole vehicle:
    stepped, with an AI driver and cab logic, and a third of the Vehicles stage.
  * Every vehicle took longer than `BUILD_BUDGET_MSEC` (8 ms), so each frame of the loading
    screen built one and paid the scenery's drawing on top.
* **Fix:** the cab logic reads typed getters (`CabinState.vehicle_component()`); one
  `RainVolume`, in the shown cab (`MaszynaDynamicTrainCabin`); a vehicle or trainset on a track
  that is not registered is left out at attach (`SceneryInstancer._attach_objects()`); the build
  budget is 33 ms. Wrzosy: 131 -> 62 ms a frame (debug build), Vehicles 59 -> 17.8 s.
* **Trap on the way:** a headless load of Wrzosy crashed in `E3DModelManager.load_model()` after
  "Attempting to initialize the wrong RID" from `servers/rendering/dummy/storage/mesh_storage.h`:
  the dummy renderer's RID owners are not thread-safe, and models are made on the preload threads
  and the main thread at once. Not a game defect - a real renderer's are - but a headless run of
  a large scenery is no proof of anything; use `gamescope --backend headless`.
* **Rule:** a per-step reader takes typed getters, never the dump; something every vehicle carries
  for the player alone (a rain volume, a cab) belongs to the player's vehicle; a scenery object
  that cannot be placed is not built.

## 2026-10-03 scenarios that did not start

* **Symptom:** signals stayed red past the departure: Wrzosy EIC (`wrzosy_eie2620.scn`, the
  player's exit signal `Sandomierz_N`), L053 poranek (shunter SM42-1096 shown its signal, not
  moving); "not always".
* **What proved it:** a probe (gamescope headless, `--audio-driver Dummy`, simulation x4-x20)
  logging every `event_launched` and, every 5 s, the AI driver's `driver_get_state()`, its route
  table and the vehicle's state. The events ran; the AI trains stood:
  * Wrzosy: the signal is opened by an AI freight passing `tor1451` (`tor1451:event1`,
    `eic2620.ctr:219-232`). Its ES64F4 stood with `engine_missing` = line breaker + converter: the
    pantographs never rose. The driver raised them through the cab's `pantfront_sw`, and a cab with
    a pantograph selector (`pantselect_sw`: ES64F4, E186) has none - the selector takes the valves
    over (Train.cpp:3154). The original's driver sets the valve directly (driverhints.cpp:269-313).
  * L053 poranek: SM42-1096 faces the end of its siding (route table: `LINE_END` 90 m ahead),
    its shunting signal (`szopa_tm11`) is behind it. `check_route_behind()`/`BackwardScan()`
    (Driver.cpp:8238, 5407) - turning back to a signal behind - was not ported.
  * Launchers (code review, then tests): the minute was `int((t - hour) * 60)`, which for a start
    time like 21:05 gives 4 (21.0833... * 60 = 1264.99999...), so a launcher on the start minute
    never fired (622 of 1440 start times); a timed launcher's condition was tried once and the
    launcher spent even when it failed; a timed launcher's radius was ignored.
* **Fix:** pantograph valves operated by the vehicle command (`MaszynaLegacyDriverHints.cue()`);
  `backward_scan()` and the turn back in the driver's update; launchers: `floor(t * 60)`, spent only
  once their condition passes, tried again when a memory changes and when the clock starts, a radius
  >= 0 within reach of the camera. Wrzosy: the freight leaves 09:24:30, `Sandomierz_N` proceeds
  09:29:50; L053 poranek: SM42-1096 turns back at 05:51 and shunts.
* **Rule:** a scenario that does not run is first a driver that does not drive - trace it; a step
  of the original's driver that sets the Mover is not ported through a cab control.

## 2026-10-03 the editor ran the scenario

* **Symptom:** a scenery opened in the editor (`demo/scenery_inspector.tscn`) flooded the log -
  `Invalid access to property or key 'sound_free'` on `ScenerySoundServer`, `occupied_cab` and
  `control_changed` on `CabinSystem`, `emit_signalp()` from `E3DModelManager` off the main thread,
  `Unicode parsing error ... after f3` - loaded twice, left its tracks and models behind on unload
  and took long in the Vehicles stage; another scenery (`zwierzyniec_osob.scn`) crashed the editor
  while its models streamed.
* **What proved it:**
  * The loader started the simulation: `SceneryInstancer` built AI drivers and ran the Lua
    scripts, the event factory queued the onstart events and made the scenery sounds, and the
    clock ticked wherever something held it. `CabinSystem` and `ScenerySoundServer` are not
    `@tool`, so in the editor every such call failed - and `_free_owned_rids()` built its list of
    `[rids, free]` pairs reading `ScenerySoundServer.sound_free` first, so the error left every RID
    unfreed.
  * The editor freed the first instance of the scene mid-load (it reopens a scene changed on
    disk); its `_exit_tree()` drained the parse queue, and the load went on reading
    `root.filename` on the freed node.
  * `E3DModelManager` emits `model_loaded_uncached` on a streaming preload worker: Godot's thread
    guard rejects the emit itself, so no model loaded off the main thread was ever cached.
  * The crashes: four backtraces, all faulting inside the Godot binary at the same two addresses -
    `godot+0x180932a` three times, each while an extension object was destroyed (`E3DModel` in
    `_drop_stale_work()` and at the end of `_plan()`), and `godot+0x18097a5` once, while an
    `E3DSubModel` was constructed on a preload worker - called from two adjacent functions. The
    core (`coredumpctl debug`, `thread apply all bt`) had no other thread in libmaszyna: the main
    thread was swapping buffers, the preload workers idle. Godot's source names them
    (`core/extension/gdextension.cpp:1086-1098`): `_track_instance()`/`_untrack_instance()` insert
    into and erase from `HashSet<ObjectID> instances` with no lock, for every object of a
    **reloadable** extension, in the editor only (`main.cpp:2224`, `object.cpp:190`). The
    extension said `reloadable = true`, and the streaming creates and frees models on its workers.
    Three guesses read off our own code came first and were wrong (the cache save, the
    destructor's `clear()`, the reload without a drain); what settled it was laying the
    backtraces side by side and looking at the other threads of the core.
  * `node.cpp:1441 p_name.is_empty()`: the scenery's nodes were packed under a `Node3D.new()` that
    never had a name (`_pack_objects()`), so every `instantiate()` of the packed scene tried to
    set an empty one. The first guess (an unnamed E3D submodel) was wrong; the GDScript backtrace
    of a later run named the line.
  * The vehicles' models were created in `vehicle_set_appearance()` at the origin with
    `detailed = true`: every vehicle built its exterior and interior as nodes.
  * The FIZ line reader decoded cp1250 as UTF-8 (`dynamic/pkp/11xa_v2/111a_old.fiz`, "wagonów").
* **Fix:** `SimulationRuntime` (the clock runs only with one in the tree; the game scenes and the
  GUT pre-run hook place it), `MaszynaLegacyScenario` started by the game's world on
  `scenery_loaded` and stopped on `MaszynaIncludeNode.unloading` (scripts, `MaszynaLegacyScenerySounds`);
  the load ends when the drained queue returns nothing; vehicles born optimized, detailed on
  placement by distance; `Windows1250.decode()`/`encode()` in the FIZ reader; `_clear_content()`
  drains the streaming before it frees anything, as `_exit_tree()` does; `E3DModelManager` saves
  by `_save_model.call_deferred()`; `reloadable = false` in `libmaszyna.gdextension`; the packed
  root is named.
* **Rule:** a loader builds, the game runs. A crash at changing places is one cause: lay the
  backtraces side by side and find the frame they share, in the engine's binary too, before
  reading our code. An extension with worker threads is not reloadable.


## 2026-10-03 a scenery's vehicles drawn on every editor tab

* **Symptom:** with `demo/scenery_inspector.tscn` loaded in the editor, its trains stayed in the
  viewport of every other scene tab; back on its own tab the scenery was gone - only the vehicles
  were there, and flying over it streamed no terrain chunks.
* **What proved it:** the editor removes the edited scene from the tree when another tab is
  shown, and every edited scene renders into the same `World3D`. `RailVehicleRenderingServer`
  built a vehicle's models in the node's scenario and freed them only with the vehicle.
  `MaszynaIncludeNode._exit_tree()` freed every RID of the scenery (tracks, traction, models,
  chunks, the region files' providers) while its nodes stayed, so nothing was there to draw or
  stream on return. A first fix freed the vehicle's models on leaving the tree and built them
  again on entering: `E3DRenderingServer.cpp:1371 Parameter "instance" is null` - the couplers
  and the rest of the server still held the freed model's RID.
* **Fix:** out of the world, moved out of it: `RailVehicle3D` on `NOTIFICATION_ENTER_WORLD`/
  `EXIT_WORLD` calls `RailVehicleRenderingServer.vehicle_set_scenario()`;
  `MaszynaIncludeNode` sets the scenario of its tracks, traction, models, chunks
  (`chunk_set_scenario()`) and providers (`SceneryStreamingServer.provider_set_scenario()`,
  which lets go of what was supplied and supplies it again into the new scenario) and frees
  them on `NOTIFICATION_PREDELETE`. `E3DRenderingServer.instance_set_scenario()` moves the
  RenderingServer instances of a built model (submodels, lights, smoke) instead of building it
  again.
* **Rule:** whatever is made in a world's scenario leaves it with its node and comes back with
  it, the RIDs unchanged, as Godot's own nodes do; it is freed with the node. The editor's tabs
  make "left the tree" an everyday event, not a teardown.

## 2026-10-03 hand-assembled vehicles stopped animating

* **Symptom:** `test_rail_vehicle_track_movement.gd` - "bogies should follow different tangents on
  a curved track", the powered wheel at its rest angle - on a vehicle assembled by hand
  (`RailVehicle3D` with an `E3DModelInstance`).
* **What proved it:** the test had passed before the same day's commit that made
  `RailVehicleRenderingServer`'s vehicles "born optimized" (`Visual::detailed = false`), and was
  not among the scripts run for that commit. The running gear is posed only while a vehicle is
  `detailed`, and `_update_detail()` flips the flag only with a streaming camera - a scene or a
  test without one never got there. The default had been changed for the vehicles this server
  builds models for; the models handed over by their owner are nodes from the start.
* **Fix:** `vehicle_set_models()` sets `detailed` - a model handed over is its owner's nodes
  already; only the server's own models are born optimized.
* **Rule:** a default changed for one owner of a field is checked against every other writer of
  it, and the tests of all of them are run - not only those of the case in hand.

## 2026-10-03 a test's vehicle was built twice, from another game directory

* **Symptom:** a test that set the fixtures' game directory, spawned a `MaszynaRailVehicle3D` and
  set the directory back at once logged `Cannot open FIZ file: <the real game dir>/...` and found
  no vehicle.
* **What proved it:** the path in the error was the restored directory's. `UserSettings` saving the
  game directory makes `GameDataServer` reload the game's data, and a vehicle built from that data
  frees itself and is built anew from the directory current then.
* **Fix:** the test frees its vehicle before it restores the directory (as the tests that restore
  it in `after_each()` do).
* **Rule:** the game directory is changed only while nothing built from it is alive.

## 2026-10-03 the loading screen faded onto a world not streamed yet

* **Symptom:** after a scenery loaded, the loading screen dissolved onto the world before the
  chunk around the cab was built (operator's report, in the game).
* **What proved it:** read off the order of the calls. `demo_scenery_loading.gd` awaits
  `world.load_scenery()`, then waits a bounded number of frames for the player's vehicle
  (`_wait_for_cabin()`), then for `area_is_ready(0)` around the current camera. Since the scenario
  is started by the game, `world.gd` started it in its `scenery_loaded` handler and relayed the
  signal only after that start (the scenery's sounds are built over frames) - while
  `load_scenery()` had long returned. The player got its vehicle after the bounded wait had run
  out, the camera was still where the menu left it, and that empty area was "ready" at once.
* **Fix:** `SceneryWorld.load_scenery()` returns only once the world has emitted `scenery_loaded`
  (its scenario runs, the player has been given its train). `test_zzz_scenery_scene_smoke.gd`
  checks that the streaming camera is at the player's vehicle and its chunk built when the
  loading screen goes.
* **Rule:** an operation somebody awaits is done only when everything its waiter goes on to rely
  on is; a signal relayed after an `await` arrives after the call that caused it has returned.

## 2026-10-03 a scenery's vehicles drawn at the origin until their trainset stood

* **Symptom:** in the editor and in the game, a loading scenery showed its vehicles piled at
  (0, 0, 0), then moved to their tracks (operator's report, twice).
* **What proved it:** the order of the calls. A vehicle's models were created when its appearance
  was set, in the world of its scene node and at that node's transform - the scenery's root, the
  origin - and a trainset stands its vehicles only after every one of them is built.
* **Fix:** `RailVehicleRenderingServer` gives a vehicle's models their world only once the
  vehicle has a place: `RailVehicleServer` placed it on a track, or `vehicle_set_transform()`
  told where a vehicle on no track stands (`RailVehicle3D`, at its node).
* **Rule:** what is drawn before it has a place is drawn in the wrong one - a thing enters the
  world with its first position, not with its construction.


## 2026-10-03 switch ballast at the origin

* **Symptom:** in Drawinowo, pieces of track infrastructure lay around the scenery's (0, 0, 0),
  turned each its own way (operator's screenshot).
* **What proved it:** nothing in Drawinowo's files is placed near the origin. The switches'
  includes (`2-5-13/l34000_r.inc`, `rainsted/r-eea5_l.inc`) open `origin (p2) (p3) (p4)` and,
  inside it, `origin 0 -0.2 0` for the ballast; the original sums the offsets
  (`simulationstateserializer.cpp:651`), `MaszynaImporterContext.push_origin()` replaced the outer
  one - the ballast kept the switch's rotation and lost its position.
* **Fix:** `push_origin()` adds the offset to the current origin; the scenery cache format
  version bumped.
* **Rule:** a scenery's `origin` is a stack of sums, not of values - an `origin` inside an
  `origin` adds to it.

## 2026-10-03 Sandomierz without its platform

* **Symptom:** Wrzosy (`wrzosy_eie2620.scn`), Sandomierz station: the shelters stand on grass, no
  platform under them, and no platform among the models of "Edit SCN"'s sector (operator).
* **What proved it:** the shelter at (-18453.5, 0.5, 52440) is `wiata_san1`
  (`linia053/scenariusz_os/l053_pozostale_dwschod.scm:748`); the platform beside it is one model,
  `node -1 0 none model ... models\linia053\peron_sandomierz.t3d` (`l053_tri.scm:59325`).
  `maszyna_node_model_importer.gd` put `models/` in front of every path but `dynamic`, so it looked
  for `models/models/linia053/`. The original tries the path as given first, then `models/` + the
  path (`TModelsManager::find_on_disk()`, `MdlMngr.cpp:146-150`). On the way: Wrzosy's own
  platforms (`linia053_wrzosy/scm_wrzosy/6-teren.scm`, `triangles`) parse and reach the cache
  whole, and no Wrzosy scenario has a region file.
* **Fix:** the importer looks for `.e3d` then `.t3d`, as given then under `models/`; the scenery
  cache format version bumped.
* **Rule:** a file named by the data is looked up where the original looks it up, in its order -
  not under the one root most of the data uses.

## 2026-10-03 l107's factory turned by 80 degrees

* **Symptom:** `linia_107_objazdy.scn`, sector -65,-20: the Agromet factory stands about 90 degrees
  off (the scenery's author, through the operator).
* **What proved it:** the node is `model -64789.1 302.559 -19760.6 0 przemysl/fabryka_agromet.t3d
  none lights 4.5 angles 0 80 0 endmodel` (`l107/deko/107_deko.scm:4035`, no `rotate` around it);
  the model's own matrices are identity (`fabryka_agromet.e3d`, read off the file). The importer
  read every token after `lights` up to `endmodel` as a light mode, so `angles 0 80 0` became the
  modes 0, 0, 80, 0 and the factory kept the node's angle 0. The original ends a `lights` or
  `lightcolors` list at any of its keywords (`TAnimModel::is_keyword()`, `AnimModel.cpp:268-277`)
  and reads that keyword next. 1654 nodes of the game data have `angles` after `lights`, 191 of
  them in `l107/`.
* **Fix:** `maszyna_node_model_importer.gd` ends the lists at the original's keywords; the scenery
  cache format version bumped.
* **Rule:** a list in the data ends where the original's loader ends it - at its keywords, not at
  the end of the node.

## 2026-10-04 a wagon's dump with a locomotive's state

* **Symptom:** a problem report's snapshot (`stary_jawor_eszelon.scn`) dumped a passenger car,
  `b16mnopux` (`pkp/bdhpumn_v3/bdhpumn.fiz`, sections `Param. Load: Dimensions: Wheels: Brake:
  Doors: BuffCoupl. Cntrl. Light: Clima:`), with `controller_main_position`,
  `master_controller_position`, `current0..2`, `relay_*`, `converter_*`, `circuit_rlist_size`, the
  horns and the radio (operator).
* **What proved it:** two sources. `RailVehicleController::_fill_state_dictionary()` wrote the
  master controller's, the engine's, the low voltage's and the radio's state for every vehicle
  from the controller's own getters; and the FIZ factory made a `MoverRailVehicleMasterController`
  on every `Cntrl.` line (`fiz_train_cntrl_parser.gd`, "every vehicle's") and attached
  `MoverRailVehicleHorns`/`MoverRailVehicleRadio` to every vehicle (`fiz_vehicle_builder.gd`). A
  wagon's `Cntrl.` carries its brake keys, and 239 wagons write `MCPN=1` there (counted over the
  data's `Category=train` files, includes resolved).
* **Fix:** the state moved to its owners - the master controller (positions, the active cab, the
  Hasler recorder, the distance counter), the engine (relays, `RlistSize`, the ammeters), the
  radio (Radio-Stop) and a new `RailVehiclePowerSupply` (battery, converter, 24 V / 110 V, made
  from `Light:LMaxVoltage` or `Cntrl.`'s battery and converter keys); the controller keeps the
  direction, the occupied cab, the damage, `Mred` and the coupler stretch. The factory makes a
  master controller only for `MCPN > 1`, the horns and the radio only with it. The cab's
  ammeters read the powered vehicle, as TTrain's `mvControlled` (`Train.cpp:8638, 10856`).
* **Rule:** a component exists only when the FIZ describes it, and the controller fills only its
  own state.

## 2026-10-04 SM42's windows missing

* **Symptom:** vehicles had holes for windows; SM42 and ST45 in `stary_jawor_osobowy1.scn` kept
  them after the first fix (operator).
* **What proved it:** printed off the data and rendered with the real renderer (headless `sway`).
  The original draws a submodel in exactly one pass: flag 0x10 in the opaque one, alpha-tested at
  the material's `opacity:` (0.5 without one, `opengl33renderer.cpp:2247-2258, 3422`); 0x20 in the
  alpha one (`:4313`); a replaceable skin with Opacity < 1 - bits 1, 2, 4, 8, not 0x20
  (`Model3d.cpp:421-441`) - in the alpha one when the skin has alpha (`DynObj.cpp:331-346`). The
  wrapper cut every vehicle submodel out, and `E3DModelBuilder` read 0x20 only: SM42's and EU07's
  glass is the skin-painted `szyby` of the low-poly interior. `szyby` hangs under `cab1`, and
  `CabinSystem` hid the occupied low-poly cab (`vehicle_set_cab()`) while the player's cab stood -
  from outside too, where the original shows it (`DynObj.cpp:1389-1397`).
* **Fix:** `E3DSubModel.skin_translucent`; a vehicle's models force alpha
  (`RailVehicleRenderingServer::_create_models()`), and a forced submodel is blended by the nodes
  backend and opaque in the optimized one (`E3DRenderingServer.Translucency`,
  `MaterialOptions.force_opaque`); a material with `opacity:` is cut out at it (EP09's skin: 0.92,
  glass at 0.79). A blended surface mirrors the sky by the material's reflectivity and takes the
  lights' diffuse part only (`types/blended_reflection.gdshaderinc`): with the lights' highlights
  a pane as smooth as glass burnt out white (SU46). The player hides the cab's interior in a view
  from outside and tells the drawing (`vehicle_set_visible_low_poly_cabins()`); `CabinSystem` no
  longer does.
* **What went wrong on the way:** a per-texel split of one submodel into an opaque and a blended
  pass, distance logic in the shaders, a darkening setting and a rewrite of the shader variants -
  none of it asked for, each breaking something (ST44's glass opaque, torn frames). All removed.
* **Rule:** a submodel is drawn in one pass, as the original's flags say; the optimized instancer
  never uses the alpha pass; what is drawn for a view is told to the drawing by the owner of the
  view, never by the cab layer.

## 2026-10-04 settings saved their minimums

* **Symptom:** opening the new settings screen once changed the game: draw distance 100 m, smoke
  at its least, the external view stuck close (operator: "dziwne wartości, pojebane min/max").
* **What proved it:** `user://settings.cfg` held a `[project_settings]` section with every slider
  at its minimum (`draw_distance=100.0`, `lights/distance=10.0`, ...). A headless probe printed the
  slider at its minimum before any step: building the row set `min_value` while the value was 0,
  `Range` clamped it and emitted `value_changed`, and the row's handler saved it.
* **Fix:** the controls only hold what the player chose; "Apply" saves, "Cancel" forgets, and a row
  saves only when its control differs from what it showed after the stored value was put on it -
  the control rounds and clamps (0.564628 at step 0.01), so comparing with the stored value saved
  untouched settings too.
* **Rule:** a control is only shown a value; saving is an explicit operation, of what differs from
  what was shown.

## 2026-10-04 CI red on a setting the server took a frame late

* **Symptom:** every CI run on `dev/integrate-track-rendering` failed the same three tests:
  `test_simulation_clock` (the running speed already at x2 after one short frame),
  `test_maszyna_environment_node` (an hour at x100 ended at 0.492 h, not 0.5) and
  `test_scenario_script_server` (`LuaTestTrain: Unknown command: battery`).
* **What proved it:** all three reproduced locally, one script at a time. Since `e4ce54b4`
  `SimulationServer` takes `maszyna/simulation/speed_change_time` on ProjectSettings'
  `settings_changed`, which Godot emits deferred; the tests set the setting and advanced the
  clock in the same frame, so the old value (0.0, or the default 0.4) ran. `76136f44` moved
  `battery` to `RailVehiclePowerSupply`, which a bare test vehicle no longer has.
* **Fix:** a test that changes the setting awaits `ProjectSettings.settings_changed` before it
  advances the clock; the Lua test's vehicle gets `build_power_supply()`.
* **Rule:** a value a server takes on `settings_changed` lands only after that signal; whoever
  sets a Project Setting and reads its effect awaits it. A change that moves a command to a
  component greps the tests for the command.

## 2026-10-04 34WE's body missing at 80 m

* **Symptom:** at one distance from the camera a 34WE (Elf) car had no side walls - its interior
  stood in the open while the nose, the roof and the cars around it were whole (operator's
  screenshot, taken after the window glass commit, so it looked like the glass).
* **What proved it:** a headless dump of `34we-b.e3d` and the skins: the body (`pudlo3`) is opaque,
  the skin's alpha is 255 at every mip level - not the glass. The body has LODs `pudlo3` 0-80 m and
  `pudlo3_lod1` 80-200 m whose meshes are centred 2 m apart along the car (z 0.17 against 2.17);
  the nose's LODs share their centre. Godot measures a visibility range to the centre of the
  instance's box (`renderer_scene_cull.cpp:1471`), the original one distance from the vehicle's
  origin for every submodel (`opengl33renderer.cpp:3388`, `3654`) - around 80 m one LOD had left
  and the other not come yet (or, from the other end, both were drawn).
* **Fix:** both E3D backends give every mesh submodel a custom box: its mesh's, widened to be
  centred on the model's origin (`E3DInstanceBackend::_visibility_aabb()`). `MeshInstance3D.get_aabb()`
  still returns the mesh's own box, so picking and cab code are unchanged.
* **Rule:** a distance the original takes once per model is measured from the model's origin in
  Godot too; a per-instance box centre is not the same point.

## 2026-10-04 EU07, ED78 and 36WE reported unstartable, no test guarding the start

* **Symptom:** a tester (build with b5e744f1) could not start an EU07, nor the ED78 and 36WE
  units: "an interlock does not work after the battery is switched on".
* **What proved it (so far):** headless probes from the scratchpad, A/B against b5e744f1^ built
  in a copy (`git archive`, own `build-debug`). 4E, EP07, 36WE and ED78 built from their FIZ get
  `MoverRailVehiclePowerSupply`; battery -> `power24_available`, pantographs, relay reset, main
  switch, converter work on HEAD and on the parent alike - through commands, through the cab logic
  of `4e.mmd` and through the FIZ cache. On l053_poludnie.scn EU06-17, taken over by the player,
  starts through its cab and runs (122 A, 32 km/h at position 6) - identical on both builds. The
  state the player takes over: battery on, pantographs up, 3400 V, **ground relay not reset**, main
  switch open (the original's ReadyFlag leaves the main switch to the driver, Mover.cpp:8915-8921):
  the main switch closes only after the reverser and the relay reset (`fuse_bt`). Not reproduced
  yet: an ED78/36WE trainset (a motor car started from a control car's cab), the keyboard path.
  What did show: no test asserts the low voltage or the start sequence - `test_train_battery.gd`
  checks only `battery_enabled` (the flag `BatterySwitch()` sets), and b5e744f1 removed
  `battery_voltage = 110` from five tests without a replacement while they stayed green.
* **Probe traps on the way:** a vehicle not on a track is not stepped; without a
  `SimulationRuntime` the clock stands at 0; a standing vehicle nobody drives switches its physics
  off; headless frames are microseconds (wait in time); a `-s` script does not compile against
  autoloads (run a scene); a hand-fed wire voltage is zeroed by the step before the cab's tick.
* **Fix:** not found yet - waiting for the tester's log and the exact step. Rules written into
  `.claude/skills/testing/SKILL.md` and `mover-parity-check`.
* **Rule:** a change to a physics component is tested through the whole start sequence; a test it
  turns red is a suspected regression, never fixed by removing its setup or assertion; physics
  tests assert only on components' getters and the state dump.

## 2026-10-04 Test audit - what the suite does not guard and why

* **Symptom:** after the tester's report (the entry above) the suite was green on CI (83a941e3:
  156 scripts, 925 tests), and nothing in it would have turned red had the low voltage gone.
* **What proved it:** a read of all 157 scripts against the start sequence; CI logs of
  2026-10-04; a GUT run on a scratch directory with a script that does not parse. Causes, each
  with its offenders listed in `TODO.md` "Tests":
  * the test hands in what it is about - `build_power_supply(110.0)`, `_feed_wire()`, vehicles put
    together with `MoverRailVehicleController.new()` + `add_component()` (46 scripts) - so the way
    the game builds a vehicle (FIZ factory, `apply_configuration` order) never runs in it;
  * it asserts what it set or that a key exists - 97 setter/getter round trips in 25 scripts,
    `has("key")` in 20, `assert_true(true)` in a benchmark;
  * physics without simulated time - 43 of 60 physics scripts read state right after a command or
    after `wait_idle_frames(2)`; a headless frame has no FPS cap, "300 frames = 5 s" is not;
  * the assertion stops before what the player sees - the AI's `Prepare_engine` is checked up to
    `battery_enabled`; nothing asserted `power24_available`, `converter_enabled`,
    `power110_available`;
  * fixtures build broken vehicles unnoticed - `test_vehicle.fiz`, `test_wagon.fiz`,
    `synthetic.fiz` get a 0 V battery; the only real electric vehicle is the EP07;
  * a test broken by a change got setup instead of suspicion (60ae3923), and four
    `test_driver_system` tests lost their battery without changing their result;
  * private members (113 uses in 17 scripts) and hand-written state dictionaries tie tests to the
    implementation;
  * GUT skips a script that does not parse with only a warning ("does not extend GutTest") and
    does not count it - its tests vanish from a green run; CI runs on pull requests and tags,
    not on a push to `main`; 18 red runs in a row on 10-04 made red the norm.
* **Fix:** the start sequence asserted to its end (`TODO.md` "Tests"); the rest listed there.
* **Rule:** a test cannot guard what it hands in itself; it asserts the end of the sequence the
  player sees, after simulated time.

## 2026-10-04 segfault in the streaming's preload

* **Symptom:** the Linux release build 20261004-1932 crashed with SIGSEGV on Wrzosy, with a 36WE;
  its log was already rotated away.
* **What proved it:** `coredumpctl info` of the crash, the stripped `libmaszyna.64.so` relinked
  from the same objects without `-s` in the SDK container (its `.text` identical to the shipped
  one) and the frames resolved with `addr2line`. A `WorkerTaskQueue` thread:
  `E3DRenderingServer::_stream_preload` -> `ResourceLazyLoader::resource_load` -> `_get_loaded`
  -> `Object::is_class`; the main thread at the same moment:
  `SceneryStreamingServer::_drop_stale_work` -> `PendingBuild` freed -> `E3DModel::~E3DModel`.
  `_get_loaded` took a loaded but not held model back from its `ObjectID`
  (`ObjectDB::get_instance` + `cast_to`) to share it with a second preload; the main thread was
  dropping that model's last reference, and an object being destroyed is still in `ObjectDB`
  until `~Object` - the worker called into a half-destroyed object.
* **Fix:** the `ObjectID` is gone; a copy not held is not shared. `resource_fetch(rid)` became
  `resource_hold(rid, loaded)`: the build holds the copy its preload loaded, or the one already
  held. Two preloads of one model before it is built load it twice (`TODO.md`).
* **Rule:** never take an object back from its `ObjectID` on a thread other than the one that may
  drop its last reference; share a copy across threads only through a `Ref` its owner keeps.

## 2026-10-04 Keys of modelled controls needed the cab model

* **Symptom:** a start-up test by the player's keys on the EP07 fixture stopped at the battery:
  `battery_toggle` did nothing. The tester's 36WEa-014A took no direction, no master controller
  step from the keyboard.
* **What proved it:** `test_zzz_startup_ep07` red at "battery: low voltage" with no 3D cab built.
  A control the cab's MMD models (`battery_sw`, `cabactivation_sw`, `main_on_bt`, `jointctrl`,
  `brakectrl`) took its key only in its widget (`CabinButton/CabinSwitch._input`, polling in
  `CabinKnob`); `LegacyCabinUnmodelledControls` skipped every modelled control. No widget - the
  fixture has no models, a game install whose cab model fails to load - no key.
* **Fix:** the keys of every control - modelled, unmodelled, keyboard-only - are mapped by
  `LegacyCabinLogic` from the MMD and the catalog (`MmdSemanticCatalog.resolve_fields()`, also used
  by the widget build) and go through its `press/release/increase/decrease`, which a click on a
  widget calls too. Widgets lost `_input`, key polling and `CabinCommand`.
* **Rule:** a key is the cab logic's, never a widget's.

## 2026-10-04 Every car sat a driver - the brake pipe of a two-unit EMU stuck at 3.3 bar

* **Symptom:** a cold 36WEa-014+015 (wrzosy_roj66582) charged its brake pipe to about 3.3 bar
  and no further, the handle in running and 5.3 bar in the main reservoir; the original charged it
  to 4.98 (operator, td_36wea.scn).
* **What proved it:** a probe of all six cars - every one had `cabin_occupied=1`; the inactive
  cabs' MHZ_K5P handles stood at bh_NP (1, "odcięcie"), which still regulates the pipe to its CP
  (hamulce.cpp TMHZ_K5P::GetPF), and CP was the cold start's LowPipePress (Mover.cpp:12029).
  Moving those handles to running filled the pipe to 4.98. `BrakeOpModes=PN` is bom_PS + bom_PN
  (Mover.cpp:10757), so the original lets only a car whose cab is occupied work its handle
  (`CabOccupied != 0`, Mover.cpp:4548) - and the wrapper overrode every unmanned car's cab 0 with
  1 (MoverRailVehicleController, a known FIXME).
* **Fix:** the cab a driver sits in is the scenery's driver type's (DynObj.cpp:1948-1964), 0 for
  nobody; the override is gone. Tests that drive a hand-built vehicle's cab give it a driver
  (`build_vehicle(..., VehicleController.DRIVER_HEAD)`).
* **Rule:** a car nobody sits in has no occupied cab - its own brake valve and controllers do
  nothing, it is driven over the couplers.

## 2026-10-05 Cars without an engine had no pantographs, and a battery locomotive read the collector

* **Symptom:** the ED78 (31WE) start-up stopped at the pantographs: its B and C cars carry them
  (`Power: EnginePower=CurrentCollector CollectorsNo=1`) but have no `Engine:`. EL16's dump showed a
  pantograph tank pressure of 5e-14 and its start-up waited on a pantograph compressor.
* **What proved it:** the collector lived inside `RailVehicleElectricEngine`, so a FIZ without
  `Engine:` built none - the B car had no pantograph commands at all. `TPowerParameters` keeps the
  collector in a union with the accumulator, the generator and the rest (MOVER.h:599): EL16
  (`EnginePower=Accumulator`) read its accumulator's bytes as collector parameters. The original
  zeroes the collector before reading it (`TCurrentCollector{0, ...}`, Mover.cpp:11547); the
  wrapper did not, so `FakePower` (never written) kept whatever the memory held.
* **Fix:** `RailVehicleEnginePowerSource` (`MoverRailVehicleEnginePowerSource`) is a component of
  its own, built from `Power:` whether there is an engine or not (`FizTrainPowerParser`); it zeroes
  the collector, reads `FakePower`, the defaults of `LoadFIZ_PowerParamsDecode` and `Power == 0`
  making the source NotDefined (`LoadFIZ_Power`), and returns its collector values only for a
  current collector. The DebugWindow "Power source" shows the pantograph carrier.
* **Rule:** a value in a Mover union is read only under the tag that says it is there.

## 2026-10-05 A series motor's MotorParamTable: was read as a diesel-electric's

* **Symptom:** EP03 and EL16 had NaN tractive force, speed and engine revolutions from the moment
  they had a direction; the line contactors never closed.
* **What proved it:** both FIZ have `MotorParamTable:` (no `0`) with five columns - `idx mfi mIsat
  fi Isat`. The builder sent that section to the diesel-electric parser, which wants seven and
  drops shorter rows, and which stores nothing on a series motor anyway: the series engine had an
  empty motor table. The original reads the section by the engine type (`readMPT`,
  Mover.cpp:9120; `readMPTElectricSeries`, Mover.cpp:9147).
* **Fix:** `FizTrainEngineParser` picks the table's parser by `context.engine_type`; the series
  parser reads the five-column rows.
* **Rule:** a FIZ table's layout is the original reader's for that engine type, never the
  section's name alone.

## 2026-10-05 A compressor without CompressorPower= ran off the main circuit

* **Symptom:** EN57KM's main reservoir stayed at 3.2 bar: the compressors of its control cars
  (`CompressorStart=Automatic`, 110 V present) never ran.
* **What proved it:** the control cars' FIZ have no `CompressorPower=`; the original takes 1 -
  converter-fed, switched by hand (Mover.cpp:10509-10519). The wrapper's default was MAIN (0),
  which runs only with the car's own line breaker closed (`Mains`, Mover.cpp:4383) - a control car
  has none. The value 1 was even named `COMPRESSOR_POWER_UNUSED`.
* **Fix:** `COMPRESSOR_POWER_CONVERTER_MANUAL` (1) is the default; a diesel's "Main" becomes
  "Engine" as in the original (Mover.cpp:11738). A TEM2 (no `CompressorPower=`) now needs its
  converter on for air, as in the original (110 V only with `ConverterFlag`, Mover.cpp:1805): the
  start-up test switches the converter on after the engine as the original's AI does for every
  vehicle (Driver.cpp:2799-2806).
* **Rule:** a property's default is the original's value for an absent key - check the
  `extract_value`/lookup fallback, not only the parsed values.

## 2026-10-05 A UTF-8 BOM hid an MMD include from the fixture cutter

* **Symptom:** EN57KM's cab had no definition in the test ("No cab1definition"), its pantographs
  could not be raised.
* **What proved it:** `akm_ex_czoper_ra_1562.mmd` starts with a BOM before `include`; read as
  cp1250 it glued to the keyword, so `scripts/cut-vehicle-fixture` never copied `*_ra_base.mmd`.
* **Fix:** the cutter drops a leading BOM before tokenising; EN57KM re-cut.
* **Rule:** a tool that reads the game's text files skips a BOM as the game's parser does.

## 2026-10-05 An EZT cab car's reverser never went back from forward

* **Symptom:** EN57-702ra: the reverser set to forward could not be set back to neutral or reverse.
* **What proved it:** a headless probe on the EN57 fixture trainset - `direction_decrease` left
  `direction` at 1. `DirectionBackward()` on an EZT at forward first switches the high start off
  (`MinCurrentSwitch(false)`, Mover.cpp:3250), which returns true whenever `Imin == IminHi`. A cab
  car has no engine, so the wrapper wrote no `IminLo`/`IminHi`/`Imin` and all three stayed 0: the
  switch "succeeded" every time and the direction never moved. The original sets 1/2/1 for every
  EZT in `LoadFIZ_Param` (Mover.cpp:10300-10305), before `Circuit:` overrides them.
* **Fix:** `MoverRailVehicleController` gives an EZT those thresholds when nothing has written them;
  the FIZ factory gives an EZT's electric engine the same defaults before `Circuit:`.
* **Rule:** a default the original sets in one section for keys another section owns is carried
  over to the vehicle even when the other section's component is absent.

## 2026-10-05 The player became the AI driver of the vehicle it took over while loading

* **Symptom:** td.scn, EP07-424: handed to the AI and taken back, the line breaker and the converter
  dropped.
* **What proved it:** a headless probe on td.scn tracing the roles - at the hand-over and the
  take-back only the player's own role changed, and after the take-back the player was DRIVER while
  `DriverSystem.vehicle_is_control_active()` was true. The player (`MaszynaPlayer.auto_start`) takes
  the first vehicle on `vehicle_configured`, inside its build, while the scenery is still loading;
  the scenery's driver goes to OBSERVER. `SceneryInstancer` then gave the AI delegate to whoever sat
  in the DRIVER role - the player's person - so the AI drove with the player, as the player.
* **Fix:** the AI delegate goes to the person `MaszynaLegacyVehicleSystem` seated for the scenery
  (`vehicle_get_driver()`), whatever role it has by then
  (`test_zzz_ep07_ai_hand_over.gd`).
* **Rule:** a role, a delegate or a seat is given to the person its owner created, never to
  "whoever holds the role now" - the occupancy changes while a scenery loads.

## 2026-10-05 CI was green over red tests - tee replaced GUT's exit code

* **Symptom:** after `set -o pipefail` in the GUT step (`023ef1c46`) CI failed on nine tests; the
  runs before it were green.
* **What proved it:** the logs of the green runs (`ad2d7b1e7`, `c259f3b76`) - their GUT summary
  already said "Failing Tests 8" and "3": `godot ... | tee gut.log` took tee's exit code, so red
  runs passed. `test_zzz_startup_en57_2000_v1` and `test_zzz_startup_sr61_v2` were never green.
* **Fix:** `pipefail` (`023ef1c46`), then the failures themselves (the two entries below).
* **Rule:** a CI step that pipes a test run keeps the run's exit code (`set -o pipefail`); a green
  run is checked against its summary, not only its status.

## 2026-10-05 Test vehicles lost their brake handle and doors to the original's defaults

* **Symptom:** `test_legacy_cabin_keys` - the brake handle did not move on its key, and
  `brake_level_set` did not move it either; `test_rail_vehicle_load_exchange`,
  `test_station_server`, `test_driver_station` - the car never opened its doors.
* **What proved it:** `ad2d7b1e7` set `RailVehicleBrake`'s defaults to the original's for absent
  keys (no handle, 0 positions, individual brake, no delays) and `RailVehicleDoors.max_shift` to 0.
  The old defaults were SM42's `Cntrl.` line (`6d1.fiz`), and `sm42_vehicle.tres` set none of it;
  a door range of 0 is "no doors" to the Mover (`update_doors()`, Mover.cpp:7920), and
  `build_passenger_car()` set none.
* **Fix:** the fixture carries SM42's brake description from `6d1.fiz`; `build_passenger_car()`
  gives its doors a travel.
* **Rule:** a hand-built test vehicle states every value the test depends on - it never leans on a
  component's default, which follows the original's absent key.

## 2026-10-05 Start-up tests read the powered car before the trainset was coupled

* **Symptom:** in CI only, `test_zzz_startup_en57_2000_v1`: "EN57-2067ra has the engine of
  ELECTRIC_MULTIPLE_UNIT" got engine 0; ED72 and EN57AL red only in the full run.
* **What proved it:** the CI log - the test reached the check 0.4 s after the scenery loaded.
  `MaszynaLegacyVehicleSystem` builds vehicles within a per-frame budget, a trainset is coupled
  only once all its vehicles are ready (`RailVehicleServer.trainset_place()`), and the test waited
  for the player's car alone: on a slow runner `vehicle_find_powered()` returned the cab car. The
  next test's load then resumed the freed scenery's coroutine, and `_is_load_given_up()`'s typed
  parameter refused the freed node before `is_instance_valid()` could say so.
* **Fix:** `MaszynaStartupTest` waits for every vehicle of the scenery and resets the clock to 1x
  at once after a test; `SceneryInstancer._is_load_given_up()` takes an untyped `root`.
* **Rule:** a test of a trainset waits for the whole trainset, not for the car it acts on; a
  function asked whether an object is gone takes it untyped - a typed parameter fails on a freed
  object at the call.

## 2026-10-05 A cab change moved into RailVehicleServer left the cab active in the state

* **Symptom:** with the cab change moved from `CabinSystem` (GDScript) into
  `RailVehicleServer.person_change_cabin()` (C++), `test_train_cab_change` read `"cabin"` 0 after
  changing to the rear cab (expected -1), and `test_rail_vehicle_cabins` saw the master controller
  still off its rest after a driver's cab change.
* **What proved it:** the same operations sent from GDScript as commands passed. The C++ version
  called the controller's methods (`cab_deactivation_auto()`, `cab_controls_reset()`,
  `cab_activation_auto()`) directly. `VehicleServer.vehicle_dump_state()` is cached on the
  controller's state serial, which only a step and `VehicleController::command_executed()` move -
  the Mover had changed, the dump the tests read had not.
* **Fix:** `person_change_cabin()` sends `vehicle_send_command()`; the gangway's
  `cabin_leave`/`cabin_enter` are registered commands.
* **Rule:** outside a vehicle's own composition an action on it is a command, never a call of the
  controller's or a component's method (`CODE_STYLE.md`, "A vehicle is commanded, not called").

## 2026-10-06 The EP09 fixture fills its pantographs from the main reservoir on its own

* **Symptom:** a probe of report MaSzyna-Reloaded/reports#9 (EP07, pantograph tank cut off from
  the main reservoir) on `startup_ep09_v1.scn` never lost its pantograph air: with the three-way
  valve on the small compressor the tank still followed the main reservoir.
* **What proved it:** `104e-039.fiz` has `PantAutoValve=Yes`, and `UpdatePantVolume()`
  (Mover.cpp:783) then feeds the tank whenever it is below the main reservoir, whatever
  `bPantKurek3` says. The 303E/EP07 files (`303e-ep.fiz`) have no `PantAutoValve`, so the valve
  decides. The same probe on `ep07.scn` (EP07-424) reproduced the report.
* **Fix:** none needed in the code; the probe moved to the EP07 fixture.
* **Rule:** before reproducing a pantograph-air report on a fixture, check the fixture's FIZ for
  `PantAutoValve` - the EP09 and the EP07 behave differently with the same valve.

## 2026-10-06 Parked vehicles scanned for neighbours in every sub-step

* **Symptom:** Wrzosy EIC (`wrzosy_eie2620.scn`, ~850 vehicles), player in the cab, profiling
  build: the main thread busy 96% of the time, and the vehicle step (`stepping_advance`) is 35.5%
  of it.
* **What proved it:** `perf record --call-graph dwarf` of the main thread for 20 s under
  `gamescope --backend headless` (real GPU, no window); the stacks were summed by the phase of
  `MaszynaMoverVehicleServer::stepping_advance()` they ran in. Of the main thread: the neighbour
  scan (`_update_neighbours` / `_find_vehicle`) 10.2%; the forces and movement 8.1%; the
  locations and track movement 4.6%; reporting placements to rendering 4.5%; current collection
  and components 3.4%; overhead 3.6%. The sub-step loop that could run on worker threads came to
  ~23%, the serial phase after it to ~9%.
* **The cause:** every vehicle walked the track for its nearest neighbour in every sub-step, even
  when it and everything around it stood still.
* **Fix:** each end remembers the tracks its last scan went along and the change serial it was
  made at (`RailVehicleServer::VehiclePlacement::NeighbourScan`). A track's serial goes up when a
  vehicle moves on it, enters or leaves it, or its switch is set, and a change of the network
  stales every scan. An end scans again only when one of its tracks changed. Neighbour scan 10.2%
  -> 4.0% of the main thread, the step 35.5% -> 30.9%.
  `test_a_parked_vehicle_is_pushed_by_one_rolling_onto_it` fails with the invalidation on
  movement left out.
* **What stays:** a parked EMU or DMU keeps its physics on - the Mover's own rule
  (`TrainType == dt_EZT || dt_DMU`, Mover.cpp:4487-4489), not the wrapper's.
* **Threads:** the sub-step loop is the only part that could run per island on workers, and it
  calls the Mover's global `std::mt19937` (`utilities.cpp:35-36`: `Adhesive`, `ComputeMovement`,
  `CouplerForce`), which a change of the vendored Mover would have to make per thread. With the
  scan cut the most that threads could take off the main thread is ~10%.
* **Rule:** a per-sub-step query whose answer depends only on placements is cached per vehicle
  against a change serial of what it read; measure the split by phase with `perf` before
  proposing threads.

## 2026-10-06 A killed test leaves the game directory pointing at the fixtures

* **Symptom:** `godot-double --path demo -- -s wrzosy_eie2620.scn` quit at once with "Invalid
  game directory: res://tests/fixtures", inside and outside the sandbox.
* **What proved it:** `demo/project.godot` sets `custom_user_dir_name="MaSzyna-Reloaded"`, so
  `user://` is `~/.local/share/MaSzyna-Reloaded/`, not `app_userdata/MaSzyna Reloaded/` (which
  still held the right directory). Its `settings.cfg` had `game_dir="res://tests/fixtures"`,
  written at 08:56 by a test that points the game directory at the fixtures
  (`save_maszyna_game_dir(FIXTURES_GAME_DIR)`) and was killed before restoring it.
* **Fix:** the setting restored by hand; the `godot-frame-time` skill checks it before a run.
* **Rule:** before a run that loads the game's data, read `game_dir` in
  `~/.local/share/MaSzyna-Reloaded/settings.cfg`; a test run that was killed may have left the
  fixtures there.

## 2026-10-06 The eszelon stood at a dwarf that had opened

* **Symptom:** on Stary Jawor - Eszelon the player drove the SU46 past the first signal, stopped
  at the next dwarf (Tm, `ms2nbk.inc`) and saw it show Ms2 (`ShuntVelocity 40`). The driving
  aid kept showing STOP and the hints cued neutral and the brakes.
* **What proved it:** the scenery's data has a `ShuntVelocity` memory for each Tm, and opening
  it writes `ShuntVelocity 40 0` (`_m40`). The table read the memory again on every update, but
  a signal the front had reached took effect once, in `_pass()`, when the front reached it. A
  Tm reached at stop set `signal_velocity_last` to 0 and nothing read it again, so
  `velocity_limit` stayed 0. Its opening also gave no `ShuntVelocity`, because a command to go
  came only from signals ahead. The original runs `TableUpdateEvent()` on every update for
  every point still in the table. A passed proper signal sets `VelSignalLast` each time
  (Driver.cpp:1554-1558), and a passed Tm that opens gives `cm_ShuntVelocity` and leaves the
  table (Driver.cpp:1649-1665, "ustawienie, gdy przejechany jest lepsze niż wcale"). A player's
  passed signal is dropped further than `max(fLength + 100, 250)` m (Driver.cpp:1549-1553).
  `test_driver_route_table.gd`'s two new tests fail on the old table.
* **Fix:** `_pass()` is gone. The table's loop reads a reached signal on every update, as the
  original does: the speed in force, a passed Tm that opens, and the player's distance.
* **Rule:** a signal the front has reached is read again on every update while it is in the
  table. One taken once on passing holds the aspect it showed then.

## 2026-10-06 The eszelon turned back to an open dwarf and still read STOP

* **Symptom:** after the fix above the operator still saw STOP on Stary Jawor. Tm18 let the SU46
  past Tm19, then the script opened Tm19 and Tm20, which face the other way, for the way back
  to H1. The operator changed the cab and put the reverser forward. The neutral hint was gone,
  but the large STOP stayed.
* **What proved it:** in the data, Tm19 and Tm20 (`skp_wskazniki.scm:167-168`, angle -50) face
  away from Tm18 (131.5), so the open dwarf was behind the train. In the original, a player's
  reverser (`OnCommand_reverser*`, Train.cpp:2670-2842) and cab change (`CabChange()`,
  Train.cpp:10343) call `Mechanik->DirectionChange()`. That sets `iDirection = CheckDirection()`
  (DirAbsolute, else CabActive). The port's driver changed its way only through its own orders,
  so it kept reading the tracks ahead of the old cab. `test_a_players_reverser_turns_the_driver`
  fails without the change.
* **Fix:** the driver follows `vehicle_driver_cabin_changed` and the vehicle's
  `direction_increase`/`direction_decrease` commands of a vehicle a player drives. Its way is set
  from `get_direction_absolute()`, else the active cab, and the trainset is read again.
* **Rule:** whatever the original lets the player change in the cab and also tells the driver
  (`Mechanik->...` in Train.cpp) has to reach the port's driver. A driver kept apart from the
  cab reads a different railway than the player drives.

## 2026-10-06 The demo's line breaker keys took the rear motor blowers' Shift+M

* **Symptom:** porting the motor blowers' switches, the original's key of the rear blowers (Shift+M)
  was already the demo's `main_switch_off`.
* **What proved it:** the original binds M to `linebreakertoggle` and nothing to
  `linebreakeropen`/`linebreakerclose` (driverkeyboardinput.cpp:108-110, 293-294); the demo's
  M/Shift+M on/off pair dates from its first commit and was never the original's.
* **Fix:** M toggles the line breaker as `linebreakertoggle` - a key of LegacyCabinMainSwitch's
  own (`main_switch_key`), pressed and let go whatever switches the cab has: tied to an unmodelled
  `main_sw` it only flipped the switch, never let go, and no series motor cab closed the breaker.
  The opening alone moved to Ctrl+Shift+M, which the original leaves free; Shift+M is the rear
  blowers again.
* **Rule:** a key is the original's binding (`driverkeyboardinput.cpp`); a free key of ours only
  for an action the original leaves unbound.

## 2026-10-06 demo_3d put the player in the Impuls looking from outside

* **Symptom:** demo_3d made the player drive impuls-a, but the view stayed outside: no cab was shown.
* **What proved it:** a headless probe of demo_3d. The player takes over the first vehicle that gets
  its controller (`player.gd` `_on_vehicle_configured()`), which happens inside
  `MaszynaLegacyVehicleSystem._build()` at `vehicle_bind_controller()`. At that moment the vehicle
  is not in `RailVehicleRenderingServer` yet ("Parameter visual is null" in `vehicle_mount_node()`)
  and `CabinSystem` has no cab scene ("the vehicle has no cab interior to show"). Nothing showed
  the cab later. Showing it when the scene came, but before the cab logic was attached, hung the
  game in `Cabin3D.set_vehicle_rid()` (`MaszynaDynamicTrainCabin._rebuild_generated()`): probed
  with prints, the probe got no further in 150 s; with the logic attached first it went on.
* **Fix:** `CabinSystem.vehicle_cabin_scene_changed` announces the scene; the player shows the cab
  of the vehicle it drives when it takes it over with a scene already there, or when the scene
  comes. `_build()` attaches the cab logic before it hands the scene over.
  `test_the_cab_is_shown_when_its_scene_comes_after_the_vehicle_was_taken_over` covers the order.
* **Rule:** a cab's scene is the last thing of the cab a builder hands over - it is the event the
  cab is shown on, and a cab built before its logic hangs.

## 2026-10-06 The driver took the way of the cab left, one cab change late

* **Symptom:** on Stary Jawor the operator changed the cab while passing the dwarf. STOP stayed
  when the dwarf turned white. Only after a second cab change did the aid show 40, and it then
  showed the same in both cabs.
* **What proved it:** a probe (SM42, the player in the front cab, reverser forward, then
  `person_change_cabin()` backward) printed the following after the change: `CabActive -1`,
  `direction_absolute -1`, driver direction `1`. Changing back gave `1`/`1` against a driver
  at `-1`, which is one change late. The first version turned the driver on
  `vehicle_driver_cabin_changed`. That signal is emitted when the person moves, before
  `person_change_cabin()` sends `cab_controls_reset` and `cab_activation_auto`, so the cab
  left was still the active one. The original calls `DirectionChange()` after
  `CabActivisationAuto()` (Train.cpp:10335-10336).
* **Fix:** the driver turns on the `cab_activation_auto` command, the last step of a cab change,
  and on the reverser. The probe shows the driver following each cab at once.
* **Rule:** what the original does after a step of an operation is hooked to that step's own
  event, not to the first event of the operation.

## 2026-10-06 A light switch in a cab switched off lit the other end's lamps

* **Symptom:** on Stary Jawor's SU46 the Tb1 hint ("switch on Tb 1 head lamp code") stayed
  whatever the operator lit, from either cab.
* **What proved it:** probes on the SU46, SU45 and ST45 fixtures, driven by the keys. With the
  right lamps (the occupied cab's end showing only its right lamp, the far end only its left; on
  the ST45 the presets "Tb1b" from the front cab and "TB1a" from the rear) the hint cleared in
  every cab, as the original's check does (driverhints.cpp:1281-1288). In the SU46's rear cab
  switched off (`CabActive` 0), though, its right-lamp switch put out the front end's right lamp.
  `MoverRailVehicleLighting::_active_end()` took the end from `CabActive`. The original takes it
  from the cab the driver sits in (`cab_to_end()`, `iCabn`, Train.h:220-227).
* **Fix:** the end is taken from `CabOccupied`, and the machine room counts as the front.
* **Rule:** a cab's controls act on the end of the cab the player sits in (`cab_to_end()`), not on
  the active cab. A cab switched off is still the one the player works.

## 2026-10-06 the Vehicles stage drew every vehicle of the scenery

* **Symptom:** on a large scenery the loading screen's Vehicles stage took 35.1 s, with 1 s
  frames and 2.49 GB resident. Hundreds of `Model /dynamic/pkp/14xa_v1 has no skins set` warnings
  showed that every vehicle's materials were being resolved, near the player or not. Even
  `demo_3d.tscn` built its few vehicles slowly.
* **What proved it:** a headless probe timed each part of a vehicle's build. The simulation took
  about 2 ms a vehicle (the FIZ description, configure and bind), and the appearance took 45-140
  ms even with every cache warm. `vehicle_set_appearance()` built every model at once.
  Splitting it further, `E3DRenderingServer.instance_build()` took 12-40 ms a model. The same model
  built again while its materials were still held took about 1 ms, so the cost was the materials
  and textures created for each type and skin. The only part the simulation took from the drawing
  was `_publish_pantograph_geometry()`, read off the built instance.
* **Fix:** `RailVehicleRenderingServer` builds a vehicle's own models when it stands within the
  streaming's draw distance of its camera. The nearest go first, `BUILD_BUDGET_MSEC` a frame,
  from the vehicles' sweep in `_update_detail()`. It frees them beyond the draw distance and its
  margin. Without a camera (while a scenery loads) nothing is built; the editor builds at once.
  The pantographs' geometry is measured off the model file (`model_load()`, the submodels' rest
  transforms and the slider's mesh), not off its drawing. It is read only for a vehicle with a
  power source component and published on `vehicle_set_appearance()` and on the config event.
  On Galicja (`linia_107_poludnie.scn`, headless under gamescope, two runs each) the Vehicles
  stage went from 2.7-2.8 s to 0.5 s, the longest frame from 0.15-0.18 s to 0.08 s, and the
  resident memory after the load from 2.16 GB to 2.07 GB. The skin warnings at load went from 22
  to 0.
* **Rule:** a vehicle's models are built when it comes within the draw distance and freed beyond
  it, never at load. What the simulation needs of the model is read off the model file, not off
  its drawing.
* **Since 2026-10-10:** the model files of a vehicle's appearance go through `ResourceLazyLoader`
  (registered in `vehicle_set_appearance()`, held by the build). Without lazy loading - the
  default - they are loaded with the vehicle and kept; only the models built from them follow the
  draw distance.

## 2026-10-06 A cab change in the 36WEa froze the game for half a minute

* **Symptom:** walking the 36WEa unit with End, the game got slower and slower until it froze
  completely. After 30-60 s the player was standing outside. `app.log` showed the driver's cab
  (`36wea-a_ks.mmd`, 93 instruments) being built again on every pass through a front or rear cab.
* **What proved it:** a headless probe (fixture scenery `startup_36wea-014a.scn`, real game data)
  timed the build of the cab by its steps. `MmdCabinInstancer.parse()` took 9 ms, the cab model
  116 ms and `build_into()` 30 s. Inside it `_submodel_islands()` merged lamp pieces in 22.3 s for
  `i-dashboardlight_on` (727 pieces, unwelded triangles) and 6.8 s for `i-dashboardlight_pom_on`
  (487). Every merge measured every pair again, which is O(P^3), in GDScript. On the ED78 fixture
  without cab models the same walk cost 7-22 ms a change, with nothing growing.
* **Fix:** every piece keeps its nearest piece and the gap to it. A merge changes only the grown
  piece. Each other piece compares its gap to it with the gap it had: no farther means the grown
  piece is now its nearest. Only a piece that had one of the two merged pieces nearest and is now
  farther from the grown one looks through all the others again. The merges come out the same (2,
  2 and 3 lights). `i-dashboardlight_on` now takes 233 ms, and the whole cab build 1.1 s.
* **Rule:** a build step that runs on every cab change is measured on the largest real cab, not
  on a fixture. A fixture without models skips the code that is slow.
* **Follow-up, the same day:** the remaining 1.1 s was mostly the lamps' texture. Every lamp mesh
  decompressed its whole 4096 px BC3 texture (53 ms) and scanned it in `detect_used_channels()`. That
  happened again for every mesh, and the decompression also changed the image the texture returned
  in place. In the player's cab (`36wea-a_kd.mmd`) the dashboard light is one mesh of 13277
  triangles in 7237 pieces, which merged into one light in 2.0 s (debug build). The islands moved to
  C++ (`LegacyCabinLampIslands`, described as a workaround for E3D models). A sample decodes only its
  DXT block, with bcdec's formulas, and a grid finds each piece's nearest. A mesh of more than 1024
  pieces is a backlight and gets one light at once. Against the GDScript on all 26 lamps of the
  36WEa cab, the same lights at the same positions, colours within 0.024. The cab build went from
  1.1 s to 0.43 s; the dashboard light from 2.0 s to 42 ms. A walk through the unit took at most
  288 ms a cab change (the first entry of the other driver's cab).

## 2026-10-06 SN61: the independent brake did not brake, the engine stopped after the start

* **Symptom:** the operator drove SN61-02 off; "the breaker trips" (the engine stopped), and the
  independent brake seemed not to brake.
* **What proved it:** `test_zzz_driver_hints_sn61_v2` driven on: after the start the hint list's
  first step was "Set master controller to neutral" (`Kp-`); followed, it took the controller to 0
  (`R=0`, no fuel) and the engine stopped. The original's check of that hint is
  `IsMainCtrlNoPowerPos()` (0 for an SN61), while its own action for a diesel (`DecSpeed()`,
  Driver.cpp:3740-3749) stops on the first position without the clutch in. The independent brake:
  handle 1.0, `LocBrakePress` 3.8 bar, cylinder 0.00 bar. An ESt3 is a `TNESt3` that passes the
  independent brake's pressure only to a `TPrzek_PZZ` relay, chosen by `SetSize()` from the
  `BrakeValve=` text (`ESt3d_PZZ`, Oerlikon_ESt.cpp); the wrapper decoded the type and never set
  `BrakeValveParams`, so no ESt3 got its `PZZ`, `AL2`, `-s216` or `-ED` relay.
* **Fix:** the "neutral" hint of a diesel with a gearbox is done on a position without the clutch
  in; `RailVehicleBrake.valve_parameters` keeps `BrakeValve=` as written and the Mover wrapper
  passes it as `BrakeValveParams` (FIZ format 45). With both, SN61-02 drives off by the player's
  keys, stays running, and the independent brake fills the cylinders to 3.8 bar and stops it; each
  fix alone was shown missing by the test going red without it.
* **Rule:** a Mover field the original reads as text (`BrakeValveParams`) is passed as text, not
  only decoded; a hint's check agrees with what its own action does.

## 2026-10-06 SN61 did not start by the driving aid's hints

* **Symptom:** SN61-02 (calkowo_sn61_zima.scn, rear cab) could not be started by the player. The
  driving aid listed "Close line breaker" (M) first, "Set reverser to reverse" and "Set engine to
  idle" (without a key) after it; M did nothing with the master controller at 0, the pumps on.
* **What proved it:** `test_zzz_driver_hints_sn61_v2` - the vehicle cut from that scenery, started
  by nothing but the listed hints, top to bottom, each by the key the window shows, held in real
  time as a player holds it. It stood exactly where the player stood. Prints in `cue()` showed the
  list reordered on every update: a hint ruled out its whole group, itself included, so the idle
  and the reverser hint were removed and appended again behind the line breaker each time they
  were cued; and the neutral hint, already done at 0, removed the idle too. The original does
  both (`remove_master_controller_hints()`, `remove_reverser_hints()` before the check,
  driverhints.cpp). With the order fixed, the reverser hint stayed undone: it counts along the
  vehicle (`DirActive * CabActive`, driverhints.cpp:921, 932), and from the rear cab the vehicle's
  "reverse" is the reverser's forward - the hint's words and key pointed the other way. At 0
  (`R=0`) `dizel_StartupCheck()` cancels the start (Mover.cpp:7063-7069), as in the original. The
  test also needed keys held in real seconds: a knob moves at the hand's speed (`KNOB_KEY_SPEED`),
  and two simulated seconds at 100x moved the independent brake nowhere.
* **Fix:** a hint rules out the others of its group, not itself, and only when it is listed;
  the hints window shows a rear cab the reverser hint of the reverser's own side (words and key);
  the idle hint has the master controller's "up" key (its step only goes up,
  driverhints.cpp:491-494) and is cued before the line breaker in the diesel traction step as
  well. `VehicleController.Direction` names the reverser's positions everywhere; the start-up
  tests' "forward" is the reverser's from any cab (`DirAbsolute = DirActive * CabActive`,
  Mover.cpp:669).
* **Rule:** the hint list is what a player follows top to bottom: it keeps the order the steps
  came in and shows each in the player's cab's terms; a hint is tested by following the list by
  its keys, in real-time key holds.

## 2026-10-06 SR61 start-up test red in the suite

* **Symptom:** `test_zzz_startup_sr61_v2` green alone, red in the full run and in CI: "start-up
  stopped at compressor: main reservoir", the engine started and then at 0 rpm with the main
  switch open, the controller at position 0.
* **What proved it:** the test started the SN61 at controller position 1 and went back to 0 as
  "idle". `sr61v1.fiz` gives position 0 `R=0`: `dizel_fillcheck()` gives no fuel there
  (Mover.cpp:7144-7228), the torque is `-Mstand` and at 0 rpm the engine switches off
  (Mover.cpp:7384). It passed alone by a quirk: the starter (`dizel_spinup`, Mover.cpp:7269) adds
  `Mstand / (0.3 + enrot/nmin)`, which balances `Mstand` at 0.7 x nmin, and spin-up ends only past
  0.95 x nmin (Mover.cpp:7117) - a decrease one fast frame after ignition left a fuelless engine
  held by the starter. Slower frames at simulation speed 100 let the engine reach its governed
  ~740 rpm first, spin-up ended, and the decrease stalled it.
* **Fix:** the test goes up from the starting position to the first one with a clutch engaged
  (`RailVehicleDieselEngine.get_clutch_desired()`, `RList[MainCtrlPos].Mn > 0`), the original's
  idle (`mastercontrollersetidle`, driverhints.cpp:489; Driver.cpp:5778). The vehicle itself
  behaves as the original.
* **Rule:** a diesel's idle is the first controller position with `Mn>0`, never a position with
  `R=0`; a start-up that passes only with fast frames is racing the starter.

## 2026-10-06 Vehicle not ready with nothing missing

* **Symptom:** in a 36WEa the driving aid kept showing "Vehicle not ready" with STOP - first with
  no list of what was missing, and once the readiness check read the whole trainset, with the line
  breaker listed while it was closed. The hints asked to raise pantograph B, which the cab could
  not raise.
* **What proved it:** `test_zzz_driver_ready_36wea_014a.gd` - the unit started by the keys and moved
  off - printed every car: the A and C cars (`PWR=1000`) with their line breakers closed, the B car
  `PWR=2`, no engine, `main_switch_enabled` absent; and the pantograph unit, the A car, with
  `CollectorsNo=1`, pantograph A up at 3499 V. `MaszynaLegacyDriverTrainset` counted every car over
  `Power > 0.01` (`IsAnyLineBreakerOpen`, Driver.cpp:6143-6144) and a car without an engine as a
  line breaker open, so the readiness was taken away on every update; `_prepare_engine()` read the
  controlling car alone and listed nothing. The original closes the B car's `Mains` like any car
  with a master controller (`MainSwitch_()`, Mover.cpp:3621-3645). The hints asked for both
  pantographs of any vehicle with a collector (Driver.cpp:2782-2813) and applied the vehicle's
  pantograph setup with both (Driver.cpp:6276-6319).
* **Fix:** `_prepare_engine()` reads the line breaker and the converter overload relay of the
  trainset, as the reset does; the trainset counts a line breaker only of a car with an engine; a
  vehicle with one collector is asked only for pantograph A, and the setup applies only to a
  vehicle with two (MASZYNA_ORIGINAL_QUIRKS.md, "A pantograph car's line breaker", "Pantograph B
  of a vehicle with one").
* **Rule:** a readiness check and the reset that takes it away read the same state, and only of
  devices a car has; a hint asks only for what the vehicle has.

## 2026-10-06 Os33733 left Zagórz before its departure

* **Symptom:** in Galicja (`linia_107_poludnie.scn`, 12:30) Os33733 stood at Zagórz waiting for
  its departure at 12:47; once the player readied the SU42 and drew up to the signal, the
  timetable passed Zagórz and the panel showed -15 min.
* **What proved it:** the first station has no arrival, so it is made a stop (mtable.cpp:570-574,
  `MaszynaLegacyTimetableFactory`); the wait was right. The SU42's FIZ has `BrakeDelays=GP`, which
  starts the Mover at G (Mover.cpp:12040-12042), and a player's own locomotive is never re-set
  (Driver.cpp:2194). `MaszynaLegacyDriverBraking.read_trainset()` took `cargo` from that setting
  (IsCargoTrain, Driver.cpp:2303) once the order became OBEY_TRAIN, and a goods train leaves a stop
  at once (`route.gd`, Driver.cpp:1297). The -15 is the arrival delay stored at 12:32.
* **Fix:** `RailVehicleServer.trainset_determine_type()` / `trainset_get_type()`
  (NONE, PASSENGER, CARGO, MIXED), from the cars' `BrakeDelays` as AutoRewident() counts them;
  the driver's AutoRewident port determines it, and every consumer of the original's
  IsCargoTrain/IsPassengerTrain (stops, proximity, braking level and distance, series motor,
  heating, the brake settings' passenger/goods choice) reads it. `braking.cargo` and the heating's
  own copy are gone (MASZYNA_ORIGINAL_QUIRKS.md, "A train is a goods train when its driver's
  locomotive is set to G").
* **Rule:** the type of a train comes from `RailVehicleServer.trainset_get_type()`, never from the
  brakes or a vehicle's G/P setting.

## 2026-10-06 "Switch on compressor" with the compressor on

* **Symptom:** in EN57-636ra on Linia 053 Poranek the compressor was switched on and running, and
  the hints kept asking to switch it on.
* **What proved it:** `test_zzz_driver_hints_en57_2000_v1.gd` - the EN57 started by the keys -
  printed every car's compressor: the ra cars `CompressorSpeed` 0.018, allowed and running; the s
  car, the controlling one, none. The hint was done by the controlling car's compressor running
  (`CompressorFlag`), which an EN57 has not got and which stops by itself once the reservoir is
  full. The original's hint is done by `IsAnyCompressorEnabled` - any vehicle under control with
  its compressor allowed to run (driverhints.cpp:394-414, Driver.cpp:6136-6137).
* **Fix:** `MaszynaLegacyDriverTrainset` keeps `compressor_enabled` and
  `compressor_explicitly_enabled` of every vehicle under control, and the compressor hints are done
  by them.
* **Rule:** a hint about a device is done by the device's switch on any vehicle of the consist
  that has it, not by the device running on the controlling car.

## 2026-10-06 EN57 braked weakly - its trailers' wheels locked

* **Symptom:** an EN57 (EN57-636ra, `pkp/en57_v2`) braked as if its brakes barely worked.
* **What proved it:** `test_zzz_startup_en57_v2.gd`'s fixture (cut by `cut-vehicle-fixture`, which
  needed `find_path()` for an include in `misc/`), braked from 40 km/h at FVel6's position 3: every
  car's cylinders reached MaxBP 4 bar, and at ~26 km/h the trailers' brake force fell from 82 kN to
  28 kN at the same pressure - locked wheels. The cars give `MaxBPMass=52` and `TareMaxBP=2.5`:
  an empty 34 t car brakes at 2.5 bar (`TEStEP2::PLC()`, hamulce.cpp:1264, from
  `SetLP(Mass, MBPM, TareMaxBP)`, Mover.cpp:11763). No FIZ parser read `MaxBPMass`, so `MBPM` kept
  its 1.0 and every car braked as fully loaded.
* **Fix:** `RailVehicleBrake.cntrl_max_brake_pressure_mass` [t], read from `Cntrl.` and given to the
  Mover in kilograms (Mover.cpp:10771-10775); `FIZ_PARSER_FORMAT_VERSION` 43. The trailers now brake
  at 2.49 bar, the motor car at 2.79.
* **Rule:** grep every `extract_value()` key of a `LoadFIZ_*` against the parser before trusting a
  vehicle's physics; the keys still unread are in `TODO.md`.

## 2026-10-06 36WEa: "deactivate the cab" and "raise pantograph A" in the C car's cab

* **Symptom:** 36WEa-024a (l053_poranek.scn) at the platform; the player walked through the gangways
  to the C car's rear cab to drive back. The driving aid asked to raise pantograph A, which could
  not be done, and after the cab was activated to deactivate it, with a green signal ahead.
* **What proved it:** `test_zzz_startup_36wea_024a.gd` on the unit's own fixture, the walk by the
  cab change keys. The driver's person (DriverSystem) moved into the C car's front cab with the
  gangway, and the in-vehicle cab changes after it moved the player alone:
  `PlayerServer._on_cabin_person_moved()` returned for a cab of the same vehicle. The driver's cab
  (+1) against the active rear cab (-1) is the original's condition for the hint
  (`CabActive == -CabOccupied`, Driver.cpp:6017). The C car's one pantograph is B
  (`PhysicalLayout=2`), which the loader counts as two collectors (Mover.cpp:11636-11637); the
  hints asked for A (one collector) or for both (two).
* **Fix:** the driver follows the player into every cab, of the same vehicle too. The pantograph
  hints go by the layout's bits, not by `CollectorsNo`: one pantograph, the one it has.
* **Rule:** the original's driver is the train's and sits in the occupied cab - every move of the
  player moves the driver; which pantographs a vehicle has is its `PhysicalLayout`.


## 2026-10-06 Crash at 0% loading on D3D12

* **Symptom:** a player on Windows (GTX 960, D3D12) crashed at 0% of loading Galicja, build
  20261006-1712. The log has `Close failed with error 0x80070057` in `command_buffer_end` right
  after `[Skydome] (Re)Initialized`, then ~1400 `Can't create buffer ... 0x887a0005`, failed
  pipelines and textures, and the crash on a texture read-back; the scenery went on parsing on the
  CPU meanwhile (`FILES 27.9 s`).
* **What proved it:** `0x887a0005` is `DXGI_ERROR_DEVICE_REMOVED`, and the first error before it is
  E_INVALIDARG on closing a command list - an invalid operation recorded, in the first frames with
  a 3D camera. The one compute effect there is gnd-skydome's sun shafts
  (`SunShaftsCompositorEffect.gd`): with no MSAA there is no `resolved_color`, so the colour it
  samples (`get_color_layer(view)`) is the image it writes - one texture as a sampler and a
  storage image in one dispatch. Vulkan allows it (GENERAL layout); D3D12 cannot hold a subresource
  as SRV and UAV at once. `project.godot` sets `driver.windows="d3d12"`, sun shafts are on by
  default. On Vulkan the same is a silent race: the shader reads pixels already overwritten.
* **Fix:** a compute pass copies the colour into a texture of the effect's own (`imageLoad`/
  `imageStore`; the scene colour has no `CAN_COPY_FROM`, `texture_copy()` refuses it) and the
  shafts sample the copy; the colour layer is only written. Checked on Vulkan under gamescope
  headless (`$td.scn`): no errors; D3D12 unconfirmed until a Windows player reports. The Windows
  build runs on Vulkan again (`driver.windows`, d3d12 since the Godot 4.6 bump `64dbf13a0`), the
  renderer the game is developed and tested on. Regression tests: `test_project_rendering_driver.gd`
  and `test_sun_shafts_compositor_effect.gd` (renders on a GPU, pending under `--headless`), both
  red on the broken versions.
* **Rule:** a compute effect never samples the texture it writes in the same dispatch.


## 2026-10-07 Tests waited on the machine's speed

* **Symptom:** tests on GitHub CI failed at random and some ran for minutes: a wait of 120 s for a
  vehicle's detail, start-up tests run at simulation speed x100 with a real-time limit per step,
  "wait N idle frames" for a command, a key or a cab element to act. The same script passed
  alone and failed in a batch or on a slower runner; one waited for a scenery whose vehicles
  could not be detailed at all (fixtures without models), so it only ran out.
* **What proved it:** the simulated time a step of the test got was a function of the frame rate
  (`SimulationRuntime` advances by the frame's delta, the x100 speed multiplied it), and so was
  how far a cab element, a sound or a held key moved between two checks - cab tools, lamps, the
  wipers, the held knobs and the E3D animations ran on the frame's delta, not the simulation's.
  Counting the simulated time of the failing runs gave a different number per run for the same
  script.
* **Fix:** a test steps the simulation itself, like a step debugger (inspired by #301):
  `MaszynaGutTest.step(count)` advances by `TICK` (1/30 s) per step, `wait_simulated_until()`
  steps until a condition holds or a limit in simulated seconds runs out (the limit from the
  vehicle's data, the margin one `TICK`), `SimulationRuntime` is not used in tests. Everything that
  moves with the simulation goes by the simulation's time: `SimulationClock` for the cab elements
  and the train sound system, `simulation_advanced` for the held knobs and
  `E3DRenderingServer`'s animations. Only loading a scenery keeps a real-time limit - as a hang
  guard, a failure when it runs out.
* **Rule:** a test advances simulated time by stepping it and waits for a condition with a limit in
  simulated seconds; never a real-time wait, a frame count or a raised simulation speed.


## 2026-10-07 A vehicle without its model held the loading screen 30 s

* **Symptom:** `test_zzz_scenery_scene_smoke` failed once its wait for the loading screen got a
  limit: the screen went down 31 s after the player got the EP07 of the fixture scenery. The test
  had waited 120 s without checking, so it never showed.
* **What proved it:** `[SceneryLoad] SURROUNDINGS 30.0 s` - `_build_surroundings()` ran into its
  `STREAMING_WAIT_TIME` with `area_is_ready()` true and `builds_get_pending_count()` 1. The
  fixture EP07 has no model: `_build_models()` failed to load it and ended in `_update_detail()`,
  which put the vehicle back into `pending_builds` - a load attempt every frame, and a pending
  build that never ends. In the game any vehicle whose `.e3d` is missing does the same.
* **Fix:** `RailVehicleRenderingServer` notes a model that could not be loaded (`model_missing`)
  and does not queue it again until another appearance is set.
* **Rule:** a build that failed is a result, not a pending build - an operation that cannot
  finish says so once instead of being retried.


## 2026-10-07 Test runs that did not exit

* **Symptom:** a headless GUT run passed all its tests and then did not exit, now and then and
  more often with several runs at once: `test_mmd_semantic_catalog`, `test_player_camera_server`,
  `test_driver_timetable_run`, `test_game_window` and others - each a batch's hang until its limit.
* **What proved it:** Godot run under gdb, interrupted 5 s after GUT's `Totals`: the main thread
  in `std::thread::join()` at shutdown, one thread of the scripts asleep on a condition variable.
  The one script thread at start-up is the `DebugMenu` autoload's (`debug_menu.gd`): it called
  `RenderingServer` (the adapter's name, the viewport) and wrote labels, with thread safety checks
  off. A call into the RenderingServer from another thread is answered through the main thread;
  a script that ends within a second stops the main loop first, the thread waits for an answer
  forever, and `_exit_tree()` waits for the thread (`wait_to_finish()`). In the game the same
  happens on a quit right after the start.
* **Fix:** no thread - the debug menu takes the information on the main thread in `_ready()`.
  The three scripts that hung went through 5 runs each clean.
* **Rule:** a script thread never calls a server that answers through the main thread, nor touches
  a node; whatever needs them runs on the main thread.


## 2026-10-07 Tests that ran at the speed an earlier script left

* **Symptom:** on CI `test_rail_vehicle_idle_pantograph_voltage_regression` (no wire voltage, the
  pantograph not even up) and `test_zzz_driver_hints_en57_2000_v1` (the master controller key did
  not take) failed, green alone; locally the suite in one process failed 80 tests more.
* **What proved it:** a bisection in CI's order in one process: the pantograph test went red only
  after `test_maszyna_environment_node` (with `test_maszyna_include_unload` between). That script,
  `test_weather_controls`, `test_simulation_clock` and `test_scenario_event_server` set
  `simulation_speed` to 20-1000 and set it back by `simulation_speed = ...` only: the clock's
  current speed stays where it was and closes on 1 over `speed_change_time`, so the scripts after
  ran at up to 100x. The local 80 came from the game directory: `UserSettings` finds the real game
  in `HOME` and uses it in place of the fixtures; with an empty `HOME` and `XDG_DATA_HOME` the
  run is CI's. There the EN57 failed alone too: a trace of the commands showed the player's
  `cab_activation_auto` at t=0.033 with `ra` coupled to nothing - `enter_vehicle()` seated the
  player once the vehicles had their simulation, before `_build_trainsets()` coupled the unit -
  so `SendCtrlToNext("CabActivisation")` never reached `s`, and `IncMainCtrl` on `s` refused
  without `CabActive` (Mover.cpp:2663).
* **Fix:** the four scripts set the speed back with `simulation_reset_speed()`;
  `MaszynaGutTest.after_all()` asserts every script leaves the simulation running at 1, so the
  one that does not goes red itself. `enter_vehicle()` seats the player on `scenery_loaded`, as
  `World.start_player()` does. The standing event test, green only at the leaked speed, waits the
  two steps its rerun takes.
* **Rule:** a script leaves the simulation's speed with `simulation_reset_speed()`, never by setting
  `simulation_speed` back; reproduce CI with an empty `HOME`; the player is seated once the scenery
  is loaded - a cab activated before the coupling never reaches the unit.


## 2026-10-08 Player initialization CI regression

* **Symptom:** CI reported 64 failing tests. Forty-seven vehicle start-up cases stopped at "the
  player in the cab", and the demo scenery smoke test had no player vehicle five seconds after
  loading.
* **What proved it:** every failure reached a valid, fully loaded vehicle but
  `PlayerServer.player_get_vehicle()` remained invalid. The shared start-up fixture instantiated
  `MaszynaPlayer` only after `MaszynaSceneryNode.scenery_loaded` had already fired, while other
  fixtures never connected that signal after `auto_start` was removed. `SceneryWorld` also passed
  an empty selection straight through instead of resolving it to the scenery's first vehicle.
* **Fix:** fixtures create and configure the player, connect `scenery_loaded`, and only then start
  the scenery load. `world.tscn` connects its own post-scenario `scenery_loaded` signal to the
  player, and `SceneryWorld` resolves an empty selection to the first vehicle before emitting it.
  Moving the cab camera now clears scene ownership before reparenting and does not preserve a
  global transform while its player is leaving the tree.
* **Rule:** a consumer of `scenery_loaded` exists and is connected before loading starts; removing
  a readiness fallback requires migrating every composition and test fixture to that signal path.


## 2026-10-09 Classes moved out of the project, hidden by a warm class cache

* **Symptom:** after the AI driver moved to the game, CI's run-tests failed with parse errors -
  `MaszynaLegacyDriverTrainset` and `MaszynaLegacyDriverTimetable` not found in
  `legacy/station/maszyna_legacy_station.gd`, `SunShaftsCompositorEffect` not found in
  `test_sun_shafts_compositor_effect.gd` - and a translation test expecting the game's "Skład".
  The same tests had passed locally; in the game, `test_maszyna_legacy_ai_driver.gd` used a helper
  class left in libmaszyna's test, and its 21 tests never ran while the run reported all green.
* **What proved it:** the local check ran in a copy whose `.godot/global_script_class_cache.cfg`
  still listed the moved classes; CI imports from no cache. GUT leaves a script that fails to parse
  out of the run without failing it, so the game's count simply lacked that file.
* **Fix:** the station (it works on the driver's trainset and timetable) and the sun shafts test
  went to the game, the core tests name the passengers load themselves
  (`MaszynaGutTest.PASSENGERS`), the translation test checks libmaszyna's catalogue only.
* **Rule:** after moving classes or files out, check from a fresh `.godot` (imported twice, as CI
  does) and parse every test script - `gut_cmdln.gd -gdir=res://tests/
  -gunit_test_name=<no such test>` runs none and reports every parse error - before trusting a
  green count.


## 2026-10-09 Port errors behind the magic numbers

* **Symptom:** none in the game reported - the values looked plausible. They came up only when the
  cleaning list's magic numbers (RC-081..RC-098) were to get a source reference each.
* **What proved it:** every literal was read against its line in `~/src/maszyna`. These places
  differ from the original: a Bare coupler adds `max_velocity` where `LoadFIZ_BuffCoupl` adds
  `Ftmax` (`Mover.cpp:10666`); the cab shake's `jolt_limit` 0.15 against 2.0 (`DynObj.h:839`);
  the track width 1.6 against 1.435 (`Track.h:205`); a switch blade timed by a duration instead
  of `fOffsetSpeed` (`Track.h:67`); doors at 112 V, which the Mover never powers
  (`Mover.cpp:8768`); `LightsDefPos` 0 against 1 (`MOVER.h:1701`); the tachometer without
  `MaxTachoSpeed` (`Train.cpp:8584`); the control pipe without `HiPP` (`Mover.cpp:10470`).
* **Fix:** the constants carry the reference and say where the value differs; the divergences are
  REQUIRED_CLEANING RC-124..RC-130 and `TODO.md`.
* **Rule:** a source reference is written only after reading the original's line - its value and
  the inputs of its formula. What differs is a port error to report, not a value to name.


## 2026-10-09 Python screens ended the game without a trace

* **Symptom:** a player on Windows (RTX 4080 SUPER, Steam MaSzyna, the game installed in another
  directory than MaSzyna) had the game close with no message and no entry in the Windows event
  log, right after taking over EP07-424; the build with `--no-python` ran without fault.
* **What proved it:** the `--verbose` `app.log` ends at `[PythonScreen] starting the interpreter`,
  with `run/flush_stdout_on_print=true` - every line is in the file the moment it is printed - and
  neither "cannot load" nor "missing from the library" before it: `LoadLibraryW()` and the symbols
  were fine, and the process ended in `Py_SetPythonHome()`/`Py_InitializeEx()` on the worker
  thread. A fatal error of CPython is a line on the C stderr of `python27.dll`'s own CRT, which a
  windowed process does not have, and `abort()`; Godot's crash handler covers the main thread
  only. Started the original's way, the same `python27.dll` and `python64/` run under wine:
  `PIL` imports and a screen script renders. The started interpreter differs from eu07.exe's in
  where it starts: eu07.exe sits in the game directory, which is its working directory and the
  first place Windows looks for DLLs, and CPython adds the executable's directory to `sys.path`.
  The player's actual error is not known yet.
* **Fix:** CPython runs in `maszyna-python-host`, a program of its own beside the library
  (`src/legacy/cabin/python_host/`), started in the game directory with it on the DLL search path,
  as eu07.exe is. When it ends, `PythonScreenServer` logs its stderr and exit code, emits
  `python_runtime_failed` once and leaves the screens blank; the scripts' tracebacks reach the log
  through a captured `sys.stderr` (PyInt.cpp:265).
* **Rule:** a runtime from the player's game directory runs out of the game's process, and what
  it says on its way out is read and logged - the game survives it and the next report carries
  the reason.


## 2026-10-09 A test that fails only after another script

* **Symptom:** `test_rail_vehicle_idle_orientation_regression.gd` failed with
  `Parameter "event" is null` (`ScenarioEventServer::event_free`) when run right after
  `test_scenario_event_server.gd`, and passed alone; the error was the same on the commit before.
* **What proved it:** the backtrace pointed at `MaszynaIncludeNode._free_owned_rids()` from its
  `NOTIFICATION_PREDELETE`, and GUT reported 2 unfreed children of `test_scenario_event_server.gd`.
  `test_a_passenger_stop_is_named_as_the_timetable_names_it` dropped the include node
  `_build_scenery()` returned and freed its event by hand. The node outlived the script, and when
  GUT freed it during the next script's test it freed the same event again - an engine error GUT
  pins on whichever test is running.
* **Fix:** the test keeps the include node and frees it, as the other scenery tests do; what the
  node built is freed only through it.
* **Rule:** a test frees what it built through the owner that built it, before it ends - never an
  owned RID by hand, never an owner left for GUT. An error of the freeing lands on the next
  script's test.

## 2026-10-10 An EN57 that "does not start" stood on its EP brake

* **Symptom:** reports#16 - after the main switch tripped before a platform and the train was
  stopped with the brake, the EN57 took no notch any more with the main switch closed again.
* **What proved it:** the report's snapshot showed every line-contactor condition clear on the
  motor car; a probe of the same unit (trip, EP brake, handle back to "drive", main switch,
  notch 1 held) closed the line contactors at once with 104 kN of tractive force and stood, the
  cylinders at 2.49 bar. FVel6's "drive" is `bh_RP` = 0 = `bh_EPN`, the EP neutral that holds the
  cylinders; only `bh_EPR` = -1 releases (`TFVel6::pos_table`, hamulce.cpp:34). The releaser does
  not touch the EP part. The player's log: EP brake at 179 s, then only "drive" and the releaser
  until 262 s, when one step down to -1 released the train. The original behaves the same.
* **Fix:** none in the physics. After the release at -1 the game's "Release train brakes" hint
  showed the "drive" key and was done only at the driving position, sending the player back to
  0 - the original checks RP too (driverhints.cpp:868). The game's hint now takes the EP
  releasing position on an EP brake (maszyna-reloaded#3).
* **Rule:** a unit with an EP handle that "does not move" is read on its cylinders first - an FVel6
  holds them at "drive" (0) and releases only at -1.


## 2026-10-10 Configuration that depended on the order of the FIZ sections

* **Symptom:** none reported. Porting REQUIRED_CLEANING RC-124 (a bare coupler built from
  `max_velocity` instead of `Ftmax`) showed that `Ftmax` is not there yet when the coupler is
  configured.
* **What proved it:** the original reads a FIZ line by line, and `LoadFIZ_BuffCoupl`
  (`Mover.cpp:10663-10672`) reads `Ftmax` before `LoadFIZ_Engine` has set it (`:11166`, `:11251`)
  - in the game's data `BuffCoupl.` comes before `Engine:` (`411d-025.fiz`: lines 7 and 22), so
  the original's bare coupler sees `Ftmax == 0`. The wrapper applies its components in the same
  order (`fiz_vehicle_builder.gd`, `context.parts`), so reading the Mover's `Ftmax` there
  repeats it. The same order decided `SpeedCtrl` (`EngineType` and `ScndCtrlPosNo` written by the
  engine and Cntrl.) and an EZT's default `IminLo`/`IminHi` (the controller saw 0 before the
  engine's Circuit: wrote them, and the engine then overwrote the defaults with its own zeros).
* **Fix:** `VehicleController::apply_configuration()` has a second pass: the controller's and
  every component's `apply_vehicle_config()`, run after every component applied its own
  configuration. The bare coupler takes the engine's `maximum_traction_force`, the speed
  control the engine's kind and the master controller's `second_position_count`, the controller
  the EZT thresholds the engine left at zero.
* **Rule:** a configuration value that depends on another component is applied in the second
  pass, from that component's properties - never from the order of the FIZ sections.


## 2026-10-10 A scripted component called its freed node

* **Symptom:** none reported. Sweeping REQUIRED_CLEANING RC-123 (scene-tree nodes holding
  pointers) showed a use-after-free reachable today.
* **What proved it:** `GenericVehicleComponentNode` put its component into the vehicle on
  `ENTER_TREE` with `set_script_owner(this)`, a `Node *`, and on `EXIT_TREE` only called
  `component->detach()` - which clears the component's controller and implementation but leaves
  it in `VehicleController::components`. `process_components()` and `get_state()` go on reaching
  `script_target()->call(...)`, the freed node. Taken out of the tree and put back ("Edit FIZ"),
  the node added a second component and its script ran twice.
* **Fix:** `VehicleController::remove_component()`; the node takes its component out of the vehicle
  it put it into (held by its RID) on `EXIT_TREE`, and the component keeps the node as an
  `ObjectID`. `VehicleController` also releases its components in the editor, so their
  `train_controller_node` back-pointer never outlives it.
* **Rule:** whatever a node puts into another object on entering the tree it takes out on
  leaving - detaching is not removing. A pointer kept in a member is judged by what it points at:
  owner, parent, children, singleton or backend, otherwise an `ObjectID` (`CODE_STYLE.md`).

## 2026-10-10 Windows streaming listed directories for every missing file

* **Symptom:** the Windows build (under wine, RTX 5050, warm cache) filled Drawinowo's
  surroundings in 30 s at 1-4 fps, with single model builds of 50-117 ms; the Linux build on the
  same machine filled it in 7 s with builds of 14 ms at most.
* **What proved it:** `perf` of the Windows process: a third of the main thread in wine's
  `NtQueryDirectoryFile`/`NtQueryAttributesFile`/`readdir64`. `WINEDEBUG=+file` over 10 s of the
  fill: ~29 000 directory listings and 1.44 million `FindNextFileW` on the main thread - whole
  `textures/` (764 entries), `dynamic/pkp/ep09_v1`, `406r_v1` and the game directory, listed for
  the candidates the callers try (`silence1.wav/.ogg/.flac` in the vehicle's directory, a `.mat`
  in four places, a model's two extensions in two directories). `MaszynaDataPath.resolve()` listed
  the directory of every candidate that was missing, twice (files, directories) - on Windows,
  whose filesystem ignores case, always in vain.
* **Fix:** `resolve()` tries the authored spelling and its lowercase form, and lists nothing. A
  scan of the data found two references that only the listing found on Linux:
  `przejazdy/plyty3_l.t3d` (`plyty3_L.e3d`) and `slupy_nn_400kv_*_atlas` (`400kV` on disk) of
  l053 and l204; they are given up.
* **Rule:** a path lookup costs a fixed number of existence checks; it never lists a directory to
  match a name letter case aside.

## 2026-10-10 An EN76 never raised its D car's pantograph

* **Symptom:** in Wrzosy, IC EIE8310 (`wrzosy_eie8310.scn`), nothing happened at Wolica after
  16:00: no route for the player's train, no shunting, the event queue holding only the station
  sounds' loop. ROJ43411 (EN76-001, `22WE_P4`) stood at its start for good.
* **What proved it:** a headless probe of the scenario in the game project (the AI registered as
  `game.gd` does - in `demo` no driver has an implementation, and a train given a starting
  velocity only rolls): everything after `final_2t849:event2` waited for the EN76 to cross that
  track, and its driver stood at "prepare the vehicle" with `engine_missing` = LINE_BREAKER. The
  A car (`PhysicalLayout=1`) had pantograph A up at 3448 V and its line breaker closed; the D car,
  powered too, has only pantograph B (`PhysicalLayout=2`), which was never raised, so its line
  breaker could not close - and the readiness counts every powered car (`Driver.cpp:6141-6144`).
  The driver asked only for the pantograph the pantograph unit has (the 2026-10-06 fix): the
  original asks for both, consist-wide (`Driver.cpp:2811-2813`, `OperatePantographValve()`
  defaults to `range_t::consist`), and takes a hint as done by the pantograph unit's **valve**
  (`Pantographs[end].valve.is_active`, `driverhints.cpp:269-310`), which a car without that
  pantograph has as well. The port tested the pantograph raised on the unit, so "raise B" never
  ended on a car without B - the reason the quirk was introduced.
* **Fix:** `RailVehicleEnginePowerSource.get_collector_pantograph_first/second_valve_active()`
  (the valve's `is_active`); the driver's pantograph hints end on them, both pantographs are asked
  for and the suggested setup applies to every vehicle, `CollectorsNo > 1` where the original
  reads it (`MaszynaLegacyDriverPantographs`, the game's `ai_driver/`). The quirk "Pantograph B of
  a vehicle with one" is gone. `test_zzz_driver_prepare_engine_elf.gd` (the EN76 cut from
  `ic8310_dekoracje.scm`): red before, green after.
* **Rule:** a hint ends on the state the original's hint reads, of the vehicle it reads it from -
  a valve is not a pantograph; a workaround for a hint that never ends is a sign of the wrong
  state read.

## 2026-10-10 An Elf that never gathered power under its driver

* **Symptom:** with its pantographs fixed (the entry above), the EN76 of Wrzosy's IC EIE8310 stood
  ready, its driver at "increase tractive force", the master controller at 4 ("add to 1") - and
  `eimic_real` at 0.01 for good.
* **What proved it:** a print in `MoverDriveUnit::process()` (removed): `MainCtrlPos` and
  `MainCtrlActualPos` 4 all the time, `LastRelayTime` back to 0 on every driver update (0.5 s),
  never reaching `InitialCtrlDelay` (1.0 s), so `CheckEIMIC()` case 2 added nothing past the first
  0.01. On every update the driver puts the controller to holding (3, `eimic > 0`,
  CheckTimeControllers(), Driver.cpp:4289) and back to driving (4, IncSpeedEIM(), Driver.cpp:3784).
  The original assigns `MainCtrlPos` - between two Mover steps it stays 4 and the relay time goes
  on. The port stepped it through the cab, `DecMainCtrl()`/`IncMainCtrl()`, which restart
  `LastRelayTime` (Mover.cpp:2361, 2558).
* **Fix:** `RailVehicleController.main_controller_set_position()`, a command; the driver puts an
  EIM controller of kind 1, 2 and 3 at its position where the original assigns it
  (`MaszynaLegacyDriverTraction.put_main_controller()`: IncSpeedEIM/DecSpeedEIM, IncBrakeEIM/
  DecBrakeEIM, CheckTimeControllers). `test_zzz_driver_prepare_engine_elf.gd`,
  `test_the_driver_told_to_go_moves_the_unit_off`: red before, green after.
* **Rule:** where the original's driver assigns a control, the port does not step it - a step is a
  Mover operation with side effects of its own (relay time, delays, messages to the trainset).

## 2026-10-10 The EP07's motor connectors stayed open

* **Symptom:** reports#22 (Wrzosy, IC EIE8310, EP07-1024): the player could not move the train;
  the controller went up and down with nothing happening. (The AI, given the cab, stood at a stop
  signal - the route waited for ROJ43411, see "An EN76 never raised its D car's pantograph".)
* **What proved it:** the report's `gameplay.log` has `motor_connectors_open true` at 1721 s with
  no release after it, and the `snapshot.json` `motor_connectors_open = True` with
  `line_contactor_closed = False`: `MotorConnectorsCheck()` refuses traction while `StLinSwitchOff`
  is set (Mover.cpp:6475-6487). The cab catalog declared `stlinoff_bt` a persistent toggle
  ("`_bt` not `_sw`"), but the original's `OnCommand_motorconnectorsopen` clears `StLinSwitchOff`
  on release unless `StLinSwitchType == "toggle"` (Train.cpp:5025-5055) - an impulse button by
  default. The FIZ parser also took only `MotorConnectors=impulse` as impulse.
* **Fix:** `RailVehicleSwitches` publishes `motor_connectors_switch_impulse` in the config;
  `stlinoff_bt` is monostable by it (true when absent); the parser takes every value but `toggle`
  as impulse. `test_train_switches.gd`, `test_mmd_semantic_catalog.gd`.
* **Rule:** a cab button springs back or stays as the original's handler decides, with the
  original's own test of the vehicle's switch type.
