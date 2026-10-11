extends Node3D
class_name BaseCabinTool3D

## A cabin element knows which vehicle it sits in and nothing else about it: every read and every
## manipulation goes through CabinSystem, which holds the vehicle's handle and is the only place
## that talks to the vehicle servers. There is deliberately no path to a controller here - the
## cabin root is told which vehicle it belongs to and passes that down.

var _vehicle_rid:RID
## The cabin of the Cabin3D this element sits in, taken when it enters the tree - a cab is rebuilt
## for another cabin (MaszynaDynamicTrainCabin)
var _cabin:RID
var _dirty:bool = false
## The element's time - the simulation's
var _clock:SimulationClock = SimulationClock.new()
## This control as CabinHUDMouseSystem knows it, once its mesh is found
var _mouse_control:RID = RID()

signal vehicle_rid_changed
## Emitted while the previous vehicle is still the one connected, for disconnecting from it.
signal vehicle_rid_changing

## Cabin control id (MMD label) - manipulations are reported to CabinSystem under this id; the
## registered cabin logic decides what they do to the vehicle.
@export var control_id:StringName = &""
## Which way an increase goes when the control is dragged with the mouse, per mouse axis (x: +1
## right, y: +1 down). Zero, the default, follows the control's grip on screen; the MMD catalog
## sets it for a control where that does not work.
@export var mouse_drag_signs:Vector2 = Vector2.ZERO
## The vehicle of the cab this element reads - the one it is in, the one its controls drive, or the
## pantographs' (MmdSemanticCatalog's `target`)
@export var target:CabinState.Target = CabinState.Target.OCCUPIED
## The keys of this control, named under its caption - the cab logic takes them (CabinLogic.input())
@export var hint_actions:PackedStringArray = []
## A wiper animation (MMD `wip`): the control's first submodel below it and that one's first turn
## with it, by the same angle (TGauge gt_Wiper, Gauge.cpp:469-479) - wipers, folding doors, the
## alarm chain's handle
@export var wiper_chain:bool = false

## How deep the wiper animation turns the submodels below the control (Gauge.cpp:472-478)
const WIPER_CHAIN_DEPTH:int = 2
## A step of at least this share of the friction sets an element at its target outright
## (TGauge::Update(), Gauge.cpp:366)
const FRICTION_SNAP_SHARE:float = 0.5
var _wiper_chain_meshes:Array[Node3D] = []
var _wiper_chain_bases:Array[Basis] = []


## The submodels a wiper animation turns with `mesh`, taken once its mesh is found
func _take_wiper_chain(mesh:Node3D) -> void:
    _wiper_chain_meshes.clear()
    _wiper_chain_bases.clear()
    if not wiper_chain:
        return
    var node:Node3D = mesh
    for _depth:int in range(WIPER_CHAIN_DEPTH):
        var children:Array[Node] = node.get_children().filter(func(child:Node) -> bool: return child is Node3D)
        if not children:
            return
        node = children[0]
        _wiper_chain_meshes.append(node)
        _wiper_chain_bases.append(node.basis)


## The wiper chain turned by `rotation`, the control's own turn
func _pose_wiper_chain(rotation:Basis) -> void:
    for index:int in range(_wiper_chain_meshes.size()):
        _wiper_chain_meshes[index].basis = _wiper_chain_bases[index] * rotation


## The vehicle this element sits in, as the cabin root hands it down.
func set_vehicle_rid(vehicle_rid:RID) -> void:
    if _vehicle_rid == vehicle_rid:
        return
    vehicle_rid_changing.emit()
    _vehicle_rid = vehicle_rid
    _dirty = true
    if _vehicle_rid:
        vehicle_rid_changed.emit()


func get_vehicle_rid() -> RID:
    return _vehicle_rid


## The cabin of the Cabin3D `node` sits in, RID() outside one
static func cabin_of(node:Node) -> RID:
    var ancestor:Node = node.get_parent()
    while ancestor and not ancestor is Cabin3D:
        ancestor = ancestor.get_parent()
    return (ancestor as Cabin3D).get_cabin() if ancestor else RID()


## One named value of the vehicle's state - what a control reads, being driven by a property name
## out of the MMD. The dump behind it is built once a frame for the whole cab.
func _vehicle_state_value(key:String, default_value:Variant = null) -> Variant:
    return CabinSystem.vehicle_state_value(CabinState.vehicle_of(_vehicle_rid, target), key, default_value)


## The whole of it, for the few places that genuinely read several unrelated values at once.
func _vehicle_state() -> Dictionary:
    return CabinSystem.vehicle_state(_vehicle_rid)


