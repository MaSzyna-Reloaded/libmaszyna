## Code style
Unless a section says otherwise, every rule here applies to GDScript and C++ alike.

> [!IMPORTANT]  
> Exported/All classes used in Godot do live in the `godot` namespace   
> Your linter must be compatible with code style defined in `.clang-format`
### Naming
1. Function naming: `snake_case`
2. Variable naming: `snake_case`
3. Class naming: `PascalCase`
4. Parameter naming: `snake_case` with `p_` as an prefix
5. A server's method (a `*Server`/`*System` singleton) is named `<subject>_<action>()` - what the
   handle or the state it acts on is, then what it does - and its signal
   `<subject>_<what>_changed` for a change of state, or `<subject>_<past-tense verb>` for an event
   (`vehicle_freed`, `instance_built`, `simulation_paused`), the way Godot's servers do
   (`RenderingServer.instance_set_transform()`, `PhysicsServer3D.body_get_state()`):

   ```cpp
   // not this - the subject last, or missing
   double get_current_simulation_speed() const;
   void enter_vehicle(const RID &p_vehicle);

   // this
   double simulation_get_current_speed() const;
   void player_enter_vehicle(const RID &p_vehicle);
   // signals: camera_changed, player_vehicle_changed, simulation_current_speed_changed
   ```

   The subject groups a server's API by what it acts on (`vehicle_*`, `camera_*`, `panel_*`), so
   the next method has an obvious name and place. The one exception is a Godot property of a server
   (`SimulationServer.time_of_day`): its accessors keep `set_<property>`/`get_<property>`, as
   "Godot properties" below requires.
### Indentation and braces
1. Braces at the end of line (K & R style)
2. As less nesting as possible
3. Indentation: 4 spaces
4. Indentation inside class privacy declarations
```cpp
//Example code matching this style
void set_door_open(bool p_state) {
    if (condition) {
        door_state = p_state;
        return;
    }
    //Do something else
    return;
}
```
```gdscript
func _process(delta):
    return delta * 2
```
### Formatting is mandatory
Every C++ line that reaches a commit is formatted by `.clang-format`, without exception. This is
not cosmetic: the `style-check` workflow fails the PR on a single violation, and a backlog of them
hides the new ones.

* Format **the files you touched**, before committing: `clang-format -i <file>`, then
  `clang-format --dry-run <file>` must print nothing.
* `make style-fix` (or `make style-fix STYLE_FILE=<file>`) runs clang-format only. clang-tidy
  findings are fixed by hand: its `--fix` run file by file renamed a declaration without its uses
  in other files and broke the build.
* **A header is self-contained**: it includes or forward-declares everything it names. clang-format
  sorts includes and puts a file's own header first, so a header that compiled only thanks to an
  earlier `#include` in the `.cpp` breaks as soon as the file is formatted.
* The `clang-tidy` stage of `make style-check` runs after clang-format and fails on any warning
  (`WarningsAsErrors: "*"`). A ported external name that the naming rule rejects (the Python C API
  in `PythonScreenServer`) is fenced with `NOLINTBEGIN/END(readability-identifier-naming)` and a
  comment saying why - never renamed.

### GDScript
The short GDScript rules (singleton guards, `/root/...`, `is_connected()`, setters and `_dirty`,
`not ... == ...`, `VehicleServer.vehicle_send_command`) are listed in `AGENTS.md`.

1. **A signal of a scene node is connected in the scene.** If the node stands in the `.tscn`, its
   signal goes into the scene's `[connection]` list - not into `_ready()`. The wiring then sits
   where the node does, the editor keeps it correct when the node is renamed or moved, and it
   exists before any script runs. `connect()` in code is for nodes the script instantiates itself
   (a row built in a loop, a player added by hand).
2. **A long node path in code is an antipattern.** `get_node("A/B/C/D")`, `$A/B/C` and worst of
   all a `../..` that climbs out of the node's own scene: the node is then pinned to a layout it
   does not own, and moving one container breaks it. Inside its own scene a node is reached by its
   unique name (`%Name`); anything outside comes in through the scene root's own signals and
   methods.

   A scene `[connection]` is a different matter and is *not* an offender: it stores a path from
   the scene root, and the `../../..` the editor's node dock shows is only how the editor draws
   where the receiver sits relative to the emitter. Nothing to clean there.

   What is worth cleaning is the tree itself: a container that wraps a single child earns nothing
   and lengthens every path through it, and a name has to say what the node is - `VehiclesScroll`,
   not `ScrollContainer`; `SceneryPanel`, not `ListPanel` when four lists share the screen.
