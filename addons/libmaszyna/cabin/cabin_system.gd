extends Node

## Cabin layer (#94) - the counterpart of the original engine's TTrain (Train.cpp), kept separate
## from vehicle commands (VehicleServer.vehicle_send_command), which execute on the vehicle
## immediately.
##
## Holds a CabinState per cabin and a registry of cabin control handlers. A cabin is its
## VehicleServer handle (RailVehicleServer knows which of the vehicle's cabs it is), a vehicle its
## VehicleServer handle - never its scenery name, which two vehicles may share and one may lack.
## Cabin controls only report manipulations through act(); the handlers are registered by the
## CabinLogic attached to the vehicle (e.g. LegacyCabinLogic) - for the cabin its driver sits in -
## and translate them into vehicle commands. CabinSystem itself has no cabin logic and forwards
## nothing by default.

signal control_changed(cabin:RID, control_id:StringName, value:Variant)
## A command reached the vehicle, from wherever - the console, a keybind, another cab. Relayed
## here so a cabin element can react without ever holding the vehicle itself.
signal vehicle_command_received(vehicle_rid:RID, command:String, p1:Variant, p2:Variant)
## The vehicle has a cab scene to show now (vehicle_set_cabin_scene()) - one its builder hands over
## after the vehicle has its simulation, and may already be driven
signal vehicle_cabin_scene_changed(vehicle_rid:RID)
## The interior of a vehicle's cab was built and is shown now - in the tree and drawn
## (cabin_show(), vehicle_get_cabin()); once per interior, showing another cab of the vehicle
## rebuilds the same one. Only the player's cab is ever shown, so what hangs on it (a rain
## exclusion volume) exists once - never emit this for a cab scene merely set
## (vehicle_set_cabin_scene()): one volume per vehicle of a scenery was a quarter of the frame
## (docs/findings-archive.md, 2026-10-03 hundreds of vehicles)
signal vehicle_cabin_built(vehicle_rid:RID)
## The light of a cabin shines at another level (cabin_set_light_level())
signal cabin_light_level_changed(cabin:RID, level:float)
## The instrument light of a cabin came on or went out (cabin_set_instrument_light_enabled())
signal cabin_instrument_light_changed(cabin:RID, enabled:bool)
## The dashboard light of a cabin came on or went out (cabin_set_dashboard_light_enabled())
signal cabin_dashboard_light_changed(cabin:RID, enabled:bool)
## The timetable light of a cabin came on or went out (cabin_set_timetable_light_enabled())
signal cabin_timetable_light_changed(cabin:RID, enabled:bool)
## A radio message sent from `position`: heard on the radio of the player's cab tuned to
## `channel`, within `reach` [m] of it when that is positive (simulation::radio_message(),
## simulation.cpp:506); `transcript` is what it says, null when unknown
signal radio_message_sent(message:SfxEvent, transcript:Transcript, channel:int, position:Vector3, reach:float)

## Manipulations a control can report (Train.cpp OnCommand_* press/release/repeat/set events).
const ACTIONS:Array[StringName] = [&"increase", &"decrease", &"hold", &"release", &"toggle", &"set"]

## By cabin: its state, its control handlers by id, the callables run with the simulation
var _states:Dictionary[RID, CabinState] = {}
var _controls:Dictionary[RID, Dictionary] = {}
var _processes:Dictionary[RID, Array] = {}
var _cab_logics:Dictionary[RID, CabinLogic] = {}
## The cab interior each vehicle's crew sits in, and the one shown, by instance id
var _cabin_scenes:Dictionary[RID, PackedScene] = {}
var _cabins:Dictionary[RID, int] = {}
## Values of a vehicle's state the cab computes rather than reads, by name (state_computed_value_register())
var _state_computed_values:Dictionary[RID, Dictionary] = {}
var _log:GameLogger = GameLog.get_logger("game")


func _ready() -> void:
    SimulationServer.simulation_advanced.connect(_on_simulation_advanced)
    VehicleServer.vehicle_command_received.connect(_on_vehicle_command_received)
    RailVehicleServer.vehicle_driver_cabin_changed.connect(_on_vehicle_driver_cabin_changed)
    VehicleServer.vehicle_freed.connect(_on_vehicle_freed)