func _vehicle_config() -> Dictionary:
    return CabinSystem.vehicle_config(_vehicle_rid)


## The driver's hand on this control - a click, a drag by one position: the cab logic decides what it
## does, the same as for its key (CabinLogic.press()). The widget only shows what follows.
func press() -> void:
    if _cab_logic():
        _cab_logic().press(control_id)


func release() -> void:
    if _cab_logic():
        _cab_logic().release(control_id)


func increase() -> void:
    if _cab_logic():
        _cab_logic().increase(control_id)


func decrease() -> void:
    if _cab_logic():
        _cab_logic().decrease(control_id)


## The logic of the cab this control is in, when it has one and the control reports
func _cab_logic() -> CabinLogic:
    return CabinSystem.vehicle_get_cab_logic(_vehicle_rid) if _vehicle_rid and control_id else null


# _notification runs on every class of the hierarchy, unlike _ready/_enter_tree overridden below.
func _notification(what:int) -> void:
    if what == NOTIFICATION_ENTER_TREE:
        _cabin = cabin_of(self)
        CabinSystem.control_changed.connect(_on_cabin_control_changed)
    elif what == NOTIFICATION_EXIT_TREE:
        CabinSystem.control_changed.disconnect(_on_cabin_control_changed)
        if _mouse_control.is_valid():
            CabinHUDMouseSystem.control_free(_mouse_control)
            _mouse_control = RID()


## Shows a control value set in CabinSystem (e.g. from the console) without reporting it back.
func _on_cabin_control_changed(cabin:RID, p_control_id:StringName, value:Variant) -> void:
    if not p_control_id == control_id or not _cabin or not cabin == _cabin:
        return
    _apply_control_value(value)


func _apply_control_value(_value:Variant) -> void:
    pass


## Makes `mesh` operable by mouse with this control's own operations; an empty `increase` leaves
## it click-only. `actions` are the keys that do the same, shown next to the caption
## (drivermode.cpp:373). `step_rotation` (degrees, applied X, Y, Z as the widgets animate) and
## `step_position` are how far one increase moves the mesh, so a drag follows the grip. A valid
## `drag` takes the drag's travel in pixels in place of the increase/decrease steps.
func _set_mouse_control(mesh:Node3D, actions:PackedStringArray, pressed:Callable, released:Callable,
        increase:Callable, decrease:Callable, step_rotation:Vector3, step_position:Vector3,
        drag:Callable = Callable()) -> void:
    if _mouse_control.is_valid():
        CabinHUDMouseSystem.control_free(_mouse_control)
    var hints:PackedStringArray = []
    for action_name:String in actions:
        if action_name and InputMap.has_action(action_name):
            for event:InputEvent in InputMap.action_get_events(action_name):
                hints.append(InputEventNames.event_name(event))
    var step_basis:Basis = Basis(Vector3.RIGHT, deg_to_rad(step_rotation.x)) \
            * Basis(Vector3.UP, deg_to_rad(step_rotation.y)) \
            * Basis(Vector3.FORWARD, deg_to_rad(step_rotation.z))
    _mouse_control = CabinHUDMouseSystem.control_create(mesh.get_instance_id(),
            MmdCabControlCaptions.caption(control_id), " / ".join(hints), pressed, released, increase, decrease,
            step_basis, step_position, drag, mouse_drag_signs)


## A two-state control's state under its caption - msgids the tooltip's Label translates, from the
## wrapper's own catalogue (addons/libmaszyna/translations)
const STATE_ON:String = "on"
const STATE_OFF:String = "off"


## What the control shows now, for the caption under the cursor (CabinHUDMouseSystem).
func _set_mouse_state(state:String) -> void:
    if _mouse_control.is_valid():
        CabinHUDMouseSystem.control_set_state(_mouse_control, state)


func _exit_tree() -> void:
    set_vehicle_rid(RID())
    _dirty = true


## How far an element moves towards its target in a step of `delta` seconds at `animation_speed`
## (1 / the MMD friction): TGauge::Update() (Gauge.cpp:364-376) - no friction, or a step of half of
## it or more, sets it outright ("zabezpieczenie przed oscylacjami dla dlugich czasow"), so a slow
## frame or a fast simulation never throws it past the target into a spin
static func friction_weight(delta:float, animation_speed:float) -> float:
    var weight:float = delta * animation_speed
    if animation_speed <= 0.0 or weight >= FRICTION_SNAP_SHARE:
        return 1.0
    return weight


func _process_dirty(delta):
    pass


func _process_tool(delta):
    pass


func _process(delta):
    var seconds:float = _clock.advance(delta)
    if _dirty:
        _dirty = false
        _process_dirty(seconds)
    _process_tool(seconds)
