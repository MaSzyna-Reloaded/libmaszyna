---
name: maszyna-architecture
description: Decide where a piece of state, behaviour or API belongs in this wrapper's layers, and check that a change does not breach a boundary. Use before adding a class, a field, a signal, a state key or a server method; before moving logic between C++ and GDScript; when something needs a value that lives in another layer; when a node, a system and a server all look like plausible homes for the same thing; and when reviewing a diff for separation of concerns. Answers "which layer owns this", "may this layer know about that one", and "is this the same mistake we already made".
---

# MaSzyna Reloaded - layers, boundaries and how they are enforced

This wrapper is a stack of layers, and the whole architecture rework (#184) exists to make each
one replaceable on its own. Most defects recorded in `FINDINGS.md` are a layer that reached into
another: a feature parked on a node vanished when a second instancer appeared without nodes; a
sound trigger's counters lived on the vehicle; a getter advanced a filter, so the value depended
on how often it was read.

**The question that settles every argument about placement:** *if this layer were replaced
wholesale, would this field go with it?* If yes, it belongs there. If it would have to be kept
for somebody else, it belongs to that somebody.

## The port is data-driven

**The port is data-driven - ABSOLUTE.** Every name, prefix, count, flag and default the original
reads from data (FIZ, MMD, SCN/SCM/INC, CHK, lang, models) is read from that data in the wrapper, the
way the original reads it, with the original's default when absent. Only what the original itself
hardcodes, or a quirk documented in `MASZYNA_ORIGINAL_QUIRKS.md`, may be hardcoded - with the
original's line cited. "Confirmed identical across the vehicles checked" is never a reason: the
pantograph arm names hardcoded on 2026-09-16 after checking three vehicles (`ramiedolne1_pant0`, read
by the original from `animpant*prefix:`) left 104 vehicles unanimated (36WE, EN57, ET22, 4E...), the
wheel prefix 109, and searching cab 2 from `cab1definition:` broke 133 cabs.

## The map

Read top to bottom: each layer may use the ones below it and must know nothing of the ones above.

| Layer | Who | Owns | Must never |
|---|---|---|---|
| Backend | `src/legacy/maszyna-mover/` (vendored `TMoverParameters`) | physical quantities, the original's own model | be edited. It is vendored; a divergence is ported around it, never into it |
| Backend adapters | `Mover*` classes (`MoverRailVehicleController`, `MoverRailVehicle<Domain>`, `MoverComponent`) | the only `TMoverParameters *` in the process | appear in any name, parameter or return type above this row |
| Vehicle model | `VehicleController` (an `Object`, any vehicle) and `RailVehicleController` (a railway one: cabs, couplers, controllers, relays), `VehicleComponent` / `RailVehicleComponent` + `RailVehicle<Domain>` interfaces | a vehicle's state, configuration and the operations that change them | name the backend; be a `Node`; hold anything only a drawing, sound or UI layer needs |
| Servers | `RailVehicleServer`, `TrackServer`, `TractionServer`, `E3DRenderingServer`, `SceneryStreamingServer`, `PythonScreenServer`, `SignallingServer`, and the GDScript servers (`TrackRenderingServer`, `TractionRenderingServer`, `MaszynaSceneryChunkRenderingServer`, `ScenerySoundServer`) | handles (`RID`), the placement and the composition of what they own, their own worker threads | hand out raw pointers, or hold state a single node could own; reach a vehicle's controller to execute vehicle logic (see "Servers forward to components") |
| Systems | `CabinSystem` (a cab per vehicle RID and cab), `TrainSoundSystem`, `MaterialManager`, `SceneryInstancer` | cross-cutting bookkeeping keyed by handle, and the vocabulary the data uses | duplicate state the model already has; drive per-frame work that a server could do natively |
| Nodes | `RailVehicle3D`, `VehiclePhysicsNode`, `Cabin3D`, `RailVehicleStepper`, cab widgets, `SignalHeadNode`/`SignallingSystemNode` (proxies holding a server RID) | what is drawn and what is in the scene tree | contain simulation; own a handle the server already owns; reach a vehicle by walking the tree |
| Importers | `src/legacy/parsers/`, `src/legacy/e3d/`, `addons/libmaszyna/legacy/fiz/`, `.../legacy/mmd/`, `.../legacy/scenery/` | turning FIZ, MMD, E3D and `.scn` into model objects | build scene trees as their output, or keep parse results in nodes |

### Servers forward to components

The components' API (a coupling component, say) is a layer below the server. A server offers the
**high-level API** for the high-level operations: walking through the cabins
(`person_change_cabin()`), coupling (`vehicle_couple()`), managing trainsets (`trainset_place()`)
and moving a vehicle (`vehicle_move()`). Whether a server has to wrap everything beyond these is
not settled - ask the operator before wrapping more. A vehicle server **forwards its execution to
the vehicle's component** for that domain - the base classes
(`RailVehicleBuffCoupl`, `RailVehicle<Domain>`) exist for this. The component's backend
implementation executes it on the physics backend: the server finds the vehicle's rail buffer and
coupling component, on the Mover's physics that is `MoverRailVehicleBuffCoupl`, and the coupling
happens there. A server never reaches the vehicle's controller to execute such logic.

Logic outside the vehicle - the AI driver, the player, scripts - **executes through the servers**.
The components' API is fixed too, but it serves reading state on hot paths and the backend's
implementation of the forwarding, not executing logic from outside.

Debt on record (2026-10-09, `TODO.md`): `RailVehicleServer`'s coupling (`vehicle_couple()`,
`uncouple`, `is_coupled_by`, `is_coupler_automatic`, adapters,
`vehicle_get_coupler_joinable_flags()`) still goes through `RailVehicleController`/
`MoverRailVehicleController`, and the AI couples by the `coupler_connect` command; nothing new is
added that way.