func _exit_tree() -> void:
    SimulationServer.simulation_advanced.disconnect(_on_simulation_advanced)
    VehicleServer.vehicle_command_received.disconnect(_on_vehicle_command_received)
    RailVehicleServer.vehicle_driver_cabin_changed.disconnect(_on_vehicle_driver_cabin_changed)
    VehicleServer.vehicle_freed.disconnect(_on_vehicle_freed)


## A freed vehicle takes its cabins along - a handle is never reused for another one.
func _on_vehicle_freed(vehicle_rid:RID) -> void:
    if _cab_logics.has(vehicle_rid):
        _cab_logics[vehicle_rid].unregister()
        _cab_logics.erase(vehicle_rid)
    _cabin_scenes.erase(vehicle_rid)
    _cabins.erase(vehicle_rid)
    _state_computed_values.erase(vehicle_rid)
    for cabin:RID in _states.keys():
        if _states[cabin].vehicle_rid == vehicle_rid:
            _states.erase(cabin)
            _controls.erase(cabin)
            _processes.erase(cabin)


func _on_vehicle_command_received(vehicle_rid:RID, command:String, p1:Variant, p2:Variant) -> void:
    vehicle_command_received.emit(vehicle_rid, command, p1, p2)


## The driver moved, sat down or got up: the controls are those of the driver's cabin now, none
## while nobody drives - the original keeps a TTrain only for a driven train, and a cab of every
## vehicle at work costs every frame
func _on_vehicle_driver_cabin_changed(vehicle_rid:RID, cabin:RID) -> void:
    var logic:CabinLogic = _cab_logics.get(vehicle_rid)
    if not logic:
        return
    logic.unregister()
    if cabin.is_valid():
        logic.register(vehicle_rid, cabin)


## The whole vehicle's state, by name. VehicleServer builds it once per step and keeps it until
## the step or a command moves it on, so the dozens of elements of a cab asking in one frame share
## one dump. An element that reads one value often enough to care takes its component instead
## (vehicle_component() below).
func vehicle_state(vehicle_rid:RID) -> Dictionary:
    return VehicleServer.vehicle_dump_state(vehicle_rid) if vehicle_rid.is_valid() else {}


## One named value of the vehicle's state. This is what a cabin element wants: it is driven by a
## property name out of the MMD and reads exactly one of them, so handing it the whole dump only
## gives it something to hold wrongly.
## A value the cab computes (state_computed_value_register()) is answered by its callable.
func vehicle_state_value(vehicle_rid:RID, key:String, default_value:Variant = null) -> Variant:
    var computed:Callable = _state_computed_values.get(vehicle_rid, {}).get(key, Callable())
    if computed.is_valid():
        return computed.call()
    return vehicle_state(vehicle_rid).get(key, default_value)


## The cab shows a value of the vehicle's state as it computes it, not as the vehicle has it - as
## TTrain::Update feeds a gauge (Train.cpp:9451-9458). callable() -> Variant, asked by
## vehicle_state_value() for `key` of this vehicle until state_computed_value_unregister().
func state_computed_value_register(vehicle_rid:RID, key:String, callable:Callable) -> void:
    if not _state_computed_values.has(vehicle_rid):
        _state_computed_values[vehicle_rid] = {}
    _state_computed_values[vehicle_rid][key] = callable


func state_computed_value_unregister(vehicle_rid:RID, key:String) -> void:
    _state_computed_values.get(vehicle_rid, {}).erase(key)


## Whether the vehicle has its low voltage, without which every lamp of its cab is dark -
## lowvoltagepower (Train.cpp:8843), handed to every lamp (TButton::Update(Power), Button.cpp:126)
func vehicle_has_low_voltage(vehicle_rid:RID) -> bool:
    var state:Dictionary = vehicle_state(vehicle_rid)
    return state.get("power24_available", false) or state.get("power110_available", false)


func vehicle_config(vehicle_rid:RID) -> Dictionary:
    return VehicleServer.vehicle_dump_config(vehicle_rid) if vehicle_rid.is_valid() else {}


## One component of the vehicle, by kind - for an element that reads a value often enough to want
## the typed property rather than the dump.
func vehicle_component(vehicle_rid:RID, type:VehicleComponentType.Type) -> VehicleComponent:
    return VehicleServer.vehicle_component_get(vehicle_rid, type) if vehicle_rid.is_valid() else null


