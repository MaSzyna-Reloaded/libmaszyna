---
layout: page
title: "Overall Architecture"
---

* Godot Engine as the core of the project: **Rendering**,
  **Multiplayer**, **Sound**, **Assets Pipeline**, **Utilities**,
  **Node system**, **Resources handling**, **Scene system**,
  **Input system**, **GDExtension** as programming interface
* High-Level API - servers addressed by RIDs - for communication between all high-level components and external
  systems
* Godot nodes as the *presence* of a component in the scene tree: a thin proxy that owns a handle, not the
  implementation



### Diagrams

#### Overall architecture

![Overall architecture](assets/overall-architecture-1.png)

#### Communication example diagram

A conceptual picture: "LegacyTrain" stands for a vehicle simulated on the vendored Mover, see
[Wrapping the MOVER](wrapping-mover.html) for how it is actually built.

![Overall architecture](assets/overall-architecture-2.png)

### High-Level Components

High-Level Components should be identified as a complete and standalone
objects in the game, which encapsulates it's behavior and rendering.
They should work just by adding them to the scene.

Examples of High-Level Components in the game:
- train vehicle
- complete cabin for a vehicle
- car
- building or element of the railway infrastructure (i.e. semaphore)
- Quake-like Console
- game's scenario system
- UART communication system

#### Nodes are proxies, servers hold the implementation

A High-Level Component is not implemented by its nodes. The node standing in the scene is a proxy: it owns a handle
(an `RID`) to an object held by a server and builds that object, and the object is what simulates, configures and
answers commands. A vehicle is the reference case:

* `VehiclePhysicsNode` is the vehicle's presence in the tree. It builds the vehicle from a copy of its description (the
  parsed `.fiz`, a `VehicleController` with its components, supplied by `MaszynaRailVehiclePhysicsNode`), owns its
  `VehicleServer` handle and frees both with itself.
* The vehicle - a `VehicleController` and the `VehicleComponent`s it is made of - is a `Resource`, not a node: its
  properties are its stored configuration. It is held through `VehicleServer`, stepped by its implementation once per
  rendered frame, and reached by its RID.
* What draws the vehicle (`RailVehicleRenderingServer`), its cabin and its sounds read the vehicle's components; they
  do not own them. The rendering server holds where the vehicle stands; a node of another layer - a cab interior, a
  sound emitter, the node of a vehicle assembled by hand - rides on it (`vehicle_mount_node()`).
* A vehicle needs no node at all. A scenery loaded from MaSzyna data builds its vehicles and trainsets through the
  servers and holds them by their handles: `MaszynaLegacyVehicleSystem.vehicle_create()` makes a vehicle from a
  `dynamic`, `RailVehicleServer.trainset_*` stands a trainset on its track and couples it. `MaszynaRailVehicle3D` and
  `TrainSet3D` are for a vehicle or a trainset placed by hand in a scene - proxies over the same operations.

