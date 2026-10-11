extends VBoxContainer


## The vehicle this panel shows, handed to it by the HUD - never looked up by a path into
## somebody else's scene.
var vehicle:RID = RID():
    set(x):
        if not vehicle == x:
            vehicle = x
            _do_update()

## Which vehicle of the occupied one's unit the section shows, as the cab's controls pick theirs
## (CabinState.Target): the engine is the controlled vehicle's, the pantographs the carrier's
@export var target:CabinState.Target = CabinState.Target.OCCUPIED
## The vehicle of `target`, resolved whenever the occupied vehicle changes
var target_vehicle:RID = RID()

## Taken once per vehicle rather than looked up per frame - a component is a live view on the
## vehicle, valid for as long as the vehicle is.
var universal_controller:RailVehicleUniversalController


func _ready() -> void:
    _do_update()

func _do_update():
    target_vehicle = CabinState.vehicle_of(vehicle, target) if vehicle.is_valid() else RID()
    universal_controller = _rail_component(RailVehicleComponentType.COMPONENT_UNIVERSAL_CONTROLLER) as RailVehicleUniversalController


## A section of a component the vehicle does not have says so instead of its controls - they would
## keep the previous vehicle's values and send commands nobody takes. Called by the section when it
## takes its component, before it picks which of its own groups to show.
func _show_applicable(applicable:bool) -> void:
    for child:Node in get_children():
        if child is CanvasItem:
            child.visible = applicable
    %NotApplicable.visible = not applicable


func _component(type:VehicleComponentType.Type) -> VehicleComponent:
    return VehicleServer.vehicle_component_get(target_vehicle, type)


func _rail_component(type:RailVehicleComponentType.Type) -> VehicleComponent:
    return RailVehicleServer.vehicle_component_get(target_vehicle, type)
