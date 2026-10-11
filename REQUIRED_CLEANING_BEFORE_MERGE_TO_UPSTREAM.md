# Required cleaning before merge to upstream

Review of `src/` against `AGENTS.md` and `CODE_STYLE.md`, done 2026-09-29 on
`dev/integrate-track-rendering` (`bd2247f9`). Scope: every C++ file in `src/` except the vendored
`src/legacy/maszyna-mover` and the generated `src/gen` - 320 files, ~42k lines.

Re-verified item by item against the code on 2026-10-09 (`61d035eb8`): fixed items ticked and
their entries removed, the open entries cut down to what is still open, line numbers as of that
commit.

Legend:

* `✔` - verified by hand in the code; the rest was verified by reading the surrounding code
* `ALARM` - a separation-of-concerns breach; per `AGENTS.md` the direction is decided by the
  operator before any code changes
* Numbers `RC-NNN` are permanent - never renumbered or reused. A resolved item is ticked in the
  index and its entry below is deleted, **in the commit that resolves it**; an item that turns out
  to be invalid the same way, marked "(invalid)" in the index
* Checked before every commit (`AGENTS.md`, "Before every commit"): the staged diff ticks what it
  resolves, corrects the lines of what it touches, and adds no new instance of an open item

## Index

### Junk

