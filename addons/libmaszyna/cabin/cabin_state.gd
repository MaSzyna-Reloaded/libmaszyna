extends RefCounted
class_name CabinState

## State of one cabin - a VehicleServer cabin, of the vehicle it is attached to. Owned by
## CabinSystem and handed to every registered cabin control handler, which may read and modify it.
##
## The vehicle is reached only by its VehicleServer handle (commands and state), never through
## the Mover directly.
##
## A cab works three vehicles, as the original's TTrain does: the one it is in (mvOccupied), the one
## its controls drive (mvControlled, FindPowered() - an EMU's motor car) and the one carrying its
## pantographs (mvPantographUnit). A control names its own (MmdSemanticCatalog's `target`, a cabin
## logic's constant), as the original's OnCommand_* handler does.

enum Target {
    ## mvOccupied: the vehicle the cab is in
    OCCUPIED,
    ## mvControlled: the vehicle its controls drive
    CONTROLLED,
    ## mvPantographUnit: the vehicle carrying its pantographs, the controlled one without it
    PANTOGRAPH_UNIT,
}

## A light of the cab itself, as a cab element follows it
enum Light {
    NONE,
    ## the cab light, at its level (CabinSystem.cabin_light_level_changed)
    CAB,
    ## the instrument light (CabinSystem.cabin_instrument_light_changed)
    INSTRUMENT,
    ## the dashboard light (CabinSystem.cabin_dashboard_light_changed)
    DASHBOARD,
    ## the timetable light (CabinSystem.cabin_timetable_light_changed)
    TIMETABLE,
}

var vehicle_rid:RID
var cabin:RID
## control_id -> current value of the physical control (button pressed, switch position, ...)
var values:Dictionary = {}
## Private state of the cabin logic behaviours (timers, state machines, ...)
var data:Dictionary = {}
## The level (0..1) the cab light shines at - TTrain::Cabine[].LightLevel (Train.cpp:8436-8453).
## Written only by CabinSystem.cabin_set_light_level().
var light_level:float = 0.0
## Whether the instrument light is lit - TTrain::InstrumentLightActive. Written only by
## CabinSystem.cabin_set_instrument_light_enabled().
var instrument_light_enabled:bool = false
## Whether the dashboard light is lit - TTrain::DashboardLightActive. Written only by
## CabinSystem.cabin_set_dashboard_light_enabled().
var dashboard_light_enabled:bool = false
## Whether the timetable light is lit - TTrain::TimetableLightActive. Written only by
## CabinSystem.cabin_set_timetable_light_enabled().
var timetable_light_enabled:bool = false


func _init(p_cabin:RID) -> void:
    cabin = p_cabin
    vehicle_rid = VehicleServer.cabin_get_vehicle(p_cabin)


func get_value(control_id:StringName, default:Variant = null) -> Variant:
    return values.get(control_id, default)


func set_value(control_id:StringName, value:Variant) -> void:
    if values.has(control_id) and values[control_id] == value:
        return
    values[control_id] = value
    CabinSystem.control_changed.emit(cabin, control_id, value)


## Whether a push control ends up pressed by the manipulation: held, or toggled to the given value -
## a toggle without one (e.g. from the console) flips its current position.
func is_pressed(control_id:StringName, action:StringName, value:Variant) -> bool:
    if action == &"toggle":
        return not get_value(control_id, false) if value == null else bool(value)
    return action == &"hold"


## The vehicle's state, through the cab's own system. VehicleServer builds the dump once per
## step for every element of every cab, and a read right after a command is not stale either - the
## dump is keyed on the vehicle's command counter as well as on the step (see `FINDINGS.md`,
## 2026-09-23).
func vehicle_state() -> Dictionary:
    return CabinSystem.vehicle_state(vehicle_rid)


## The vehicle of a target of the cab in `occupied`
static func vehicle_of(occupied:RID, target:Target) -> RID:
    match target:
        Target.CONTROLLED:
            return RailVehicleServer.vehicle_find_powered(occupied)
        Target.PANTOGRAPH_UNIT:
            var carrier:RID = RailVehicleServer.vehicle_find_pantograph_carrier(occupied)
            # without one the controlled vehicle stands in (Driver.cpp:5901-5904)
            return carrier if carrier.is_valid() else RailVehicleServer.vehicle_find_powered(occupied)
    return occupied


## One named value of a target's state - what a control driven by an MMD property name reads
func vehicle_state_value(key:String, default_value:Variant = null, target:Target = Target.OCCUPIED) -> Variant:
    return CabinSystem.vehicle_state_value(vehicle_of(vehicle_rid, target), key, default_value)


## One component of a target - what a cab logic reads on every step: the dump is composed anew
## for each read in a step, which for a scenery's driven vehicles was half of the frame
## (docs/findings-archive.md, 2026-10-03 hundreds of vehicles)
func vehicle_component(type:VehicleComponentType.Type, target:Target = Target.OCCUPIED) -> VehicleComponent:
    return CabinSystem.vehicle_component(vehicle_of(vehicle_rid, target), type)


## A command of a control, to its target
func send_vehicle_command(
    command:String, p1:Variant = null, p2:Variant = null, target:Target = Target.OCCUPIED
) -> Variant:
    return VehicleServer.vehicle_send_command(vehicle_of(vehicle_rid, target), command, p1, p2)