## The cab logic of a vehicle; null detaches it. One per vehicle, whoever drives it - the player's
## cab and the AI act on the same controls - registered for the cabin its driver sits in
## (RailVehicleServer.vehicle_get_driver_cabin()) while somebody drives it.
func vehicle_attach_cab_logic(vehicle_rid:RID, logic:CabinLogic) -> void:
    if _cab_logics.has(vehicle_rid):
        _cab_logics[vehicle_rid].unregister()
        _cab_logics.erase(vehicle_rid)
    if not logic:
        return
    _cab_logics[vehicle_rid] = logic
    var cabin:RID = RailVehicleServer.vehicle_get_driver_cabin(vehicle_rid)
    if cabin.is_valid():
        logic.register(vehicle_rid, cabin)


func vehicle_get_cab_logic(vehicle_rid:RID) -> CabinLogic:
    return _cab_logics.get(vehicle_rid)


## The cab interior of a vehicle, a scene rooted in a Cabin3D - what cabin_show() builds.
## Only a view: the cab logic is the vehicle's (vehicle_attach_cab_logic()).
func vehicle_set_cabin_scene(vehicle_rid:RID, scene:PackedScene) -> void:
    _cabin_scenes[vehicle_rid] = scene
    vehicle_cabin_scene_changed.emit(vehicle_rid)


func vehicle_get_cabin_scene(vehicle_rid:RID) -> PackedScene:
    return _cabin_scenes.get(vehicle_rid)


## The interior of the cabin built under `parent`, which rides on its vehicle (the caller mounts it,
## RailVehicleRenderingServer.vehicle_mount_node()) - the cab keeps its own place in the vehicle's
## frame; it is built within add_child() (Cabin3D's cabin_ready comes from its NOTIFICATION_READY),
## so it returns built. One interior a vehicle is shown: the one of another of its cabins is rebuilt
## for this one. Null for a vehicle without a cab.
func cabin_show(cabin_rid:RID, parent:Node) -> Cabin3D:
    var vehicle_rid:RID = VehicleServer.cabin_get_vehicle(cabin_rid)
    var shown:Cabin3D = vehicle_get_cabin(vehicle_rid)
    if shown:
        shown.set_cabin(cabin_rid)
        return shown
    var scene:PackedScene = _cabin_scenes.get(vehicle_rid)
    if not scene:
        push_warning("CabinSystem: the vehicle has no cab interior to show")
        return null
    var cabin:Cabin3D = scene.instantiate() as Cabin3D
    if not cabin:
        push_error("CabinSystem: the root of a cabin scene must be a Cabin3D")
        return null
    _cabins[vehicle_rid] = cabin.get_instance_id()
    parent.add_child(cabin)
    # a cabin holds the handle of its cabin, its vehicle comes with it, and takes everything else
    # from here - told once it is in the tree, because building its interior puts nodes there
    cabin.set_cabin(cabin_rid)
    vehicle_cabin_built.emit(vehicle_rid)
    return cabin


## The cab interior freed; a camera put into it is to be taken out first
func vehicle_hide_cabin(vehicle_rid:RID) -> void:
    var cabin:Cabin3D = vehicle_get_cabin(vehicle_rid)
    _cabins.erase(vehicle_rid)
    if not cabin:
        return
    cabin.get_parent().remove_child(cabin)
    cabin.queue_free()


## The cab interior while it is shown, else null
func vehicle_get_cabin(vehicle_rid:RID) -> Cabin3D:
    return instance_from_id(_cabins.get(vehicle_rid, 0)) as Cabin3D


## The cab light of a cabin at `level` (0..1); the vehicle's drawing follows cabin_light_level_changed
func cabin_set_light_level(cabin:RID, level:float) -> void:
    var state:CabinState = get_cabin_state(cabin)
    if state.light_level == level:
        return
    state.light_level = level
    cabin_light_level_changed.emit(cabin, level)


func cabin_get_light_level(cabin:RID) -> float:
    var state:CabinState = _states.get(cabin)
    return state.light_level if state else 0.0


func cabin_set_instrument_light_enabled(cabin:RID, enabled:bool) -> void:
    var state:CabinState = get_cabin_state(cabin)
    if state.instrument_light_enabled == enabled:
        return
    state.instrument_light_enabled = enabled
    cabin_instrument_light_changed.emit(cabin, enabled)