- [x] [RC-001](#rc-001) Stale SCons `.os` object files and directories left in `src/`

### Correctness

- [x] [RC-002](#rc-002) `BrakeMethod` reaches the Mover unmapped ✔
- [x] [RC-003](#rc-003) Diesel backend forces test power source ✔
- [x] [RC-004](#rc-004) Configuration applied up to five times per vehicle ✔
- [x] [RC-005](#rc-005) Cargo list duplicated by repeated configuration
- [x] [RC-006](#rc-006) Radio call commands never unregistered ✔
- [x] [RC-007](#rc-007) `e3d_loaded` connected again on every dirty frame ✔
- [x] [RC-008](#rc-008) Headlight colour 255 times overbright ✔
- [x] [RC-009](#rc-009) Main switch voltage defaults derived from zero
- [x] [RC-010](#rc-010) Mover controller commands dereference a null backend
- [x] [RC-011](#rc-011) Singleton teardown leaks `TractionServer` and breaks the order ✔
- [x] [RC-012](#rc-012) `MaszynaTranslationServer` dereferences `UserSettings` unchecked
- [x] [RC-013](#rc-013) `RailVehicleDoors::VOLTAGE_AUTO` not bound
- [x] [RC-014](#rc-014) State keys published before the simulation is ready
- [x] [RC-015](#rc-015) `VehicleComponent::send_command()` discards the command result
- [x] [RC-016](#rc-016) Cab control drag signs lost without a mesh
- [x] [RC-017](#rc-017) `PlanarMirror3D` default outside its own range
- [x] [RC-018](#rc-018) `get_cache_dir` bound with a default for a missing argument
- [x] [RC-019](#rc-019) Mover fields written by two components
- [x] [RC-124](#rc-124) Bare coupler built from `max_velocity` instead of `Ftmax`
- [x] [RC-125](#rc-125) `Cabin3D` shake: jolt limit and the random jolt
- [x] [RC-126](#rc-126) `TrackServer` track width and switch blade speed
- [x] [RC-127](#rc-127) Doors `VOLTAGE_112` never powered
- [x] [RC-128](#rc-128) Lighting defaults off the original
- [x] [RC-129](#rc-129) Tachometer without `MaxTachoSpeed` and the slow jump
- [x] [RC-130](#rc-130) Brake control pipe pressure and the `NBpA` clamp

### Separation of concerns and getters (ALARM)

- [x] [RC-020](#rc-020) `wire_get_voltage()` changes the power source's state ✔
- [x] [RC-021](#rc-021) Vehicle server calls the scene node
- [x] [RC-022](#rc-022) Drawing node runs pantograph physics and writes simulation inputs
- [x] [RC-023](#rc-023) Base vehicle layer knows rail
- [x] [RC-024](#rc-024) Controller and server call each other; `get_state()` builds a cache
- [x] [RC-025](#rc-025) `vehicle_get_transform()` writes a cache
- [x] [RC-026](#rc-026) Drawing node creates a second vehicle RID
- [x] [RC-027](#rc-027) Mover member names as public state keys
- [x] [RC-028](#rc-028) `RailVehicleHorns` includes the vendored Mover
- [x] [RC-029](#rc-029) `Mover*` classes public and instantiated from GDScript (accepted: the FIZ importer creates them by name)
- [x] [RC-030](#rc-030) `Mover*` components hold state the Mover does not have
- [x] [RC-031](#rc-031) Brake backend keeps a rate only the sound needs
- [x] [RC-032](#rc-032) Radio component calls up into the vehicle servers
- [x] [RC-033](#rc-033) `E3DInstanceBackend` and `E3DRenderingServer` include each other
- [x] [RC-034](#rc-034) Driver layer tracks player-controlled vehicles
- [x] [RC-035](#rc-035) `Cabin3D` keeps a controller path it says it has not got
- [x] [RC-036](#rc-036) `SimulationServer` holds cache, build and language
- [x] [RC-037](#rc-037) `track_get_endpoints()` fills a cache (accepted: not a problem)
- [x] [RC-038](#rc-038) `build_get_number()` reads a file and sets a flag
- [x] [RC-039](#rc-039) `MaszynaParser::get*()` advance the cursor (accepted: not a problem)
- [x] [RC-131](#rc-131) `RailVehicleHorns::get_combined_signal()` returns the Mover's bits
- [x] [RC-132](#rc-132) `MoverTypes.hpp` - shared Mover translations without an owner

### Missing events and wiring

- [x] [RC-040](#rc-040) Cabin shown after counting frames
- [x] [RC-041](#rc-041) `pending_start_track_retry` retry flag
- [x] [RC-042](#rc-042) `force_detail_refresh` "try again next tick" flag
- [x] [RC-043](#rc-043) `RailVehicle3D` wires nodes inside `_process`
- [x] [RC-044](#rc-044) Animation bindings resolved twice after reload
- [x] [RC-045](#rc-045) Component enable lands a tick late through flags
- [x] [RC-046](#rc-046) Lazy `owner_create` inside the streaming build
- [x] [RC-047](#rc-047) `build_check_version()` once-guard called "to be sure"
- [x] [RC-048](#rc-048) Loading queue waits by sleeping and is polled
- [x] [RC-049](#rc-049) `area_is_ready()` has no event and is polled per frame
- [x] [RC-050](#rc-050) Lazy initialisation re-checked on every call

### Per-frame work

- [x] [RC-051](#rc-051) Every `RailVehicle3D` processes every frame
- [x] [RC-052](#rc-052) Whole config dictionary built per frame for the wiper angle
- [x] [RC-053](#rc-053) Coupler lookups and string building per frame
- [x] [RC-054](#rc-054) Bogie track samples computed twice per frame
- [x] [RC-055](#rc-055) Pantograph geometry through string-keyed dictionaries per frame
- [x] [RC-056](#rc-056) `ProjectSettings` read per vehicle every 0.25 s
- [x] [RC-057](#rc-057) Allocations and boxing in the vehicle server step
- [x] [RC-058](#rc-058) Track roll read by property name per moved vehicle
- [x] [RC-059](#rc-059) Unbounded E3D animation loop with allocations
- [x] [RC-060](#rc-060) Time and light level each walk all E3D instances
- [x] [RC-061](#rc-061) Smoke emitters looked up and ticked while idle
- [ ] [RC-062](#rc-062) `E3DOptimizedBackend::update()` allocates a map
- [x] [RC-063](#rc-063) Scenery streaming runs every frame while idle
- [x] [RC-064](#rc-064) Scenery streaming entry found by linear scan
- [x] [RC-065](#rc-065) `TractionServer` ticks forever
- [x] [RC-066](#rc-066) `Cabin3D` processes forever
- [x] [RC-067](#rc-067) `PlanarMirror3D` sets shader parameters every frame
- [x] [RC-068](#rc-068) `SimulationServer::get_instance()` looked up per frame
- [x] [RC-069](#rc-069) Wipers ticked while parked

### Raw pointers in public API

- [x] [RC-070](#rc-070) `E3DRenderingServer::instance_attach_node(Node3D *)` ✔
- [x] [RC-071](#rc-071) `SceneryStreamingServer::streaming_set_camera(Camera3D *)` ✔
- [x] [RC-072](#rc-072) `RailVehicleServer::vehicle_component_get()` returns a pointer
- [x] [RC-073](#rc-073) `SignalHeadNode::set_model(Node *)` / `get_model()` (invalid)
- [x] [RC-074](#rc-074) `MaszynaTrianglesImporter::import_triangles(MaszynaParser *)` ✔
- [x] [RC-075](#rc-075) Vehicle layer bound methods taking and returning pointers
- [x] [RC-076](#rc-076) `E3DSubModel::set_parent(E3DSubModel *)` ✔
- [x] [RC-123](#rc-123) Scene-tree nodes holding pointers to objects they do not own

### Calls by name

- [x] [RC-077](#rc-077) GDScript `E3DModelInstance` calls without the required comment ✔
- [x] [RC-078](#rc-078) Commands registered with `Callable(this, "name")`
- [x] [RC-079](#rc-079) Signal names as literals despite constants

### Magic numbers

- [x] [RC-080](#rc-080) `RailVehicle3D`
- [x] [RC-081](#rc-081) `Cabin3D`
- [x] [RC-082](#rc-082) `TractionServer`
- [x] [RC-083](#rc-083) `TrackServer`
- [x] [RC-084](#rc-084) `MoverRailVehicleBuffCoupl`
- [x] [RC-085](#rc-085) `MoverRailVehicleBrake`
- [x] [RC-086](#rc-086) `MoverRailVehicleMasterController`
- [x] [RC-087](#rc-087) Warning signal bitmasks in horns and security system
- [x] [RC-088](#rc-088) `MoverCircuitUnit` thresholds
- [x] [RC-089](#rc-089) `MoverRailVehicleWheels`
- [x] [RC-090](#rc-090) `60.0` rpm conversion repeated in six files
- [x] [RC-091](#rc-091) Speed control preset count and doors remote control value
- [x] [RC-092](#rc-092) `E3DNodesBackend` light settings defaults
- [x] [RC-093](#rc-093) Microsecond conversions and E3D distances
- [x] [RC-094](#rc-094) `e3d_parser` flags and offsets
- [x] [RC-095](#rc-095) `MaszynaParser` characters and buffer size
- [x] [RC-096](#rc-096) Scenery loading and streaming timings
- [x] [RC-097](#rc-097) Vehicle server, controller and coupler literals
- [x] [RC-098](#rc-098) FIZ defaults in rail component headers without a source

### DRY / KISS

- [x] [RC-099](#rc-099) `_rename()` duplicated in three servers
- [x] [RC-100](#rc-100) Clock-hold processing code duplicated
- [x] [RC-101](#rc-101) Electric traction forwarders copied into three engines
- [x] [RC-102](#rc-102) Several public roads to one effect
- [x] [RC-103](#rc-103) Camera mode written past `camera_set_mode()`
- [x] [RC-104](#rc-104) `TrackServer::set_is_topology_changed()` second writer
- [x] [RC-105](#rc-105) Dead code
- [x] [RC-106](#rc-106) Private helpers with a single call site
- [x] [RC-107](#rc-107) Forwarding wrappers
- [x] [RC-108](#rc-108) Same work done twice
- [x] [RC-109](#rc-109) Duplicated declarations
- [x] [RC-110](#rc-110) Config keys nobody reads

### Naming

- [x] [RC-111](#rc-111) New radio signals not `<subject>_<what>_changed`
- [x] [RC-112](#rc-112) Older server methods and signals named otherwise
- [x] [RC-113](#rc-113) `consist` instead of `trainset`

### Cosmetic

- [x] [RC-114](#rc-114) `MAKE_MEMBER_GS*` macros switch the section (macros kept as `MAKE_GS`, section left alone)
- [x] [RC-115](#rc-115) `macros.hpp` included where unused
- [x] [RC-116](#rc-116) Functional casts instead of `static_cast`
- [ ] [RC-117](#rc-117) State keys with slashes
- [x] [RC-118](#rc-118) Headers not self-contained
- [x] [RC-119](#rc-119) `using namespace godot;` in a header
- [x] [RC-120](#rc-120) Privacy sections
- [x] [RC-121](#rc-121) Stale, orphaned and non-English comments
- [ ] [RC-122](#rc-122) Commands named `set_*` without a property

---

## Per-frame work

### RC-062

**`E3DOptimizedBackend::update()` allocates a map**

* **Where:** `src/legacy/e3d/E3DOptimizedBackend.cpp:38, 57`
* **Problem:** `HashMap<E3DSubModel *, bool> overrides` is allocated, and every RID is walked,
  on each call - reached from blink edges in `_process_lights` and from `_resolve_all_lights`.
* **Fix:** keep the map as a member and update only what changed.


## Cosmetic

### RC-117

**State keys with slashes**

* **Where:** about 73 keys in 6 files: `MoverRailVehicleElectroPneumaticDynamicBrake.cpp`
  (`dcemued/…`, 6), `MoverRailVehicleSpeedControl.cpp` (`speed_control/…`, 6),
  `MoverRailVehicleSpringBrake.cpp` (`spring_brake/…`, 5), `MoverRailVehicleLighting.cpp`
  (`lights/…`, 20), `src/vehicles/rail/RailVehicleEnginePowerSource.cpp` (30, mostly
  `current_collector/…`), `RailVehicleElectricEngine.cpp:130-135` (`indicators/…`)
* **Rule:** by analogy with "property names ... without slashes"
* **Decision:** whether state keys follow the property rule; renaming them touches the GDScript
  readers.

### RC-122

**Commands named `set_*` without a property**

* **Where:**
  * `src/vehicles/rail/RailVehicleSpringBrake.cpp:34-37`: commands
    `set_spring_brake_active/enabled`
  * `RailVehicleElectroPneumaticDynamicBrake.cpp:49-50`: command `set_ep_brake_force`
  * `RailVehicleHorns.cpp:19-21`: the commands `horn_low`, `horn_high` and `whistle` dispatch to
    methods named `set_*`
* **Rule:** `CODE_STYLE.md` "Godot properties" - `set_<property>` is reserved for property
  setters
* **Fix:** use command verbs (`spring_brake_activate`, ...) for the commands and the methods
  behind them.