Autoloads are listed in `demo/project.godot`; C++ singletons are registered in
`src/register_types.cpp`. A singleton is reached by its typed `get_instance()` and the result is
null-checked - never by node path, never by `call("name")`.

## A breach is an alarm, not a problem to solve quietly

When the work at hand would break a boundary - a field in a layer that does not own it, a lower
layer calling or naming an upper one, two layers each calling the other, a copy of state kept "for
convenience" - **stop and report it to the operator before writing it**: which layer, which field
or call, which rule it breaks, and the options you see. Do not invent a workaround (a shim, a
cached copy, a signal relayed through a third party to hide the dependency, a string `call()`), and
do not redesign in silence halfway through a task. The operator decides.

Examples of what must raise the alarm:
* a camera server holding what the player drives (the player's state, not the view's);
* the player's server calling the camera server while the camera server reads the player's -
  a cycle: one of them must only read and listen;
* a HUD component keeping its own copy of a vehicle RID another server owns.

## The five boundaries that carry the design

**1. The backend never appears above its adapter.** No `Vehicle*.hpp` interface mentions
`TMoverParameters`, and only `Mover*` files hold one. Every one of the `MoverRailVehicle<Domain>`
implementations has a `RailVehicle<Domain>` interface of the same name, and none of those interfaces
names the backend. A component reaches the backend through `MoverComponent`: the vehicle hands
every component its implementation (`VehicleComponent::attach_implementation()`), and a Mover
component takes its vehicle's Mover from `MaszynaMoverVehicleServer`, which owns the Movers, in its
own `_implementation_changed()` - no `dynamic_cast`, one lifetime (from the simulation's start to
`release()`).

    grep -l TMoverParameters $(find src -name 'Vehicle*.hpp')   # must print nothing

