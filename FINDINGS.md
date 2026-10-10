# Findings

These are rules left behind by root causes that already cost someone a measurement. Each rule
names the entry in `docs/findings-archive.md` that has the symptom, the proof and the fix; code
comments cite those entries by date and title. Check the matching area before diagnosing
anything. Open work belongs in `TODO.md`.

## Diagnosing
* A local clang-tidy older than CI's (`LLVM_VERSION` in `clang-tidy.yml`) passes what CI fails:
  checks added since are unknown to it. Run CI's major version (`pip install clang-tidy==22.*`).
  *(10-01 style-check red behind a green local check)*
* Measure the data before reading the code, and after two failed hypotheses read off the code,
  stop reading and print. *(09-24 pantograph lost the wire; 09-23 four guesses before one print)*
* Split frame time into CPU and GPU before any performance hypothesis, and confirm the adapter,
  power profile and build flags. *(09-22 GPU that never woke up)*
* Measure by phase before restructuring: the visible loop is rarely the cost. *(09-22 sound
  system's per-frame cost)*
* Before proposing threads, split the main thread by phase with `perf` on a profiling build; a
  query that depends only on placements is cached against a change serial of what it read.
  *(10-06 parked vehicles scanned for neighbours in every sub-step)*
* A streaming budget is checked between pieces: measure the longest single piece per owner
  (`owner_max_msec`), not the total. A real-renderer run goes through `gamescope --backend
  headless` - `xvfb-run` shows Godot on a Wayland desktop and has no DRI3 for Vulkan. *(10-02
  streaming hitches)*
* When a build or a code change "has no effect", prove that the binary running is the one built
  (mtime, md5), then read the cache file (`strings`, `.hash`) before any other hypothesis.
  *(09-21 release never unpacked; 09-20 low-poly interior)*
* When a cache "does not work", load an entry and run its own validity check before suspecting
  invalidation. *(09-23 game dir ".")*
* "One action late" means a read of a snapshot taken before the write. Look for the cache
  between them. *(09-23 cab one keypress late)*
* Crashes at changing places are one cause: lay the backtraces side by side and find the frame
  they share - in the engine's binary too - and look at the core's other threads, before reading
  our code. *(10-03 the editor ran the scenario)*
* An **abort**: read the engine's error lines first. "Already initialized RID" means concurrency,
  "invalid RID" means a double free. Use `coredumpctl debug`, not Godot's dump, and
  `addr2line -f -C -e <.so>` on the extension's hex frames. *(09-22 RID allocator; 09-22
  uninitialised pointer)*
* A hang costs nothing to read: `/proc/<pid>/task/*/wchan`, and `kill -ABRT` for a symbolised
  core. A shipped build keeps its symbol table. *(09-24 shipped library had no symbols)*
* When a fix is being reinvented, `git log -S` the moved code and read the commit that made it
  first. *(09-24 simulation stepped after readers)*
* Many copies of one sound: the defect is phase, not level. Look for a start offset first, and
  prove it on every path a clip starts by (automation, timeline, sustain). *(09-24 trainset
  ringing)*
* Prove a sound fix on the stream class the game plays (`MaszynaAudioStream` reads its file only on
  the first playback), and give every emitter its own pitch factor as the original does.
  *(10-01 start offset that never reached the game)*
* No delay-based effect (reverb, echo, Haas stereo widening) on the bus of the vehicles' sounds:
  it combs many copies of one recording. *(10-01 a phaser on the Exterior bus)*
* Prove a fix to a value by printing it where it is used, not where it is set. A later line
  can overwrite it. *(09-24 trainset ringing, the fix that did not work)*
* Before changing a sound constant, dump the whole built bank (`track.volume_db` of every clip).
  *(09-21 +38 dB SfxTrack)*
* A red test that survives many unrelated commits is no evidence of the commit that turned it red.
  *(09-23 loco with nobody in the cab)*
* A cab build runs on every cab change: measure it on the largest real cab, not on a fixture
  without models. Pairwise merging that measures every pair again on each merge is O(P^3).
  *(10-06 a cab change in the 36WEa froze the game)*

## Porting the original engine
* A configuration value that depends on another component (a bare coupler on the engine's
  `Ftmax`, `SpeedCtrl` on the engine kind and `SCPN`) is applied in the second pass,
  `apply_vehicle_config()`, from the other component's properties - never from what the FIZ
  section order happened to write into the Mover first. *(10-10 configuration that depended on
  the order of the FIZ sections)*
* A vehicle's backend is configured once, in `TDynamicObject::Init()`'s order: every FIZ value, the
  load, then `CheckLocomotiveParameters()` once - nothing configured after it, or it undoes what it
  derived. A test builds a vehicle through its description, as the game does, not by adding
  components to a built one. *(10-09 Mover configured five times, spring brake released)*
* Port the whole `LoadFIZ_*` / loader function: derived counts, container sizes, fallbacks, and
  the state it sets at the end. A struct default is what a vehicle **without** that section gets.
  *(09-24 NaN forces; 09-24 spring brake)*
* What a scenery order asks of the couplers is masked by what the pair can join: the original's
  `Attach()` sets the asked couplings whatever `AllowedFlag` says, so its `UpdateConnect()` never
  waits; a port that joins only allowed flags must not wait for the rest. *(10-09 SN61 coupled for
  ever on `Shunt -3 -99`)*
* A Mover field the original keeps as the FIZ text (`BrakeValveParams`: an ESt3's `PZZ`, `AL2`,
  `-s216`, `-ED` relays) is passed as text, not only decoded into its enum. *(10-06 SN61: the
  independent brake did not brake, the engine stopped after the start)*
* One FIZ key can feed several Mover fields. Grep every `extract_value(..., "Key", ...)`.
  *(09-24 line breaker opened)*
* A value from a scenery token carries the unit the loader gives it right after parsing. Read the
  lines after `>>`. *(09-24 wire 100x too resistive)*
* A Mover method nothing in `Mover.cpp` calls is driven from DynObj.cpp/Train.cpp. Grep for its
  callers. *(09-24 induction motor never pulled)*
* The type of a train (passenger, goods) comes from `RailVehicleServer.trainset_get_type()`,
  never from the brakes or a vehicle's G/P setting. *(10-06 Os33733 left Zagórz before its
  departure)*
