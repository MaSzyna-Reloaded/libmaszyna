# Required cleaning before merge to upstream

Review of `src/` against `AGENTS.md` and `CODE_STYLE.md`, done 2026-09-29 on
`dev/integrate-track-rendering` (`bd2247f9`). Scope: every C++ file in `src/` except the vendored
Mover (then `src/legacy/maszyna-mover`, now `src/maszyna/maszyna-mover`) and the generated
`src/gen` - 320 files, ~42k lines.

Re-verified item by item against the code on 2026-10-09 (`61d035eb8`): fixed items ticked and
their entries removed, the open entries cut down to what is still open, line numbers as of that
commit.

That `src/` was split into libreloaded's and libmaszyna's (`src/maszyna`). Kept here are the items
whose code is now libmaszyna's and the rules that covered both trees; the items of code now in
libreloaded alone are only in libreloaded's own copy of this file.

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
- [x] [RC-014](#rc-014) State keys published before the simulation is ready
- [x] [RC-019](#rc-019) Mover fields written by two components
- [x] [RC-124](#rc-124) Bare coupler built from `max_velocity` instead of `Ftmax`
- [x] [RC-127](#rc-127) Doors `VOLTAGE_112` never powered
- [x] [RC-128](#rc-128) Lighting defaults off the original
- [x] [RC-129](#rc-129) Tachometer without `MaxTachoSpeed` and the slow jump
- [x] [RC-130](#rc-130) Brake control pipe pressure and the `NBpA` clamp

### Separation of concerns and getters (ALARM)

- [x] [RC-020](#rc-020) `wire_get_voltage()` changes the power source's state ✔
- [x] [RC-027](#rc-027) Mover member names as public state keys
- [x] [RC-029](#rc-029) `MaszynaMover*` classes public and instantiated from GDScript (accepted: the FIZ importer creates them by name)
- [x] [RC-030](#rc-030) `MaszynaMover*` components hold state the Mover does not have
- [x] [RC-031](#rc-031) Brake backend keeps a rate only the sound needs
- [x] [RC-033](#rc-033) `MaszynaE3DInstanceBackend` and `MaszynaE3DRenderingServer` include each other
- [x] [RC-039](#rc-039) `MaszynaParser::get*()` advance the cursor (accepted: not a problem)
- [x] [RC-132](#rc-132) `MoverTypes.hpp` - shared Mover translations without an owner

### Missing events and wiring

- [x] [RC-042](#rc-042) `force_detail_refresh` "try again next tick" flag
- [x] [RC-044](#rc-044) Animation bindings resolved twice after reload
- [x] [RC-048](#rc-048) Loading queue waits by sleeping and is polled
- [x] [RC-050](#rc-050) Lazy initialisation re-checked on every call

### Per-frame work

- [x] [RC-052](#rc-052) Whole config dictionary built per frame for the wiper angle
- [x] [RC-053](#rc-053) Coupler lookups and string building per frame
- [x] [RC-054](#rc-054) Bogie track samples computed twice per frame
- [x] [RC-055](#rc-055) Pantograph geometry through string-keyed dictionaries per frame
- [x] [RC-056](#rc-056) `ProjectSettings` read per vehicle every 0.25 s
- [x] [RC-057](#rc-057) Allocations and boxing in the vehicle server step
- [x] [RC-059](#rc-059) Unbounded E3D animation loop with allocations
- [x] [RC-060](#rc-060) Time and light level each walk all E3D instances
- [x] [RC-062](#rc-062) `MaszynaE3DOptimizedBackend::update()` allocates a map
- [x] [RC-069](#rc-069) Wipers ticked while parked

### Raw pointers in public API

- [x] [RC-070](#rc-070) `MaszynaE3DRenderingServer::instance_attach_node(Node3D *)` ✔
- [x] [RC-074](#rc-074) `MaszynaTrianglesImporter::import_triangles(MaszynaParser *)` ✔
- [x] [RC-076](#rc-076) `MaszynaE3DSubModel::set_parent(MaszynaE3DSubModel *)` ✔
- [x] [RC-123](#rc-123) Scene-tree nodes holding pointers to objects they do not own

### Calls by name

- [x] [RC-077](#rc-077) GDScript `MaszynaE3DModelInstance` calls without the required comment ✔
- [x] [RC-078](#rc-078) Commands registered with `Callable(this, "name")`
- [x] [RC-079](#rc-079) Signal names as literals despite constants

### Magic numbers

- [x] [RC-084](#rc-084) `MaszynaMoverRailVehicleBuffCoupl`
- [x] [RC-085](#rc-085) `MaszynaMoverRailVehicleBrake`
- [x] [RC-086](#rc-086) `MaszynaMoverRailVehicleMasterController`
- [x] [RC-087](#rc-087) Warning signal bitmasks in horns and security system
- [x] [RC-088](#rc-088) `MaszynaMoverCircuitUnit` thresholds
- [x] [RC-089](#rc-089) `MaszynaMoverRailVehicleWheels`
- [x] [RC-090](#rc-090) `60.0` rpm conversion repeated in six files
- [x] [RC-091](#rc-091) Speed control preset count and doors remote control value
- [x] [RC-092](#rc-092) `MaszynaE3DNodesBackend` light settings defaults
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
- [x] [RC-105](#rc-105) Dead code
- [x] [RC-106](#rc-106) Private helpers with a single call site
- [x] [RC-107](#rc-107) Forwarding wrappers
- [x] [RC-108](#rc-108) Same work done twice
- [x] [RC-109](#rc-109) Duplicated declarations
- [x] [RC-110](#rc-110) Config keys nobody reads

### Naming

- [x] [RC-112](#rc-112) Older server methods and signals named otherwise
- [x] [RC-113](#rc-113) `consist` instead of `trainset`

### Cosmetic

- [x] [RC-114](#rc-114) `MAKE_MEMBER_GS*` macros switch the section (macros kept as `MAKE_GS`, section left alone)
- [x] [RC-115](#rc-115) `macros.hpp` included where unused
- [x] [RC-116](#rc-116) Functional casts instead of `static_cast`
- [x] [RC-117](#rc-117) State keys with slashes
- [x] [RC-118](#rc-118) Headers not self-contained
- [x] [RC-119](#rc-119) `using namespace godot;` in a header
- [x] [RC-120](#rc-120) Privacy sections
- [x] [RC-121](#rc-121) Stale, orphaned and non-English comments
- [x] [RC-122](#rc-122) Commands named `set_*` without a property