3. **GDScript is interpreted and `_process` is not free.** Anything recurring is written in this
   order of preference:
   1. **C++** - a singleton connects itself to `SceneTree`'s `process_frame` and does the work
      natively (`SceneryStreamingServer::_process_streaming()`,
      `E3DRenderingServer::_process_smoke()`). No script runs per frame at all.
   2. **A `Timer`** - the engine fires the callback, so nothing is interpreted between ticks. Use
      it whenever the work is periodic and the node carrying it is a single one (an autoload, a
      system, a screen). Not when it would be one `Timer` per instance of something there are many
      of - that trades interpretation for nodes.
   3. **`_process` with a delta accumulator** - only when the work genuinely has to look at every
      frame and cannot move to C++.
   A bare `_process` that runs every frame to do a handful of calls is the thing to avoid: the
   interpreter costs more than the calls. Whichever of the three it ends up being, it is still
   bound by "Per-frame work" below.

### No identifying elements by name

An element whose owner already holds it - in a list, by reference, by its position in a menu - is
addressed through that. A name given to it only so that it can be found again is a second identity
to keep in step with the first, and a lookup by name (`for win in _windows: if win.panel == name`)
where the owner could simply index or hold the element:

```gdscript
# not this - every window named, the menu goes through a server by the name, the owner searches
HUDServer.panel_toggle(_windows[index].panel)

# this - the parent holds its windows and the menu entry is their index
var win: HUDWindow = _windows[index]
win.visible = not win.visible
```

A name is for what has no owner to hold it - a HUD element a Lua script asks for, a vehicle found by
its scenery `train_id` - not for the owner's own children.

### A case that every receiver branches on is not a parameter

**First ask what the receiver does with the value.** When every listener opens by branching on it,
the branch *is* the signal - emit one per case and let each listener connect to the one it cares
about. That removes the parameter, the branch in every receiver, and the dead half of each handler:

```gdscript
# not this - one signal carrying a side, and an "if" at the top of every listener
signal navigate_out(edge: Edge)

# this - the screen connects only what means something to it
signal navigate_left
signal navigate_right
```

**What does travel is an enum, never a number.** A mode that is passed on, stored or compared
carries its own type; a number standing for a case is semantic rubbish - the call site cannot be
read, nothing checks the value, and the reader has to open the emitter to learn what it meant:

```gdscript
# not this
func set_detail(level: int) -> void:   # 0? 2? the caller reads like arithmetic

# this
enum Detail { LOW, HIGH, OPTIMIZED }
func set_detail(level: Detail) -> void:
```

An `int` parameter is for something counted or offset - a row step, a size, an index. A direction,
a side, a mode or a state is never one. The same goes for a `bool` that names nothing at the call
site (`build(true)`): make it an enum or split the function.

Names themselves come from the vocabulary of the data and of the original engine: a `.scn` declares
`trainset`, so the code says `trainset` - not `consist`, not any other synonym the wrapper invents.

### A component names no path outside itself

What a reusable component preloads decides whether it can be moved or reused at all:

```gdscript
# not this - the component knows where it lives and who uses it
const MARKER: Shader = preload("res://ui/selection_marker.gdshader")
const UI_SOUNDS: SfxBank = preload("res://startup/ui_sounds.tres")

# this - its own asset relative to itself, the user's asset as a slot the user fills
const MARKER: Shader = preload("selection_marker.gdshader")
@export var sounds: SfxBank = null
```