* The vendored engine assumed its own `stdafx.h`. Check overload-sensitive calls (`abs`, `min`,
  `max`) when something numeric just stops. *(09-24 C's abs())*
* Before porting a field that looks like data, read what the backend does with its **name**
  (`pantstate`). *(09-24 what a "load" is)*
* A ported tolerance carries the original's value, never a rounder "safer" one. A tolerance that
  papers over data becomes load-bearing once something trusts the structure. *(09-20 double
  slips; 09-24 pantograph)*
* A component's default is what a vehicle **without** the FIZ key gets - check it against the
  original's default, not against a plausible value. *(09-28 `permit_list` [0, 0, 0])*
* A source reference is written only after reading the original's line: its value, and the
  inputs the formula takes (`Ftmax`, not `Vmax`). What differs is reported as a port error, never
  named over. *(10-09 port errors behind the magic numbers)*
* A ported formula carries the original's frame with it: when our loft or basis maps an axis the
  other way (profile x to the left, not `RenderLoft`'s right), every angle in that plane flips.
  Compare world coordinates of both sides, not the formulas. *(09-28 cant reversed)*
* Mirroring a loft's cross vector flips its winding: with culling off the face shows lit from
  below. A joint between two meshes takes each side's section from the code that built that mesh.
  *(09-28 switch trackbed dark, ballast wings)*
* Geometry the original does not have is a workaround until measured: a stitch lifted 1 cm
  over both beds fought them in depth, and closed gaps of 2-5 cm. *(09-28 trackbed stitches
  flickered)*
* Before caching or dropping a call as redundant, open the callee and the original line cited
  above it. *(09-20 coupled wagons drifting)*
* A tuning factor with no counterpart in the original scales the data's errors with the data.
  *(09-20 blotchy ground)*
* A driver's readiness is tested on every vehicle under control (`IsAnyLineBreakerOpen`), in its
  reset and in its check alike, and only on devices a car has - a car without an engine has no line
  breaker to be open; a hint is done by the switch of any car that has the device, not by the
  device running. *(10-06 Vehicle not ready with nothing
  missing; "Switch on compressor" with the compressor on)*
* A vehicle that is not driven is not simulated (`CabActive`/`PhysicActivation`). *(09-23 loco
  with nobody in the cab)*
* A parser of the original's data mirrors its tolerance: skip what is not known, and let every
  section header close the open table. *(09-25 E186 cab half built)*
* A scenery's `origin` is a stack of sums, not of values: an `origin` inside an `origin` adds to
  it. *(10-03 switch ballast at the origin)*
* A file named by the data is looked up where the original looks it up, in its order - not under
  the one root most of the data uses. *(10-03 Sandomierz without its platform)*
* A list in the data ends where the original's loader ends it - at its keywords, not at the end
  of the node. *(10-03 l107's factory turned by 80 degrees)*
* A workaround in a `Mover*` call is a sign that a FIZ key is not ported yet. Before keeping one,
  read the key's default in `LoadFIZ_*`. *(09-25 pantographs raised only with the master valve forced)*
* A Mover field every engine type computes (EngineVoltage) is published by the part they all
  have, not by the first subclass that needed it. *(09-29 the E186 screen showed no line voltage)*
* Grep every `extract_value(..., "Key")` of LoadFIZ_Cntrl against the parser before calling the
  section ported. *(09-29 MainInitTime was never loaded; 10-06 EN57 braked weakly - its trailers'
  wheels locked)*
* The player's driver (DriverServer) sits in the player's cab after every cab change, of the same
  vehicle too; which pantographs a vehicle has is its `PhysicalLayout`, not `CollectorsNo`.
  *(10-06 36WEa: "deactivate the cab" and "raise pantograph A" in the C car's cab)*
* A `Cntrl.` key belongs to the vehicle, not to one engine type. Check that it reaches the Mover
  for every `EngineType` that uses it. *(09-25 SU46 would not release its train)*
* Most `OnCommand_*` work without their gauge; those that refuse (`ggX.SubModel == nullptr`) are
  `requires_gauge` in `MmdSemanticCatalog`. A control only in the catalog is dead in a cab that does
  not model it. *(09-20 E186 Ctrl+J, 10-05)*
* A pantograph at 0 V has three causes (no wire in reach, dead wire, no contact). Report them
  separately. *(09-24 pantograph)*
* A wire chain that shares no end with a powered one is fed across the overlap by the section-ends
  pass (Traction.cpp:858). Check the span's chain and its feed before the contact. *(09-29 td.scn's
  second track dead)*
* Survey the data (a histogram) before mapping a parameter or trusting a "supported" list, and
  read the geometry drawn for a glow before adding a tuning constant. *(09-21 spot cone, street
  lamp; 09-20 missing shaders)*
* When porting a contract consumed by scripts nobody here maintains, run the real scripts against
  it. *(09-24 Python cab screens)*
* Check sun-altitude thresholds against a winter day. *(09-20 orange fog)*
* A section is parsed for every `EngineType` that has it, and a parser never reaches its engine
  node by a cast to one engine class - a failed cast drops the section without a word. Check each
  `EngineType` against its `LoadFIZ_*` case and its `readMPT*()`. *(09-27 SA134 without a gearbox)*
* A property's default is the Mover's default, not a value that looks sensible. *(09-27 SA134
  without a gearbox)*
* A loader's derived fields are part of the port: the fields it sets from the ones it read
  (`Imin = IminLo`). *(09-27 automatic start without thresholds)*
* A loop that steps a control until it gets somewhere ends on "did not move", never on "is not
  there yet" - the vehicle may refuse the step. *(09-27 the driver's update hung on a refused
  controller)*
* "Zero" of a master controller is its no-power position (`MainCtrlPowerPos()`), not position 0:
  below it a universal controller brakes. *(09-27 the SM42 stood braked)*
* A field goes where the original's loader reads it, not where its first consumer is (MCPN is
  every vehicle's, not the engine's). *(09-27 a control car had no controller)*
* A FIZ section is applied whatever the order the file gives it in: `Cntrl.` may follow
  `Engine:` (EN57 keeps it in the brake include). *(09-27 EN57 without a master controller)*
* The game data is written for Windows and `cParser`: a quoted text is one token without its
  quotes. Keep a file name's authored spelling and resolve it exactly, then lowercase - never
  lowercase the token before the first lookup, and never list a directory to match a name letter
  case aside. *(09-28 timetable screens without their background; 10-02 uppercase 2M62 files
  reported missing; 10-10 Windows streaming listed directories)*
* A vehicle's MMD is read as an include with `(p1)` name, `(p2)` type, `(p3)` skin
  (`DynObj.cpp:5260`); a model named `none` is a missing parameter. *(10-02 SN61 drawn without
  its body)*
* The player takes a vehicle only once the scenery is loaded: a cab activated before the
  trainset is coupled reaches no other car (`SendCtrlToNext`). *(09-28 ED72 motor cars dead)*
* A vehicle may have several components of one type (two couplers): capture and keep them all,
  never one per type. *(09-28 ED72 tore apart on the first pull)*
* A coupled controller (`CoupledCtrl`) goes on into the field shunt: the cab's range and position
  are main + shunt (Train.cpp:985, 9410). *(09-28 ED72 stuck at 36-43 km/h)*
* A property default is the original's default for the absent key (`TCoupling::PowerFlag` is
  24V|110V, not 0) - a wrong one breaks every vehicle that omits the key. *(09-28 ED72 dead)*