**2. A public API takes handles, not pointers.** `VehicleServer` exposes `vehicle_create`,
`controller_create`/`controller_configure`, `vehicle_bind_controller`, `vehicle_dump_state`, and
`RailVehicleServer` `vehicle_attach`, `vehicle_set_track`, `vehicle_get_transform` - `RID` in,
`Variant` out. Their controller pointers are private helpers, which is the rule working: pointers
stay inside one class. What a consumer would have wanted the controller for comes out under the
handle instead - the controller's moves and commands are relayed by `VehicleServer`
(`vehicle_moved`, `vehicle_command_received`), its rail events by `RailVehicleServer`
(`vehicle_driver_cabin_changed`, `vehicle_trainset_changed`, `vehicle_coupler_*`).

**3. State has one owner and one writer, and a getter only reads.** A value reaches other layers
as a typed getter of the component that owns it, and configuration as that component's (or the
controller's) properties - a reader takes the component once (`vehicle_component_get()`) and calls
it. The same values are also published by name through `_fill_state_dictionary` /
`_fill_config_dictionary` (`VehicleComponent.hpp:90,104`), composed by the server into a dump that
is rebuilt after every step or command (the config dump on every call). The dump is for readers
driven by a name out of the data - the cab (CabinSystem, no second cache in front of the server's),
MMD sound triggers, the console, tests - and **never for a hot path**: a sound, a driver, a camera
reading it per frame composes hundreds of keys for one value (2026-09-30, sound system at x8).
Change detection,
filters and flag consumption belong in the tick, never in a fetch or a getter - the four
violations of this cost a day each (`FINDINGS.md`, 2026-09-22).

**4. Ordering is an event, never a retry.** The simulation steps before anything reads it
(`RailVehicleStepper`, `process_priority` = -100), and a value that is not there yet is an
ordering defect: react to `simulation_configured`, do not raise a "try again next frame" flag.

**5. A vehicle is its handle, never its name.** Everything that holds a vehicle holds its
`RailVehicleServer` RID and commands it with `vehicle_send_command(vehicle_rid, ...)`; a vehicle
keeps its own commands (`VehicleController::register_command`). The scenery name (`train_id`) may
be empty or repeated, and it lives only in the server's name registry: `vehicle_get_rid_by_name`
leaves `""` and `"none"` unmapped and gives a repeated name to the later vehicle, as the original
`basic_table::insert` does (`Names.h:27-40`). Only what knows a vehicle by name alone - a scenario,
an event, the console - goes through it. A registry keyed by that name elsewhere is how the second
of two same-named vehicles lost its commands and its cab (TrainSystem, removed 2026-09-26).

## Where a new thing goes

* **A value the simulation produces** - a component's own field, published through its
  `_fill_state_dictionary`. Before reusing a config getter for it, grep the backend for an
  assignment to that field: a value the simulation writes is state, a value only the wrapper
  writes is configuration, and confusing the two is how a battery reported a voltage that never
  moved.
* **A value only one consumer needs** - that consumer's layer. Sound bookkeeping lives in the
  sound system; what a gauge shows lives in the cab; a counter a trigger compares against is not
  the vehicle's business.
* **A handle, a placement, a composition** - the server that owns the thing. The body transform is
  composed by `RailVehicleServer` from the two bogie pivots, not by the node that draws it; two
  writers agreed on every straight track in the game and disagreed on curves.
* **Anything a scenery places in the world** (a model, a track, a wire, a light, a smoke emitter,
  a sound) - a server that registers it with `SceneryStreamingServer` (`owner_create` +
  `stream_register`) and builds it only while the camera is in its range. Never a node per
  placement built at load: a scenery has tens of thousands of them. Scenery sounds are event
  pairs of `ScenerySoundServer`'s one `SfxPlayer3D` and one `SfxBank`, placed by
  `SfxSpatialConfig.position`, not a player each (2026-09-29).
* **Something an instancer must honour** - the server, not the node that happens to create the
  instance. The moment a second instancer appeared without nodes, every feature parked on a node
  silently stopped existing.
* **A new component** - a `RailVehicle<Domain>` interface plus a `MoverRailVehicle<Domain>` implementation, added
  with `add_component()` and reached with `get_component(VehicleComponentType::TYPE)`.
* **Behaviour that differs between variants of one server object** - an implementation, not a subclass
  of the object: an interface class of `GDVIRTUAL`s whose C++ virtuals forward to the script
  (`SignallingImplementation`), implemented in C++ (`MaszynaLegacySignallingImplementation`) or GDScript,
  attached after `*_create()` (`system_attach_implementation`). Every callback carries the owner's RID,
  since a `Resource` is shared; the implementation changes state only through the server's API. The
  original's behaviour is one such implementation (`MaszynaLegacy*`), never the base.
* **Something that lives as long as another server's handle** - follow the owner's freed signal
  (`E3DRenderingServer.instance_freed` frees the instance's signal head), not a validity check.
* **Configuration of a handle** - the server's struct under the RID (`SignalHeadData`), set by
  `<thing>_set_<field>` and announced by `<thing>_config_changed`. What the data already knows
  is derived by the server, never typed in by the user (a signal head's light count comes from its
  model when `E3DRenderingServer` emits `instance_built`). Shared data describing a *kind* of
  thing is a `Resource` handed to the server (`signal_head_set_kind(rid, SignalHeadKind)`), not
  fields copied onto each handle. Where the original has no such description, the importer
  derives it from the data (`MaszynaLegacySignalHeadKindFactory` makes a kind of the `lights`
  events aimed at a model, and equal kinds become one resource) - the original's encoding is
  read once there and never reaches the server.
* **A node over a handle** - its properties are proxies: getters read the server, setters call
  it, the server's change signals are relayed filtered to the node's RID and refresh the property
  list (`SignalHeadNode`). The node keeps only what the scene sets before the handle exists
  (`kind`, the initial `aspect`) and hands it over once. A node-typed export in C++ is an
  `ObjectID`, not a `Node *` - the node it names may be freed first. A GDScript accessor on a
  native subclass needs a name the native class does not have (`get_e3d_instance`, not
  `get_instance`, on a `VisualInstance3D`).
* **Recurring work** - C++ on `process_frame`, then a `Timer`, then `_process` with an
  accumulator. Never a bare per-frame `_process`, never a loop in one, and never wiring
  (`connect`, `get_node`, a path) inside one.

## Breaches that exist today

These are known and being removed. **They are not precedent** - do not copy their shape, and do
not cite them as "how this codebase does it". Counts measured 2026-09-27; re-measure before
relying on one.

* There are no update phases: the step order is a hand-written sequence in
  `MaszynaMoverVehicleServer::stepping_advance()`, and `UpdatePhase` exists nowhere.
* The dump carries two key conventions - nine `prefix/key` namespaces against a majority of flat
  `component_key` names. 53 `state_property` values in the MMD catalog contain `/`, so this is a
  data contract, not a rename.

## Before committing a change to any of this

1. `grep -l TMoverParameters $(find src -name 'Vehicle*.hpp')` - the backend has not leaked.
2. `grep -n '\*' <server>.hpp` - no pointer crossed a public boundary.
3. Read every `get_*` the diff adds: it returns and does nothing else.
4. Read every `_process` the diff touches: no loop, no lookup, no wiring, nothing while idle.
   `grep -n 'vehicle_dump_' <changed files>` - no dump on a per-frame, per-step or per-tick path.
5. Ask the placement question of each new field, out loud, and write the answer in the commit.
6. `grep -rn 'train_id' addons src` - a scenery name is never a key a vehicle is held, cached or
   commanded by; only `vehicle_get_rid_by_name` turns a name into a vehicle.

**And verify against the code, never against a document.** `TODO.md`'s stage list was written when
the first part of each stage landed and went stale; this file will too. Every count and line
number here was measured, and the next reader measures again - a claim about this codebase that
nobody re-ran is a guess with a citation.