A scene another component needs is attached the normal way: the consumer has a `.tscn` of its own
(a plugin's dock, a HUD window) that instances it as an `ext_resource`, and the code preloads only
that own scene, relative to itself. A `preload("../../other/thing.tscn")` that climbs out of the
component's folder is the same offence as an absolute path, and a UI built in code to avoid it
(`ScrollContainer.new()` around `Thing.new()`) is no fix either - UI is a scene:

```gdscript
# not this - a path out of the plugin's folder, the dock assembled in code
const PANEL: PackedScene = preload("../../scenery/scenery_streaming_panel.tscn")

# this - the plugin's own dock scene instances the panel scene; code preloads only its own file
const STREAMING_DOCK: PackedScene = preload("./scenery_streaming_dock.tscn")
```

A relative `preload` survives the component being renamed, moved or lifted into another project; an
absolute `res://` does not. And whatever belongs to the user rather than to the component - a sound
bank, a theme, a texture, a scene to spawn - is an `@export` its scene fills in, the same rule the
addon follows towards `demo/`.

### Exclusive state has one manager, and every state has an owner

**Exclusive state** is state only one thing may hold at a time: the focused section of a screen,
the open modal, the selected row, the active camera. It needs one manager, and the manager is the
only code that hands it out:

```gdscript
# not this - the section lights itself up, and nobody dims the one that had the focus
[connection signal="navigate_down" from="…/Vehicles" to="…/Actions" method="grab_section_focus"]

# this - the section asks, the screen decides and dims the rest
[connection signal="navigate_down" from="…/Vehicles" to="." method="focus_actions"]
```

A component that takes exclusive state for itself is not obviously wrong at the call site, and the
failure shows up as two of them holding it at once - two lit sections, two open windows.

**Every piece of state has an owner**, and it changes through a named operation of that owner
rather than by an assignment made from somewhere else:

```gdscript
# not this, somewhere in the middle of another method
_previous_section = Section.TRAINSETS

# this
reset_focus_history()
```

The name is the reason the state changed, written down once where the field lives. This holds
inside a single script as well as across objects: one writer per field, and the writing has a name.

### Do not multiply entities (DRY, KISS)

* **A private function with one call site is not a helper.** Its body belongs at that call site.
  Splitting it out hides the order of what happens and buys nothing back.
* **Never do the same thing twice to be safe.** An immediate call plus a deferred one, a direct
  call plus the same work through a signal, a condition checked in the caller and again inside the
  callee - each pair means the author did not know which one was correct. Work that out and keep
  one.
* **A wrapper that only forwards is noise.** So is a variable that is read once, a parameter that
  is always passed the same value, and state that is derivable from state already kept.

Doubling up does not make a doubtful fix more likely to work; it makes the next reader carry the
doubt as well, and it hides which of the two paths the behaviour actually comes from.

### Never create an `ensure_*` API

Not `_ensure_built()`, not `_ensure_viewport()`, not
`_ensure_sections()`, and not the same idea under a friendlier name.

A function like that re-checks and re-derives, on every call, state the code already knew at the one
moment it changed. Its cost is whatever it happens to walk, its call site says nothing about what it
changes, and the moment the state is actually set is nowhere to be found. **State is initialised
where it is created and set where it changes - once, explicitly.**

The engine's own are no pattern to copy. `ScrollContainer.ensure_control_visible()` is the example
that got this written down; where it exists, compute the value and set the property:

```gdscript
# not this
scroll.ensure_control_visible(item)

# this
scroll.scroll_vertical = int(item.position.y + item.size.y - scroll.size.y)
```

It has a failure mode on top of the cost: it reads state that a change made in the same frame has
just invalidated - a `visible` toggle, a queued re-sort - and then silently does nothing.

### Separation of concerns, enforced

This is the rule the rest of this file exists to protect.

A layer owns one kind of thing. A simulation backend owns physical quantities; a sound system owns
what is audible; a rendering server owns what is drawn; a screen owns what is on it. **State that
only one layer ever needs lives in that layer**, and the question that settles an argument is:
*if this layer were replaced wholesale, would this field go with it?*

```cpp
// not this - twelve counters on the vehicle, existing only so a sound trigger can see a change
int coupler_sound_counts[ 12 ] = {};

// this - the vehicle reports the event once, and whoever cares counts it
emit_signal( detaching ? coupler_detached_signal : coupler_attached_signal, element );
```

The failure is not untidiness. It is that the vehicle can no longer be tested without the sound
model in mind, the sound model cannot be replaced without touching the vehicle, and the field is
maintained by people who have no idea what it is for.

**The backend never appears in a public interface** - not in a method name, not in a parameter,
not in a returned type:

```cpp
// not this - "mover" is the vendored backend, and it is nobody's business out here
Dictionary get_mover_state();
void update_mover();

// this - a vehicle has state and config, and operations that change them
Dictionary get_state();
void apply_config();
```

### No magic numbers

A literal that is not self-evident from the expression around
it gets a named constant: a threshold, a limit, an index base, a conversion factor, a count, a
delay, a bitmask.

```gdscript
# not this
_coupler_events[vehicle_rid][element + 6] += 1

# this - the same code, saying why
const COUPLER_DETACH_OFFSET:int = 6
_coupler_events[vehicle_rid][COUPLER_DETACH_OFFSET + element] += 1
```

The name is where the next reader learns what the value means, and the one place it changes. Two
call sites sharing a literal by coincidence are a bug waiting for one of them to be tuned.

A value ported from the original engine carries **the original's value and a reference to where it
came from**, never a rounder number that looks safer:

```gdscript
## Original engine: Track.cpp:35 fMaxOffset
const SWITCH_MAX_OFFSET: float = 0.1
```

`FINDINGS.md` (2026-09-20) is what this rule is made of: a 0.25 m tolerance that "looked safe"
where the original used 0.02 m silently merged every double slip in the data set.

What needs no name: 0, 1, -1 and 2 used as themselves, and an index that is literally the position
being addressed.

### A getter never changes state

A `get_*`, a property getter, a `_get()` - anything a *reader*
calls - returns a value and does nothing else. It does not advance a filter, consume a flag, emit
a signal, write to another object, or build what it returns on the way out. The one exception is
`GameLog.get_logger()` ("Logging").

The failure mode is what makes this worth a rule of its own: a value computed inside a getter
depends on **how often it is read**, and nothing at the call site says so. One reader looks
correct. The bug appears when a second reader is added, or when a frame skips the read - far from
the getter, and looking like anything but a getter.

```cpp
// not this - the filter advances once per read, so the value depends on the number of readers
void TrainBrake::_do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) {
    local_brake_pressure_previous += (p_mover->LocBrakePress - local_brake_pressure_previous)
                                     * get_process_delta_time() * 5.0;
    p_state["brake_loco_pressure"] = local_brake_pressure_previous;
}

// this - the filter advances once per tick, with the tick's own delta; the getter returns it
void TrainBrake::_do_process_mover(TMoverParameters *p_mover, const double p_delta) {
    local_brake_pressure += (p_mover->LocBrakePress - local_brake_pressure) * p_delta * 5.0;
}
```

The same rule kills three more idioms seen in this codebase: consuming a flag on the Mover while
filling a state dictionary (the flag is then eaten by whoever happened to read first), emitting a
change signal from inside a fetch (the signal fires on read, not on change), and comparing against
"the previous value" pulled back out of the dictionary the fetch is filling. Change detection
belongs in the tick, against the owner's own member.

### Call a method, do not name it

When the class is known, include its header and call the
method. `Object::call("name")` gives up every check the compiler would have made - the name, the
argument count, the types - and a typo or a rename returns `null` at run time with nothing
printed. It is the same class of silent failure as a bare `[]` passed to an `Array[T]` parameter
(see `FINDINGS.md`, 2026-09-22): the call simply does not happen, and the state stays as it was.

```cpp
// not this - the header is already included two lines up, and the enum is used typed
electric_engine->call("set_pantograph_wire_voltage", TrainElectricEngine::PANTOGRAPH_FIRST, voltage);

// this
electric_engine->set_pantograph_wire_voltage(TrainElectricEngine::PANTOGRAPH_FIRST, voltage);
```

A singleton is reached the same way - a typed `static X *get_instance()` and typed methods, the
shape `RailVehicleServer`, `SceneryStreamingServer` and `SimulationServer` already have. Not a name looked
up on an `Object`, and not `get_tree()->get_root()->get_node_or_null(name)` standing in for one.

A string call is allowed only where the class genuinely cannot be known at build time - a GDScript
node that a C++ node merely hosts - and the call site says so in a comment.

### No pointers in a public API

Applies to C++ above all, and to servers first. A public method takes and returns `RID`s,
`Variant`s and `Callable`s. A handle is a `RID`, an object is an `ObjectID`, a callback is a
`Callable`.

```cpp
// not this - the server now depends on a lifetime it does not own
RID controller_create(RailVehicleController *p_controller);

// this - Godot's own servers are the reference
// (PhysicsServer3D::body_attach_object_instance_id)
RID  vehicle_create();
void vehicle_attach_object_instance_id(const RID &p_vehicle, uint64_t p_id);
```

A pointer that crosses a public boundary makes every caller responsible for a lifetime it did not
create, and the resulting dangle surfaces far from the code that caused it. Pointers stay inside
one class.

Two shapes are not this rule's business. A `RefCounted` is passed as a `Ref<T>` - it carries its
lifetime with it. And an exported node property (`PROPERTY_HINT_NODE_TYPE`, as
`SignalHeadNode.model`) stays a `Node`: Godot saves it as a `NodePath` and the inspector picks it,
which an `ObjectID` cannot do - the class keeps it as an `ObjectID` inside.

**A stored pointer is judged by what it points at, not by being one.** A pointer that only crosses
a call costs nothing; one kept in a member is a defect when its target may die first. Allowed is
only what outlives the holder by construction: its **owner**, which clears the pointer when it
releases (`VehicleComponent::train_controller_node` - a `Ref` back would be a cycle), its
**parent** in the tree, cleared on `EXIT_TREE` (`PlanarMirror3D::glass`), its own **children**, a
**singleton**, a non-Object **backend** (`TMoverParameters *`). Anything else - a sibling, a
cousin, another vehicle's part, a node handed in from outside - is held by its `ObjectID`, a
vehicle by its RID. `GenericVehicleComponent` kept its script's node as a pointer, and a freed
node was called from the next tick.

### Never work around a missing event

A value that is not there yet is an ordering defect, and the
things that look like a fix are all the same mistake:

```cpp
// not this - the placement could not finish, so it asks to be run again
if (pivot_spacing <= 0.0) {
    force_detail_refresh = true;   // "try again next frame"
    return;
}

// this - the owner announces that the configuration reached the backend, and the work happens
// there, once (RailVehicleRenderingServer places the vehicle's running gear again)
vehicle_server->connect(VehicleServer::vehicle_config_changed_signal,
                        callable_mp(this, &RailVehicleRenderingServer::_on_vehicle_config_changed));
```

The same applies to a deferred call added beside a direct one, a second `_ready()`-time retry, a
counter that gives up after N frames, and a `_process` that keeps checking whether something has
appeared. Each of them works often enough to survive review and leaves the real defect - the
operation that published its result before it had one, or never published it at all - in place.

Two questions settle it. *What produces this value, and has that operation finished?* If it has
not, the observer is being run too early: move it behind the event, or make the producing
operation complete before anything can observe it. *Is there an event for it?* If there is none,
add one at the owner - a signal that means "this has landed", not "this is about to happen" -
rather than polling for its effect.

### Wiring is not per-frame work

Resolving a path, finding a node, connecting a signal,
subscribing to anything: that happens **once**, where the node comes into being - `_enter_tree()`,
`_ready()`, or an init the owner calls. Never in `_process`/`_physics_process`, and a `_dirty`
flag around it does not make it acceptable - the flag only hides that the wiring is being
re-decided on a frame boundary.

```cpp
// not this - the subscription lives in the per-frame path
void Node::_process(double delta) {
    if (dirty) {
        vehicle = get_node_or_null(vehicle_path);
        vehicle->connect(changed_signal, callable_mp(this, &Node::_on_changed));
    }
}

// this - wired on entering the tree, and the frame does only frame work
void Node::_enter_tree() {
    vehicle = get_node_or_null(vehicle_path);
    vehicle->connect(changed_signal, callable_mp(this, &Node::_on_changed));
}
```

There is a second failure beyond the cost: a node that switches its processing off until the
thing it depends on exists can never subscribe to it, because the code that would subscribe is
the code that is not running. That deadlock is what this rule exists to prevent.

### Per-frame work

A loop in a native `_process` scales with the collection just
as badly, it only takes more objects to show.

**Running per frame is a last resort.** Before writing one, ask whether the work can be
event-driven instead: a signal, a setter, a `_dirty` flag consumed on the next change rather than
polled. If it truly has to run every frame:

* **No loops.** A per-frame loop makes the frame cost scale with the number of things iterated.
  Keep the path flat: mirror what the loop would have looked up onto the object that needs it, at
  the moment it changes, and let the frame do arithmetic on that alone (as
  `E3DRenderingServer::SmokeObject` carries its own transform and visibility instead of looking
  its instance up).
* No allocations, no `get_node()`, no string work, no `find`/`has` over a collection, no singleton
  or `ProjectSettings` lookups - resolve all of it once and cache it.
* Nothing at all while the work is idle: turn the processing off (`set_process(false)`, or
  disconnect from `process_frame`) when there is nothing to do, and back on when there is.
* If a per-frame loop is genuinely unavoidable, **bound it** - a fixed budget per frame, or the
  nearest N, the way `SceneryStreamingServer` spends a few milliseconds per frame and leaves the
  rest for the next one, or `E3DRenderingServer::_process_smoke()` visits at most
  `MAX_SMOKE_SOURCES_PER_FRAME` emitters and carries on round-robin.

### A hot path reads a component, never a dump

A vehicle publishes each value twice: as a typed getter of the component that owns it, and by name
in `VehicleServer.vehicle_dump_state()` / `vehicle_dump_config()`. The dump is **composed on
request** - every component's `_fill_state_dictionary()`, a few hundred keys - and is valid only
until the next physics step or command; the config dump is not cached at all. Reading one value
out of it costs the whole dictionary.

Anything that runs per frame, per simulation step or per tick - a sound, an AI driver, a camera -
takes the component **once**, where it resolves its vehicle (`VehicleServer.vehicle_component_get()`,
`RailVehicleServer.vehicle_component_get()`), takes it again when the vehicle gets another
controller, and calls the component's typed getters. Configuration is the component's (or the
controller's) **properties** - `wheels.track_width`, `controller.max_velocity` - not a key of the
config dump. The hot values have their own server getters (`VehicleServer.vehicle_get_speed()`,
`vehicle_get_velocity()`, `RailVehicleServer.vehicle_get_driver_cabin()`).

```gdscript
# not this - a 300-key dictionary built for one number, every frame
var ratio:float = float(VehicleServer.vehicle_dump_state(vehicle_rid).get("brake_force_ratio", 0.0))

# this - the component taken when the vehicle was resolved, its getter per frame
var ratio:float = _brake.get_force_ratio()
```

The dump is for a reader driven **by a name out of the data**: a cab element (the MMD names its
value; `CabinSystem.vehicle_state_value()`), an MMD sound trigger, the console, a test, a
diagnostic. A value a hot path needs and no component exposes gets a bound getter in C++ - never a
dump read "for now".

### A vehicle is commanded, not called

A vehicle offers two ways to make it do something: a **command** -
`VehicleServer.vehicle_send_command(vehicle_rid, "name", p1, p2)` - and the **method** behind it
on its controller or a component (`controller.cab_activation_auto()`, `doors.operate_doors()`).
The command calls the same method, and then does what the vehicle needs to know that it was
acted on (`VehicleController::send_command()`, `command_executed()`):

* **the state is renewed.** The controller's state serial goes up and the state is updated. The
  state dump (`vehicle_dump_state()`) is cached per vehicle on that serial, which otherwise moves
  only with a simulation step - so after a direct call every reader of the dump (the cab, the
  console, a test, `get_state()`) sees the vehicle as it was before it, until the next step;
* **it is announced.** `command_received` is emitted, relayed as
  `VehicleServer.vehicle_command_received` - the cab (`CabinSystem.vehicle_command_received`) and
  the scenario scripts (`maszyna.vehicle.on_command_received`) follow the vehicle's commands by it;
* **it has a name.** It is reached by the vehicle's handle alone - no controller or component held,
  no class known - so the cab's controls, the console, the AI, the player, Lua and the tests all
  act on a vehicle the same way; an unknown name is reported, and the handler's return value says
  whether the command was taken.

```cpp
// not this - the Mover acts, nobody learns of it, and the next state read is the previous one
controller->cab_controls_reset();

// this
vehicle_server->vehicle_send_command(vehicle, "cab_controls_reset");
```

So anything **outside the vehicle** that **acts on it** - a server, the cab, the player, the AI, a
script, a test - sends a command, never calls the method. The method is called directly only
**inside the vehicle's own composition**: a component acting on another component of the same
vehicle, or the controller on its components. Two things are not actions and stay calls: a
**read** (a typed getter, see "A hot path reads a component, never a dump") and **state its owner
hands down** that the vehicle only takes - `RailVehicleController::set_driver_cabin_kind()`, which
is no command because nobody but `RailVehicleServer` may decide it.

An operation another layer has to start (`cabin_leave`, `cabin_enter` for a crossing through a
gangway) is therefore registered as a command (`_register_commands()`) and bound, even when only
one server sends it. The two ways side by side: `docs/architecture.md`, "Commands and method
calls".

## Classes
1. Explicit privacy declarations
```hpp
//Example of explicit privacy modifiers and indentation inside them
class Example {
    public:
        int variable
        int variable2

    protected:
        godot::String "aaa";

    public:
        void _bind_methods();
};
```
### Declarations
1. Enums - explicit
2. Namespaces - explicit
3. Classes - explicit
```cpp
namespace godot {
    class Example {
        public:
            TrainDoor::Controls controls = TrainDoor::Controls::CONTROLS_PASSENGER;
    }
}
```

### Conversions
Always use `static_cast<type>`, don't use C-style cast

### Logging
For dev logging, use and only Godot's built-in methods.

For in-game logging, use a `GameLogger` of `GameLog`, as Python's `logging`: a logger by its id
(`GameLog.get_logger("game")`, `"ai"`, `"gameplay"`, `"scenario"`), kept in a member where a
script logs more than once (`var _log: GameLogger = GameLog.get_logger("ai")`, then
`_log.debug(...)`). There is no hierarchy of loggers. A line goes at once - nothing is kept in
memory - to the logger's handlers. `GameLog` only manages: a handler (a `GameLogHandler` with its
`min_level`, e.g. `GameLogFileHandler`) is registered by name (`GameLog.register_handler()`) and
assigned to loggers by name (`GameLog.assign_handler()`), and `GameLog` attaches it to them - one
handler may serve many loggers, and either may come first. The logger calls its handlers itself
and knows nothing of `GameLog`. The game sets up the log files (its `game.gd`); the HUD's Logs
window is a handler of its own, a tab per logger, following `logger_created` and
`logger_removing`. Nothing goes to the Godot's console nor to the developer console (`~`).

`GameLog.get_logger()` is **the one getter that changes state**, a deliberate exception to "A
getter never changes state": it creates a logger missing yet (and emits `logger_created`), as
Python's `logging.getLogger()` does, so whoever logs never has to know who made the logger first.

### Sound
This project has a sound system - the vendored `gnd-sfx` addon (`SfxBank` / `SfxEvent` /
`SfxPlayer`, `SfxPlayer3D` for positional sound). Use it. A bare `AudioStreamPlayer` with a
`preload`ed stream is not the way to add a sound, not even a single UI click.

How a bank is built:

1. One `SfxBank` resource per screen or subsystem, saved next to the assets it uses
   (e.g. the game's `startup/ui_sounds.tres`).
2. **Events are named after what happened, not after the sample or the widget**:
   `load_scenery`, `back_button`, `list_item_click`, `list_item_hover`, `apply_skin`. Code says
   `_ui_sounds.play(&"list_item_click")` and never learns which file that is - swapping the
   sample, or pointing several events at one sample, is then the bank's business alone and
   touches no code.
3. A plain one-shot needs no automation: an `SfxEvent` with `name` and one `SfxClip` in its own
   `clips` is enough. Automations are for sound driven by a continuous parameter.
4. `SfxPlayer` for non-positional sound (UI, music), `SfxPlayer3D` for anything in the world.
5. One bank may be shared by several scenes; each scene owns its own player and plays its own
   gestures, rather than reaching into another scene's player.
6. Gain belongs in the bank - in the event's own track `volume_db` or its curves. Do not add a
   global multiplier or a Project Setting on top of correctly calibrated per-event data, and
   never apply a value that a curve already normalised against (see `FINDINGS.md`, 2026-09-21).

Before changing any sound constant, dump the built bank first - every event, every clip's
`track.volume_db`, `unit_size` and `max_distance` - and look for the value that stands out. An
anomaly is visible in one listing; guessing at multipliers is not.

### A shader variant is a file

**Never build shader code at run time.** No `code.replace()` on a shader's source, no string put
together and assigned to `Shader.code`:

```gdscript
# not this - a new Shader object whose code exists nowhere on disk
var code: String = source_shader.code
code = code.replace("shader_type spatial;", "shader_type spatial;\n#define MASZYNA_ALPHA_BLEND")
code = code.replace("cull_back", "cull_disabled")
variant_shader.code = code

# this - the variant is a file, picked by the factory
const BLEND_SHADER: Shader = preload("types/normalmap_blend.gdshader")
```

* **It cannot be precompiled.** A shader that exists only as a string made at run time is
  compiled when the first object using it is drawn - in the middle of the game. Every vehicle
  instanced with such a material is a stall: the FPS drops while its pipelines compile. A shader
  that is a file is known before the game starts and can be compiled ahead.
* **Nothing shows it.** The editor has no file to open, a search for the define finds only the
  place that pastes it in, and one changed word in the source shader silently stops the
  replacement from matching.

A variant that needs another compilation - a shader that writes `ALPHA`, another cull or specular
mode - is a small `.gdshader` of its own: its `render_mode`, a `#define`, and an `#include` of the
shared code (`.gdshaderinc`). What a uniform can switch is a uniform, not a variant.

`MaszynaMaterialFactory._get_shader_variant()` still does the forbidden thing, and the blended
variant a vehicle's glass is drawn with comes out of it - so the first vehicle drawn near the
camera compiles it in the game. Replacing it with files is in `TODO.md`; nothing more is added to
it.

### Input is the project's actions, matched exactly

**Every key the game reacts to is an input action of the project, and every test of it is an
exact match.** Not a keycode, not Godot's built-in `ui_*` actions, not a loose match - none of the
three, ever.

```gdscript
# not this - a keycode, a built-in action, a loose match
if event is InputEventKey and event.keycode == KEY_ENTER:
if event.is_action_pressed("ui_text_submit"):

# this - the project's action, exact
if event.is_action_pressed("menu_activate", false, true):
```

* **An action of the project** (`demo/project.godot`, `[input]`) is named after what it does -
  `menu_activate`, `menu_back`, `toggle_fullscreen`, `cabin_next` - so the binding can be changed
  in one place and read in the input map, and the code says what happens, not which key it was.
* **Not `ui_*`.** Those are Godot's own, for its controls: a `LineEdit` takes `ui_text_submit`,
  `ui_left`, `ui_home` in its `gui_input`, a `Button` takes `ui_accept` (Space too). Binding game
  behaviour to them shares the keys with every control on the screen, and a remap of the editor's
  defaults moves the game with it.
* **Exactly** - `is_action_pressed(action, echo, true)`, and `Input.is_action_pressed(action,
  true)`. Godot's default is a loose match: an action bound to Enter is pressed by Alt+Enter,
  Ctrl+Enter and Shift+Enter as well. The starter's sections took `ui_text_submit` loosely and
  loaded a scenery on Alt+Enter, the fullscreen shortcut (`FINDINGS.md`, 2026-10-01). A shortcut
  with a modifier is another action, and only an exact match keeps it one.

### Tests
Tests use only the public interface of the tested classes - no calls to private methods
(`_name()`) and no reads/writes of private members (`_name`). If a test needs private access,
treat it as a sign that the class API should be redesigned (e.g. expose a public query, split
the logic into a separate class) instead of reaching into internals.

## C++ notes

### Singletons C++

* Never inherit from RefCounted (use plain `Object` as a base)
* Register and de-register signletons in a proper order (check dependencies)
* De-registering sequence should follow the schema:
  - call `Engine::unregister_singleton()` with checking `Engine::has_singleton()`
  - call `memdelete()` if pointer is not nullptr, then assign nullptr to the pointer
  - double check order of deallocation singletons
* `Engine::get_singleton()->get_singleton("<name>")` and/or `CustomSingleton::get_instance()` does not guarantee
  a valid instance / pointer. Always check that the singleton pointer is not `nullptr`.

### Pointers, Refs and RefCounted objects

* Prefer Refs instead of raw pointers, if applicable
* Do not mix `Ref` with `memnew()` / `memdelete()`
* Instantiate Refs in the heap:
  ```
  SomeRefCountedObject obj;
  obj.instantiate();
  ```
* Avoid circular dependencies between Refs - they may cause infinite lifecycle and memory leaks
* Use raw pointers to avoid circular dependencies

### E3DModel / E3DSubModel

* when public API of `E3DModel` or `E3DSubModel` changes,
  the `E3DModel.FORMAT_VERSION` must be updated to invalidate the E3D cache automatically

### Godot properties

* A property is stored configuration - saved, settable, part of what describes the object. Live
  state is never a property: it is a typed getter (bound with `ClassDB::bind_method`) and a key of
  the dump (`_fill_state_dictionary`, `VehicleServer.vehicle_dump_state()`), so a saved object
  carries no live values (`test_property_bindings.gd`).
* Property names exposed to Godot must use canonical `snake_case` without slashes.
* A property's setter and getter must be named `set_<property_name>` and `get_<property_name>`; custom accessor names
  are not allowed.
* Inspector grouping paths may contain slashes and must be passed only through the optional grouping argument of the
  `BIND_PROPERTY_*` macros. Grouping paths are not part of the public property name.
* Collapse overlapping grouping segments in public names, for example `power/power_source` becomes `power_source`,
  not `power_power_source`.

### MAKE_* macros

* MAKE_* macros / macros.hpp are deprecated. Use straight and readable declarations.  

### Random generation

* **Do not use C/C++ native random generation (`std::rand`, `random`, etc.). Always use Godot's built-in functions such as `godot::UtilityFunctions::randf_range`, `godot::UtilityFunctions::randi`, etc.**

## Addon architecture rules

* use sub-plugins https://docs.godotengine.org/en/stable/tutorials/plugins/editor/making_plugins.html#using-sub-plugins
* place editor plugins in `addons/libmaszyna/editor` directory
* register all types and singletons in main plugin: `addons/libmaszyna/libmaszyna.gd`
* register / unregister custom types and singletons in `_enable_plugin()` and `_disable_plugin()` methods,
  see: https://docs.godotengine.org/en/stable/tutorials/plugins/editor/making_plugins.html#registering-autoloads-singletons-in-plugins
* avoid leaking instances at runtime/editor exit