* A shadow's normal bias is texel x `shadow_normal_bias` per cascade: compute it against the
  thinnest caster before tuning; two shadowed directional lights halve the atlas. *(09-27 thin
  station objects lost their sun shadows)*
* A gap the vertices do not show is a cut-out: check the texture's alpha where the UVs land. The
  trackbed's texture length is its material's `size:`, not the track's tex_length. *(09-28 the
  trackbed hung over the terrain)*
* A ground-level surface lit at a grazing angle shadows itself: take it out of the casters rather
  than raise the light's bias, and render the spot without shadows before blaming geometry.
  *(09-28 a gap between the trackbed and the terrain)*
* A RenderingServer light starts with the server's defaults, not a node's: diff the whole
  `Light3D` constructor against `_light_initialize()` - `shadow_blur` 0 zeroes a spot's depth
  bias. *(09-28 street lamps shadowed their own pool)*
* A blurry texture: compare its DDS size with the limit it loaded under before blaming mipmaps -
  the cab has its own limit, as in the original. *(09-29 blurry cab gauges)*

* A scenery model does not hide its `*_on` submodels - only a vehicle does (Model3d.cpp:2221);
  dump the model's tree before calling a light missing. *(09-29 shunting signal dwarfs never lit)*

* "The AI can drive it, the player cannot": log the AI's vehicle commands and replay them on the
  player's path - what the AI sends and the cab cannot is the gap. *(09-28 ST45 FuelStart)*