The class diagrams are in [Wrapping the MOVER](wrapping-mover.html#class-diagrams).

#### Communication between game objects

The communication between High-Level Components (a game objects) **must be**
implemented through **High-Level API** like **RailVehicleServer**.

The communication is based on commands identified by unique string names. A vehicle and each of its components
register the commands they answer to on the vehicle itself; `RailVehicleServer` dispatches a command to the vehicle a
RID names. Every vehicle handles its own subset of all commands, which can be inspected at runtime
(`vehicle_get_commands`). Adding and removing commands is also possible at runtime, because commands are dynamic - a
disabled component gives its commands back.

A vehicle is addressed by its `VehicleServer` handle (RID); `RailVehicleServer` knows the same handle for what is rail
(the track, the couplers, the rail component kinds). One known only by its scenery name - which may be empty or shared
by several vehicles - is found first. For example, to enable battery in the `train1` vehicle:
```gdscript
var vehicle: RID = VehicleServer.vehicle_get_rid_by_name("train1")
VehicleServer.vehicle_send_command(vehicle, "battery", true)
```

A command runs on the vehicle immediately, but its effect may take time (i.e. some systems must spin up). To see
where the vehicle is, read the component that owns the value - a typed getter, read straight from the simulation
(a component's properties are its stored configuration, never its live state):

```gdscript
var switches: RailVehicleSwitches = RailVehicleServer.vehicle_component_get(
        vehicle, RailVehicleComponentType.COMPONENT_SWITCHES)
var sanding: bool = switches.get_sand_active()
```

`VehicleServer.vehicle_dump_state(vehicle)` returns everything the vehicle publishes in one `Dictionary`. It is
built at most once per step and is meant for a console, a test or a diagnostic - not for a reader that wants one
value every frame.

The cabin is a layer of its own: `CabinSystem` (the counterpart of the original engine's `TTrain`) receives what the
player does with cab controls and turns it into vehicle commands through `RailVehicleServer`.


#### Internal communication

High-Level Components are usually built from many sub-components, which will
handle a subset of a logic or rendering. The communication between these
components is private, should be fastest as possible and
straightforward. Because HLC (High-Level Component) know it's internal
structure, it can communicate with subcomponents using direct method
calls, accessing properties, signals.

Inside a vehicle that means typed calls on its components: a component reaches the vehicle it belongs to with
`get_controller()` and the other components through it, the way the wrapper's own components do.

A script adds its own part to a vehicle through a second proxy, `GenericVehicleComponentNode`, placed under the
vehicle's `VehiclePhysicsNode`. The node puts a `GenericVehicleComponent` into the vehicle, and the component calls
back into the script:

```gdscript
extends GenericVehicleComponentNode

var locked: bool = false

func _ready():
    register_command("lock_power", self._on_lock_power)

func _on_lock_power(p1, _p2):
    locked = true if p1 else false

func _process_component(delta):
    var controller: RailVehicleController = get_controller()
    if not locked and controller.get_power24_available():
        operate_something()

func operate_something():
    # some logic here
    pass
```

`demo/examples/powered_train_part.gd` is a complete example. What the script declares, other parts of the same
vehicle can call directly. Any other game object (HLC) goes through the **High-Level API**:

```gdscript
VehicleServer.vehicle_send_command(vehicle_rid, "lock_power", true)
```

This approach hides internal structure of the vehicle and creates a
stable interface between game components.

#### Commands and method calls

Every command is a method of the vehicle's controller or of one of its components, registered under a name
(`_register_commands()`). The same action can therefore be done two ways, and they are not the same:

| | `VehicleServer.vehicle_send_command(vehicle, "name", p1, p2)` | `controller.method()` / `component.method()` |
|---|---|---|
| What it needs | the vehicle's handle (RID) | the controller or the component itself, and its class |
| The method runs | yes - `VehicleController::send_command()` calls it with as many arguments as it takes | yes |
| The state dump | renewed: the controller's state serial goes up and the state is updated (`command_executed()`) | **stale** until the next simulation step - `vehicle_dump_state()` is cached on that serial |
| Who learns of it | everyone following the vehicle: `command_received`, relayed as `VehicleServer.vehicle_command_received` - the cab (`CabinSystem.vehicle_command_received`), the scenario scripts (`maszyna.vehicle.on_command_received`) | nobody |
| An unknown name | reported (`Unknown command`), the return value says whether the command was taken | a compile or a parse error |

So the choice follows from who acts:

* **Outside the vehicle** - a server, the cab, the player, the AI, a script, the console, a test - an action on the
  vehicle is a **command**. A server in C++ that holds the controller still sends the command:
  `RailVehicleServer.person_change_cabin()` switches the cab off and on with `cab_deactivation_auto`,
  `cab_controls_reset`, `cab_activation_auto`, `cabin_leave` and `cabin_enter`, never with the controller's methods.
* **Inside the vehicle's own composition** - a component acting on another component of the same vehicle, the
  controller on its components - a **direct call** (the "Internal communication" above).
* **A read** is a call of a typed getter from anywhere (`vehicle_component_get()`, see above); it changes nothing.
* **State handed down by its owner**, which the vehicle only takes and nobody else may decide, is a C++-only call and
  no command: `RailVehicleController::set_driver_cabin_kind()` is called by `RailVehicleServer` alone.

An operation another layer has to start is registered as a command and bound, even when one server alone sends it.
The rule and its reasons are in `CODE_STYLE.md` ("A vehicle is commanded, not called"); what it cost when broken is in
the [findings archive](findings-archive.html) (2026-10-05, the cab change in `RailVehicleServer`).