func cabin_get_instrument_light_enabled(cabin:RID) -> bool:
    var state:CabinState = _states.get(cabin)
    return state.instrument_light_enabled if state else false


func cabin_set_dashboard_light_enabled(cabin:RID, enabled:bool) -> void:
    var state:CabinState = get_cabin_state(cabin)
    if state.dashboard_light_enabled == enabled:
        return
    state.dashboard_light_enabled = enabled
    cabin_dashboard_light_changed.emit(cabin, enabled)


func cabin_get_dashboard_light_enabled(cabin:RID) -> bool:
    var state:CabinState = _states.get(cabin)
    return state.dashboard_light_enabled if state else false


func cabin_set_timetable_light_enabled(cabin:RID, enabled:bool) -> void:
    var state:CabinState = get_cabin_state(cabin)
    if state.timetable_light_enabled == enabled:
        return
    state.timetable_light_enabled = enabled
    cabin_timetable_light_changed.emit(cabin, enabled)


func cabin_get_timetable_light_enabled(cabin:RID) -> bool:
    var state:CabinState = _states.get(cabin)
    return state.timetable_light_enabled if state else false


func get_cabin_state(cabin:RID) -> CabinState:
    if not _states.has(cabin):
        _states[cabin] = CabinState.new(cabin)
    return _states[cabin]


## handler(state:CabinState, action:StringName, value:Variant) -> Variant
# FIXME(#184, #28): imperative, per-cabin registration mirroring VehicleController.register_command;
# cabin behaviours should rather declare the control ids/actions they handle.
func register_control(cabin:RID, control_id:StringName, handler:Callable) -> void:
    if not _controls.has(cabin):
        _controls[cabin] = {}
    _controls[cabin][control_id] = handler


func unregister_control(cabin:RID, control_id:StringName, handler:Callable) -> void:
    var controls:Dictionary = _controls.get(cabin, {})
    if controls.get(control_id) == handler:
        controls.erase(control_id)


func has_control(cabin:RID, control_id:StringName) -> bool:
    return _controls.get(cabin, {}).has(control_id)


## Control ids with a registered handler in the cabin, sorted.
func get_controls(cabin:RID) -> Array:
    var controls:Array = _controls.get(cabin, {}).keys()
    controls.sort()
    return controls


## callable(state:CabinState, delta:float) - called every frame while registered
func register_process(cabin:RID, callable:Callable) -> void:
    get_cabin_state(cabin)
    if not _processes.has(cabin):
        _processes[cabin] = []
    _processes[cabin].append(callable)


func unregister_process(cabin:RID, callable:Callable) -> void:
    var processes:Array = _processes.get(cabin, [])
    processes.erase(callable)


## Reports a manipulation of a cabin control; returns the handler's result (#43), or null when
## no handler is registered for the control.
func act(cabin:RID, control_id:StringName, action:StringName, value:Variant = null) -> Variant:
    var vehicle_name:String = VehicleServer.vehicle_get_name(VehicleServer.cabin_get_vehicle(cabin))
    if not action in ACTIONS:
        _log.error("%s: Unknown cabin action: %s" % [vehicle_name, action])
        return null
    var handler:Callable = _controls.get(cabin, {}).get(control_id, Callable())
    if not handler.is_valid():
        _log.error("%s: Unknown cabin control: %s" % [vehicle_name, control_id])
        return null
    return handler.call(get_cabin_state(cabin), action, value)


func get_control(cabin:RID, control_id:StringName) -> Variant:
    return get_cabin_state(cabin).get_value(control_id)


## A scenery's radio message, to the cab radio of whoever listens (radio_message_sent)
func send_radio_message(
    message:SfxEvent, transcript:Transcript, channel:int, position:Vector3, reach:float
) -> void:
    radio_message_sent.emit(message, transcript, channel, position, reach)


func get_state(cabin:RID) -> Dictionary:
    return get_cabin_state(cabin).values.duplicate()


## The cabs' own timing runs on the simulation's clock, as the original's TTrain::Update(dt) does
## with the scaled time (Train.cpp:8436-8474): a relay held for its delay at x10 closes in a tenth
## of the real time, and nothing runs while paused
func _on_simulation_advanced(seconds:float) -> void:
    for cabin:RID in _processes:
        var state:CabinState = _states.get(cabin)
        for callable:Callable in _processes[cabin].duplicate():
            callable.call(state, seconds)