* A FIZ key given twice counts with its first value (`extract_value`'s `find()`); a dictionary
  that keeps the last one made BR285's `Vadd` 0 and its traction force 0/0. *(09-30 BR285 NaN)*
* A key is the original's binding (`driverkeyboardinput.cpp`); a key of our own only for an action
  the original leaves unbound - never move the original's keys around. *(10-06 the line breaker's
  keys took the rear motor blowers' Shift+M)*

* An EP handle that "does not move" the train: read the brake cylinders first - an FVel6 holds
  them at "drive" (0, the EP neutral) and releases only at -1 (hamulce.cpp:34). *(10-10 an EN57
  that "does not start" stood on its EP brake)*

* A driver's hint ends on the state the original's hint reads, of the same vehicle: a pantograph
  is "raised" when the pantograph unit's valve of it works (`driverhints.cpp:269-310`), on a car
  without that pantograph too. *(10-10 an EN76 never raised its D car's pantograph)*
* Where the original's driver assigns a control (`MainCtrlPos = 4`), the port does not step it: a
  step (`IncMainCtrl()`) restarts the relay time an Elf waits out. *(10-10 an Elf that never
  gathered power under its driver)*
* A cab button springs back or stays as the original's handler decides it, with the original's
  test (`StLinSwitchType != "toggle"` is impulse, Train.cpp:5045) - never a catalog guess from the
  `_bt`/`_sw` suffix. *(10-10 the EP07's motor connectors stayed open)*

## State, ownership, events
* An operation somebody awaits is done only when everything its waiter relies on is; a signal
  relayed after an `await` arrives after the call that caused it returned. *(10-03 the loading
  screen faded onto a world not streamed yet)*
* A thing enters the world with its first position, not with its construction: drawn before it
  has a place, it is drawn in the wrong one. *(10-03 a scenery's vehicles drawn at the origin)*
* A default changed for one owner of a field is checked against every other writer of it, and
  the tests of all of them are run. *(10-03 hand-assembled vehicles stopped animating)*
* A hot path takes a vehicle's component once and calls its getters; the state dump is rebuilt
  after every step or command and the config dump on every call, so one value read from either
  costs the whole dictionary. *(09-30 the sound system's dump per frame)*
* An action that needs two things is spent only when both exist: a call with an invalid handle is
  ignored silently, and the flag that said "still to do" is gone. *(09-30 the vehicles stood off
  their tracks in the editor)*
* A geometric value nobody publishes reads as zero, not as missing, and zero makes two things
  identical - grep for assignments to an exported property before trusting it is filled, and treat
  "both halves report the same number" as the signature. *(09-27 both pantographs at the origin;
  09-23 bogie pivot spacing of zero)*
* A value composed from two inputs that land in either order is published by **every** event that
  changes an input; one event plus a consumed dirty flag loses the race silently. *(09-27 both
  pantographs at the origin)*
* Removing an exported property needs a grep for its **reads**, not only its assignments. *(09-27
  both pantographs at the origin)*
* A command the original sends along the couplers (`SendCtrlToNext`) is sent only once the
  trainset is coupled: couple at endtrainset, after the vehicles stand on their tracks, then give
  the driver its orders. A cab switched on before that leaves the unit's other cabs inactive.
  *(09-27 EN57 vented its pipe from the rear cab)*
* Configuration never sets what the original switches at run time: the alerter is enabled by the
  cab's activation, not by a component's `enabled`. *(09-27 EN57 vented its pipe from the rear
  cab)*
* Every timer of the simulated train, the cab's relays included, runs on the simulation clock
  (`SimulationServer.simulation_advanced`), never on the frame. *(09-27 the cab's relays ran on real
  time)*
* One piece of state, one writer. Two writers that both look correct disagree only where the
  geometry shows it. *(09-24 parked vehicle jumping)*
* A getter never changes state. A value that depends on how often it is read stays invisible until
  a second reader appears. *(09-22 reading state changed it)*
* A cache keyed on a tick is correct only while the tick is its only writer. Write that premise
  down. *(09-23 cab one keypress late)*
* A config property and a state key of the same name are different when the backend writes the
  field. Grep for the assignment. *(09-22 config vs state)*
* A state/config key is data, not a method name (grep for `["get_`). `Dictionary.get(key,
  default)` hides typos, so test that the key is present. *(09-23 pivot spacing zero)*
* Config-derived values are recomputed on the config event (`VehicleServer.vehicle_config_changed`),
  never by a retry flag or a "moved" gate. *(09-23 bogies never placed)*
* A setter applies what it is named after, not "everything the instance knows". *(09-23 cab
  backlight blinking)*
* Put a state key on the part that owns the concept. *(09-21 diesel_max_rpm)*
* A feature parked on a node vanishes when an instancer without nodes appears, so the owning
  server subscribes to the events itself. *(09-24 switch blades; 09-21 scenery unlit)*
* Readiness describes built content for a specific camera revision, not an empty queue.
  *(09-20 streaming from menu camera)*
* A vehicle is configured once, before anything can see it: a controller given after it entered the
  tree restarts it and clears every coupling made meanwhile. *(09-30 couplings undone by the second
  configuration)*
* A trainset placed again in another order lets go of its old pairs first, or it closes into a ring
  and every walk along it never ends. *(09-30 reordering a trainset hung the editor)*
* A component exists only when the FIZ describes it, and the controller fills only its own state:
  what a vehicle may or may not have - the master controller, the engine, the low voltage, the
  radio - fills its keys from its component. *(10-04 a wagon's dump with a locomotive's state)*
* A submodel is drawn in one pass, the one its flags say - 0x20, or a translucent replaceable
  skin (bits 1, 2, 4, 8), in the alpha pass - and the optimized E3D instancer never uses the alpha
  pass. Port the original's rule before inventing a mechanism. *(10-04 SM42's windows missing)*
* What is drawn for a view is told to the drawing by the owner of the view (the player), never by
  the cab layer: `CabinSystem` knows no view, and hid the low-poly cab from outside too. *(10-04
  SM42's windows missing)*
* A cab (the original's TTrain) is at work only for a driven vehicle: its logic is attached on
  `DriverServer.vehicle_driven_changed`, never to every vehicle. *(09-29 every vehicle's cab ran
  each step)*

* Outside a vehicle's composition an action on it is `vehicle_send_command()`, never the
  controller's method - a direct call leaves the state dump stale until the next step and is
  announced to nobody. *(10-05 cab change in RailVehicleServer)*

## Godot / GDExtension
* A compute effect never samples the texture it writes in the same dispatch: D3D12 refuses the
  command list (`Close` E_INVALIDARG) and removes the device - every allocation after fails with
  `0x887a0005`. Copy the source first. *(10-06 crash at 0% loading on D3D12)*
* Godot measures a visibility range to the centre of the instance's box, the original from the
  model's origin for every submodel: an E3D submodel's box is centred on the model's origin
  (`E3DInstanceBackend::_visibility_aabb()`), or two LODs with meshes of their own centres leave a
  gap around their common bound. *(10-04 34WE's body missing at 80 m)*
* An extension that creates or frees objects off the main thread is not `reloadable`: Godot
  tracks a reloadable extension's instances in a set it does not lock (`_track_instance()`), in
  the editor only - the editor crashed in the engine at ever different places of ours, the game
  never. *(10-03 the editor ran the scenario)*
* A key is the project's input action, matched exactly (`is_action_pressed(a, echo, true)`): a
  loose match takes Alt+Enter for Enter. *(10-01 Alt+Enter loaded a scenery)*
* Memory that grows with a flat object count and no leak reported at exit is a referenced
  container: diff two jemalloc heap dumps before reading code, and suspect the binding's value
  types too. *(09-27 every rebuilt state dump stayed in memory)*
* A node in the scene tree keeps no pointer to another object: nodes as `ObjectID`, the vehicle by
  RID. *(09-30 "Edit FIZ" aborted the editor)*
* A `RefCounted` crosses a binding as `Ref<>`: a raw `T*` returned to GDScript takes a reference
  away and frees it. *(09-30 a raw pointer returned to GDScript freed the vehicle)*
* A `Resource`'s setter stores data; joining, registering and connecting happen where the object
  becomes live - loading a resource calls every setter. Its server handle is `_get_rid()`, never a
  `get_rid()` of its own. *(09-30 a stored vehicle description came up half a vehicle)*
* A builder hands the servers what it built by the node's handle, never through the node's exported
  properties - a script subclass of a native node saves them with the scene. *(09-30 a builder's
  paths saved into the scene)*
* A native node a script may subclass does its lifecycle work in `_notification()`: a script's
  `_enter_tree()`/`_ready()` replace the extension's. *(09-30 a script subclass shadows the native
  lifecycle)*
* Code resumed by `await` of a signal runs inside the emission: it cannot free the emitter ("Object
  is locked"). *(09-30 a test ran on the stack of the signal it awaited)*
* A node taken out of the tree and put back ("Edit FIZ") runs `_enter_tree()`/`_exit_tree()` again
  but `_ready()` once: subscribe where you unsubscribe. *(09-30 "Edit FIZ" disconnected what was
  never connected)*
* `SceneTree.process_frame` is emitted **before** every node's `_process` (measured, Godot 4.7.2):
  a C++ singleton on it steps ahead of every reader, no node needed. A node added to the root from
  an `_enter_tree()` of the main scene fails (`add_child()`, root busy). *(09-30 the simulation
  clock never ticked; corrects 09-24)*
* The Mover measures couplers from "the last refresh plus ten times the movement since": refresh
  locations and neighbours every physics sub-step, never once a frame, or the result depends on
  the frame rate. *(09-27 couplers stiffened by a long frame)*
* What the AI remembers of the tracks belongs to one way of driving: a turn or a takeover from a
  player starts it afresh. *(09-27 the AI stood at a clear signal)*
* A point of the route is passed when the train has driven up to it (distance counter), never
  because it is no longer read - a thrown switch changes the route standing. *(09-29 a thrown
  switch held the train at a clear signal)*
* A signal a train ignores (a Tm at stop) is ignored behind it as much as ahead; an `if ... else if`
  of the original stays exclusive in the port. *(09-29 a train held by a Tm at stop it had passed)*
* A control showing the vehicle's state never acts on it (a setter that acts, `button_pressed`
  that emits `toggled`); the driver never sends a command the vehicle does not have. *(09-29 a cab
  built on a running train lowered its pantograph)*
* A `.scn` key may repeat (`event2` twice): check the original's loader for a list before keeping
  a key in a Dictionary. *(09-29 a goods train left past its exit signal)*
* An event the original clears once passed (a signal at proceed, `Point.Clear()`) is cleared in the
  port - kept, it holds on what it shows later. *(09-29 a signal closing behind the train braked
  it hard)*
* A signal the front has reached is read again on every update while it is in the table
  (`TableUpdateEvent()`): one taken once on passing holds what it showed then. *(10-06 the eszelon
  stood at a dwarf that had opened)*
* A player's cab change and reverser turn the driver (`DirectionChange()`, Train.cpp:2670-2842,
  10343): a driver that keeps its own way reads the tracks behind the player. *(10-06 the eszelon
  turned back to an open dwarf)*
* Hook what the original does after a step to that step's event: the driver turned on the cab
  change's first event read the cab left still active. *(10-06 one cab change late)*
* A cab's controls act on the end of the cab the player sits in (`cab_to_end()`, CabOccupied),
  never on the active cab: a cab switched off is still the one worked. *(10-06 a light switch lit the
  other end)*
* The vehicle's supply voltage is held through a loss of up to 0.2 s (NoVoltTime), and the driver's
  readiness is tested against the consist on every update (a line breaker tripped while driving).
  *(09-29 EP07 rolled out of Markowo without power)*
* `lerpf()` does not return its end exactly, `std::lerp` does: a port whose result is compared
  exactly takes the end as it is. *(09-29 the driving aid flickered)*
* A model rebuilt is a view change: nothing the simulation reads lives in the node that draws it
  (a pantograph's raise), and a rebuild resets nothing. *(09-29 a model rebuilt dropped the
  vehicle's voltage)*
* `instance_set_submodel_visible(true)` overrides a dynamic model's `dynamic_hidden` default;
  pose and material settings do not imply visibility. *(09-30 coupled vehicles lost their
  couplers)*
* A bilateral detach that clears both backend links retains the former neighbour until both
  owners have announced their state change; do not invalidate a rendering cache to compensate
  for the missing event. *(09-30 recoupled wagon kept stale coupler state)*
* A cab control is its mesh and every mesh under it (a handle), unless another control lies under
  it - then it is a panel and its own mesh only. Decided by the model's tree, never by the widget
  class or the cab. *(09-29 the E186 screen's OP1/OP2 turned its page off; 09-30 its op12 still
  did; 09-30 EP07's brake valve and reverser handles could not be grabbed)*
* Only an opaque submodel hides a cab control from the mouse - a translucent one is not in the
  original's pick pass (`opengl33renderer.cpp:1208`). *(09-30 E186's spring brake release could
  not be clicked through its glass cap)*
* A control whose value lives only in the cab (`CabinState`) shows that value when it is built; a
  cab rebuilt resets nothing. *(09-29 a rebuilt cab showed the E186 screen's page button off)*
* Who drives a vehicle is kept by the vehicle, not by its driver: a player may take the cab before
  the driver exists. *(09-27 the AI drove the cab the player started in)*
* Whoever attaches something shared per vehicle takes away only what it attached - another owner
  may have replaced it meanwhile. *(09-27 the AI stood still in the cab the player left)*
* Simulated time has one clock, `SimulationServer`'s: read `simulation_get_time()` or take
  `simulation_advanced(seconds)`, never a `delta * simulation_speed` of your own. *(09-27 three
  clocks)*
* A C++ class under an existing GDScript subclass keeps its lifecycle in `_notification()`, never
  in `_ready()`/`_process()`. A script shadows a native method only for `call()` callers.
  *(09-23 subclass replaced _ready())*
* Exposed properties make getters reachable before `ENTER_TREE` and during `pack()`. Raw pointer
  members are `= nullptr` at declaration. *(09-22 uninitialised pointer)*
* A typed `X::get_instance()` removes the string, not the null. Guard it, especially in handlers
  run from teardown. *(09-22 unguarded singleton)*
* Porting an autoload: enums flatten, no inner classes, no constructor arguments, no float/RID
  constants, packed arrays. *(09-22 porting an autoload)*
* Never pass a bare `[]`/`{}` to a typed collection parameter. The call silently never happens.
  *(09-22 vehicle strip)*
* Probe a screen by running it as a scene. `--script` has no autoloads. *(09-22 vehicle strip;
  09-22 RID allocator)*
* A RenderingServer RID inherits none of its node's defaults. Set every parameter the node's
  constructor sets, and place particles through the instance transform. *(09-21 RenderingServer
  light; 09-21 smoke at origin)*
* A full-screen pass that reads `hint_screen_texture` only adds (`blend_add`): the copy is taken
  before translucent geometry, and writing it back erases it. *(09-30 the torch put out the
  signals)*
* `color`, `color_initial_ramp` and `amount_ratio` reach particles already in the air. Only the
  emission itself affects new ones. *(09-21 plume cut off)*
* An emitter whose rate its owner drives spawns nothing until the owner has set it - a default
  rate spawns before the first tick, and emits on a culled system burst when it comes into view.
  *(09-28 puff from a diesel that is off)*
* A valid RID does not mean its `Node3D` is in the tree. *(09-20 E3D node leaving tree)*
* The sky and the geometry are fogged by different parameters. An opacity summed from two sources
  is summed where it is set, and unclamped values extrapolate. *(09-21 fogged sky)*
* An `EditorImportPlugin`'s save extension changes with `_get_resource_type()`. Godot reimports
  on md5, not mtime. *(09-24 .fiz stopped importing)*
* `global_script_class_cache.cfg` is updated only by an editor scan. Run `--import`. *(09-20
  new class_name; 09-22 sfx tick)*
* A node-typed property written into a `.tscn` by hand needs `node_paths=PackedStringArray(...)`
  in the node's header; without it the `NodePath` is not resolved, the property stays empty and
  the next save from the editor drops it. *(09-26 semaphore lost its model)*
* The editor's scene tabs share one `World3D` and switching a tab takes the scene out of the
  tree: what a node made in the world's scenario stays drawn on every tab. Out of the world it
  is moved out (scenario `RID()`), as `VisualInstance3D` does - never freed and made again, its
  RIDs are held elsewhere - and freed with the node. *(10-03 a scenery's vehicles drawn on every
  editor tab)*
* A new catalogue in the same locale re-translates nothing: `set_locale()` of an unchanged locale
  and `add_translation()`/`remove_translation()` send no `NOTIFICATION_TRANSLATION_CHANGED`.
  The code that swaps it notifies the main loop. *(09-25 catalogue swapped, UI unchanged)*

* The Mover's train brake handle has three positions and only `BrakeLevelSet()` moves them
  together, comparing with `fBrakeCtrlPos`: a second setup leaves `BrakeCtrlPosR` at lap. A pipe
  that will not charge - read `dpMainValve` first. *(09-26 FV4a handle left at lap)*
* Setting a `Range`'s `min_value`/`max_value`/`step` clamps its value and emits `value_changed`: a
  control is only shown a value, and saving happens on an explicit apply of what differs from what
  was shown. *(10-04 settings saved their minimums)*
* A runtime loaded from the player's game directory (CPython 2.7) runs in a process of its own,
  never in the game's: its fatal error is `abort()` on a stderr nobody sees, and Godot's crash
  handler covers only the main thread. The host's stderr and exit code go to the log.
  *(10-09 Python screens ended the game without a trace)*

## Threads and teardown
* Every sink of parsed geometry - a subscene's too - writes to a directory with a limit, and hands
  its chunks on as files, never as a copy of everything. *(10-02 a subscene kept its triangles)*
* An include with parameters (a placed object, any size) is parsed in place, never as a task - a
  task costs a context kept until its parent's merge; after a parse on many workers the allocator's
  free memory is given back (`ProcessMemory.release_unused()`). *(10-01 a task per include; 10-02 a
  queue task for every placed object over 16 KB)*
* A parse that repeats an include in world space reduces each node to its final form as it reads
  it (`SceneryTrianglesSink`), with a bound on memory; nothing is kept per include until the end.
  *(10-01 the parse kept every include's triangles)*
* Nothing on a worker reads back from the RenderingServer (saving a mesh does - off the main thread
  it waits for the main thread), and nothing that loads or saves runs on the `WorkerThreadPool`,
  which the engine's loading waits for; ours is `WorkerTaskQueue`. Headless never shows it -
  reproduce on a real renderer (`xvfb-run ... --rendering-driver opengl3`). *(10-02 Infrastructure
  hung with parallel preloads)*
* A grid cut keeps a cell only when a piece with area lands in it: a triangle touching a border
  made an empty chunk, cached and failing as a mesh when streamed (`array_len == 0`). *(10-02 an
  empty terrain chunk)*
* What a streamed piece loads is held by its build and let go by its clear
  (`ResourceLazyLoader`, with `maszyna/resources/lazy_loading`; off - the default - it is loaded
  at registration and kept); a memo that never evicts grows with the session, and a cache file
  holds data, never a mesh. *(10-01 a scenery's whole terrain kept in memory)*
* An object is never taken back from its `ObjectID` on another thread than the one that may drop
  its last reference: while it is destroyed it is still in `ObjectDB`, and `cast_to` on it
  crashes. A copy shared across threads is held by a `Ref` its owner keeps. *(10-04 segfault in
  the streaming's preload)*
* What points into another owner's memory asks for that owner by `ObjectID`, not by the
  singleton's name - at teardown the name goes first. *(09-30 the Mover server freed the Movers)*
* Every worker needs an owner that stops it before the scripts go. A destructor runs too late. A
  stop must not wait for the whole job, and a drain must not drop tasks someone waits on.
  *(09-24 no symbols / parser; 09-22 RID allocator)*
* A `Callable` across a thread is only as valid as its script, and `is_valid()` does not tell you.
  Read a worker `Callable` all the way down. *(09-24 parser; 09-22 RID allocator)*
* The scene tree is not thread safe; global-scope servers are. Work the tick redoes anyway does
  not also belong in the synchronous API. *(09-22 sfx tick off main thread)*
* An async load checks its root after every await that a drain can end: the editor frees a scene
  it reopens mid-load. A worker cannot even emit a signal of a node in the tree; it hands work
  over by `call_deferred()`. *(10-03 the editor ran the scenario)*

## Build, release, export
* No C++ iostreams in the extension: libstdc++ is linked statically (`GODOTCPP_USE_STATIC_CPP`) and
  a stream crashed the release library only - `FileAccess` and `String` instead.
  *(10-02 a C++ stream crashed the release build)*
* A game or editor still running the old library writes cache entries under a version the new
  scripts bumped - rebuild with Godot closed, or bump the version again. *(09-28 ED72 cache)*
* glibc is only forward compatible. Check the highest `GLIBC_` of every shipped binary, and build
  on an old sysroot. *(09-24 Linux release glibc)*
* A path handed to `FileAccess` is absolute, or it is silently `res://`. *(09-23 game dir ".")*
* A script-source default does not survive export (`get_property_default_value()` is null), and
  EditorPlugin settings do not exist in a release. *(09-21 no fog in release)*
* A one-liner that `cd`s and uses a relative destination has two working directories.
  *(09-21 release never unpacked)*
* Code whose output is cached on disk bumps its cache tag (`structure-vN`, `CACHE_VERSION`,
  `FIZ_PARSER_FORMAT_VERSION`) in the same commit, and every `ResourceCache` is wired into "Clear
  cache". *(09-20 low-poly interior; 09-20 E186 main tank)*
* Never delete a code span with a regex whose optional prefix can match across lines, and verify
  scripted edits structurally. `undefined symbol` means a deleted definition. *(09-22 regex that
  deleted 588 lines)*
* Never apply clang-tidy `--fix` per file: a rename lands in the declaration, not in its uses
  elsewhere. `style-fix` only formats, headers are self-contained, and style-check binds the same
  double API as the build. *(09-29 style-fix broke the build)*
* The export reconverts a scene only when its own file changes; a scene instancing another,
  changed one (`[editable]` above all) ships its old diff of it. A release is exported from an
  empty `demo/.godot/exported`. *(09-29 vehicle card missing in release)*
* The export is run by the editor, and the editor loads the debug library whatever the export:
  every `release-*` export builds `compile-debug` too. *(09-30 release export without CabinSystem)*

## Tests
* `user://` is `~/.local/share/MaSzyna-Reloaded/` (`custom_user_dir_name`); a killed test may
  leave `game_dir` there pointing at the fixtures - check it before a run on the game's data.
  *(10-06 a killed test leaves the game directory pointing at the fixtures)*
* The game directory is changed only while nothing built from it is alive: saving it reloads the
  game's data, and a vehicle rebuilds itself from the directory current then. *(10-03 a test's
  vehicle was built twice)*
* A test is checked against a build without the fix, and one that cannot fail is deleted.
  *(09-24 line breaker opened; 09-24 parser)*
* Test the invariant, not the intermediate. *(09-21 no fog in release)*
* The headless dummy renderer keeps no texture data. A vehicle without mass or a track is NaN, and
  NaN never compares equal or culls. *(09-24 Python screens; 09-24 parked vehicle; 09-22 sound
  cost)*
* The headless dummy renderer's mesh storage is not thread safe: meshes created on the streaming
  worker and on the main thread at once corrupt the heap, and the crash shows later, at teardown.
  *(09-26 headless test crashes at teardown)*
* A hint's check agrees with what its own action does: a diesel's "neutral" is a position
  without the clutch in (`DecSpeed()`), not 0, where its engine stops. *(10-06 SN61: the
  independent brake did not brake, the engine stopped after the start)*
* The hint list is followed top to bottom: a hint keeps its place (it rules out the others of its
  group, not itself), a rear cab is shown the reverser's own side, every hand step shows its key;
  test it by following the list by its keys, held in real time. *(10-06 SN61 did not start by the
  driving aid's hints)*
* A diesel's master controller position with `R=0` is stop, not idle: idle is the first position
  with `Mn>0` (driverhints.cpp:489). The starter holds a fuelless engine at 0.7 x nmin while it
  still spins up, so the stall shows only when frames are slow. *(10-06 SR61 start-up test red
  in the suite)*
* A test that drives a scenery vehicle by commands takes it from its driver and activates a cab:
  `IncMainCtrl()` refuses every step while no cab is active. *(09-28 the EP07 trip test never
  moved)*
* A test that passes alone and fails in the suite inherits a singleton's state from an earlier
  script: a test that changes a server's state (`SimulationServer.simulation_speed`, a Project
  Setting) restores it in `after_each`. Reproduce with `-gpre_run_script` setting that state.
  *(09-30 EP07 tests driven at x20)*
* `ProjectSettings.settings_changed` is emitted deferred: a test that sets a Project Setting a
  server takes on that signal awaits it before reading the effect. A command moved to a component
  is grepped for in the tests. *(10-04 CI red on a setting the server took a frame late)*
* A scenery vehicle has a driver from the start: taken over after it has stood, it may be held
  by its independent brake (Driver.cpp:8166-8180). A test that drives it sets every control it
  needs, the independent brake too. *(09-30 the EP07 orientation test braked by its driver)*
* A vehicle standing at load is built with its hand brake full (Mover.cpp:8946), and only the
  AI's `AutoRewident()` releases it, never on a vehicle the player already drives; no hint asks
  for it. Braked with empty cylinders: read `brake_manual_position` first. *(10-09 SN61 stood on
  its hand brake after the player took it on scenery_loaded)*
* Only a vehicle standing on a track is stepped, and the component a test built its controller
  from is a description, not the vehicle's: a test of a component's tick puts the vehicle on a
  track and takes the component by `VehicleServer.vehicle_component_get()`. After an operation,
  read the component's getters - the state dump is cached until the next step. *(09-30 the load
  exchange that never ran)*
* A change to a physics component is tested through the whole start sequence (battery, low
  voltage, pantographs, main switch, converter, traction); a test it turns red is a suspected
  regression, never fixed by removing its setup or assertion; physics tests are blackbox
  (components' getters, the state dump). A headless probe proves nothing unless the vehicle stands
  on a track, a `SimulationRuntime` runs the clock and somebody drives it. *(10-04 EU07, ED78 and
  36WE reported unstartable, no test guarding the start)*
* A car nobody sits in has no occupied cab (CabOccupied 0, the scenery's driver type): with
  bom_PS its brake valve leaves the pipe alone. *(10-04 every car sat a driver)*
* A key is the cab logic's, never a widget's: a control's key that only its 3D widget takes is dead
  wherever the cab model is not built (a fixture, a model that failed to load). *(10-04 keys of
  modelled controls needed the cab model)*
* A test cannot guard what it hands in itself: a battery given by `build_power_supply()`, a wire
  voltage fed by hand, a vehicle assembled with `add_component()` - the game's way of building it
  is then never run. A test asserts the end of the sequence the player sees, after simulated time,
  never only the flag it switched or a key's presence. *(10-04 test audit)*

* Hold an `E3DModel` in a variable for as long as its submodels are used: freeing it clears
  every submodel (`E3DModel::clear()`), so `load_model(...).get_node(...)` gives a mesh-less
  submodel. *(09-29 submodels without meshes)*
* A pantograph-air report is reproduced on a fixture whose FIZ matches its `PantAutoValve`: the
  EP09 fixture has it, the EP07 (303E) does not, and the valve matters only without it.
  *(10-06 EP09 fixture fills its pantographs on its own)*

## Sound
* A method bound with different arguments is still one connection: when the key changes
  (a vehicle's RID), disconnect it first. *(09-28 coupler events under a handle nothing read)*
* A cab control sounds through the cab's bank as an event placed at its submodel, never through
  its own `AudioStream` player. *(09-25 cab clicks cut each other off)*
* A gain derived as a normalisation divisor is never also applied as a gain. *(09-21 +38 dB)*
* A sound whose original computes its gain at the call site (filters, hysteresis, a hand-made
  fade) is ported as that code with its own state, not as a curve over one parameter. *(10-01
  the local brake hiss keyed to a parameter nobody sent)*
* Check what the MMD declares (`placement:`) before modulating with a parameter. *(09-21 brake
  hiss)*
* Stopping a player resets what its triggers remember about playing. A sound that is due
  out of earshot resumes past its opening bookend (sound.cpp:360). Skip a clip by not starting
  it: an Ogg playback asked to start at its end starts at 0. *(09-28 engine silent after the
  camera came back)*
* A vehicle lamp's hotspot-to-falloff band is 1 deg wide (sm42 fspot 21.5-22.5), so anything
  ramped over it switches on and off; the glare fades over its own band. *(09-28 glare blinking
  with the viewing angle)*
* `LastStationLatency` (driver `latency`) is the departure less the arrival: positive is early,
  and it is the AI's, not a delay to show - the panel shows the timetable's `delay`. *(09-29 an
  early freight train shown 7 min late; 09-30 the timetable's delay frozen on the way)*
* A scenery keyword read and dropped is a behaviour dropped: `departuredelay` moves an event to
  the departure of the train that queued it. *(09-30 the departure sound played on arrival)*
* An extension class named like an engine class (`CameraServer`) builds fine and is refused at
  run time: check the name against Godot's classes, and the `--import` log for "already
  registered". *(09-29 an extension class named like an engine class never registered)*
* When the API godot-cpp is generated from changes (`extension_api.json`, precision), rebuild
  godot-cpp from clean objects in every build dir, and check the library for undefined `godot::`
  symbols (`nm -D -C --undefined-only`). *(09-29 debug library would not load)*
* The scenery parser reads cp1250 (as the original's files are written); a byte past ASCII taken
  as a signed char became U+FFFD and every W4 of a Polish-named station (`Krzyżowa#...`) matched
  no timetable. A W4's station is cut at `#` and made plain ASCII (Event.cpp:715-719). Look for
  W4 through every include level (`ip/pkp/w4n.inc <name>`). *(09-30 Krzyżowa 2 timetable never
  moved)*
* The Mover's `DistCounter` (`total_distance`) grows only in its own movement, not with
  `vehicle_move()`/`trainset_move()`; a way driven that a test must see is measured along the
  driver's route table. *(09-30 station shown never caught up in a test)*
* Whatever reads the game directory and keeps the result - a memo, a material, a built vehicle -
  follows `GameDataServer.data_unload_requested`/`data_reload_requested` itself; a disk cache key
  names the game directory. *(09-30 track textures stayed after a game directory change)*
* A per-step reader (cab logic, sound, AI) takes typed getters, never `vehicle_dump_state()`: the
  dump is composed anew after every step. What only the player's vehicle needs (a rain volume)
  lives in its shown cab; an object that cannot be placed is not built. A headless load of a large
  scenery can crash in the dummy renderer's RIDs - measure under `gamescope --backend headless`.
  *(10-03 hundreds of vehicles)*
* A scenario "not starting" is first an AI train that does not move: trace the driver's
  `stop_reason`/`engine_missing` before the events. The original's driver sets the Mover directly
  (`if( AIControllFlag ) mvOccupied->...`, driverhints.cpp) - a step ported through the cab fails
  on a cab without that control. *(10-03 scenarios that did not start)*
* A launcher's minute is `floor(t * 60)`, as the clock counts it; truncating `(t - hour) * 60`
  lost the start minute of 622 of 1440 start times. *(10-03 scenarios that did not start)*
* A streamed piece reads every file it needs (model, material, texture) on the preload thread;
  the main thread only creates what has to be created there. *(10-02 streaming hitches)*
* A vehicle's sound bank is built when the vehicle comes within earshot, not at load; a sound's
  length is read off its Ogg pages, never by loading the file. *(10-02 the Vehicles stage spent
  its time on sound banks nobody heard)*
* A vehicle's models are built when it comes within the draw distance and freed beyond it, never
  at load (their files are loaded at load unless lazy loading is on); what the simulation needs of the model (the pantographs) is read off the model file,
  not off its drawing. *(10-06 the Vehicles stage drew every vehicle of the scenery)*
* Code that runs in the editor calls no autoload that is not `@tool` (`TrainSoundSystem`,
  `CabinSystem`): the call is a script error returning null into the caller's data - a null part
  of a vehicle crashed the editor on its first rebuild. *(09-30 editor crash on a game dir change)*
* A loaded scenery runs nothing: `SimulationServer`'s clock ticks only under a `SimulationRuntime`
  the game places, and the scenario's scripts and sounds are started by the game
  (`MaszynaLegacyScenario`). A loader that starts the simulation runs it in the editor too.
  *(10-03 the editor ran the scenario)*
* A vehicle's models are made before it stands on its track: its detail is decided where it is
  placed, never assumed - born detailed, every vehicle of a scenery built its whole node hierarchy
  at the origin. *(10-03 the editor ran the scenario)*
* A value in a Mover union (`TPowerParameters`) is read only under the tag that says it is
  there; a collector is a component of its own, built from `Power:` whether there is an engine or
  not. *(10-05 ED78 and EL16 start-up)*
* A FIZ table's layout is the original reader's for the engine type (`readMPT`), never the
  section's name alone. *(10-05 EP03 and EL16 NaN force)*
* A property's default is the original's value for an absent key - read the lookup's fallback,
  not only the parsed values (`CompressorPower` falls back to 1). *(10-05 EN57KM compressor)*
* A tool reading the game's text files skips a UTF-8 BOM as the game does. *(10-05 EN57KM cab)*
* A default the original sets in one FIZ section for another section's keys reaches the vehicle
  even without that section's component (`LoadFIZ_Param`'s EZT `IminLo/Hi`). *(10-05 EN57 cab car reverser)*
* An implementation, a role or a seat goes to the person its owner created, never to whoever holds the
  role at that moment - the player takes a vehicle over while the scenery still loads. *(10-05 player as the AI driver)*
* A CI step piping a test run keeps its exit code (`set -o pipefail`); a green run is checked
  against its summary. *(10-05 CI green over red tests)*
* A hand-built test vehicle states every value its test depends on, never a component default -
  defaults follow the original's absent key. *(10-05 test vehicles lost brake handle and doors)*
* A trainset test waits for every vehicle of it; an "is it gone" check takes the object untyped -
  a typed parameter fails on a freed object at the call. *(10-05 start-up tests and the trainset)*
* A player's cab is built from its scene only once the vehicle has its cab logic - the builder
  hands the scene over last; a cab built without it hangs the game. *(10-06 demo_3d cab not shown)*
* A test steps the simulation (`step()`, `wait_simulated_until()`, limits in simulated seconds
  from the data); never real time, frames or a raised speed; what moves with the simulation runs
  on its time. *(10-07 tests waited on the machine's speed)*
* A build that failed is a result, not a pending build - a vehicle whose model is missing is not
  queued again. *(10-07 a vehicle without its model held the loading screen 30 s)*
* A script thread never calls a server that answers through the main thread, nor touches a node -
  the exit joins it while it waits forever. *(10-07 test runs that did not exit)*
* A script sets the speed back with `simulation_reset_speed()` - `simulation_speed = x` leaves the
  clock at the old speed for the scripts after; reproduce CI with an empty `HOME`; the player is
  seated after `scenery_loaded`. *(10-07 tests that ran at the speed an earlier script left)*
* A consumer of `scenery_loaded` exists and is connected before loading starts; removing a
  readiness fallback means every composition and its test fixture must use that signal path.
  *(10-08 player initialization CI regression)*
* After moving classes or files out of a project, check from a fresh `.godot` (imported twice) and
  parse every test script (`-gdir=res://tests/ -gunit_test_name=<none>`) - a warm class cache hides
  the move, and GUT drops a script that fails to parse without failing the run. *(10-09 classes
  moved out, hidden by a warm class cache)*
* A test frees what it built through its owner (an include node, not the RIDs it owns) before it
  ends - a leftover freed later errors inside the next script's test. *(10-09 a test that fails
  only after another script)*
* Whatever a node puts into another object on entering the tree it takes out on leaving -
  detaching is not removing; a pointer kept in a member points only at its owner, parent,
  children, a singleton or a backend. *(10-10 a scripted component called its freed node)*
