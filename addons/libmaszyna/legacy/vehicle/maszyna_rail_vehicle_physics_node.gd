@tool
extends RailVehiclePhysicsNode
class_name MaszynaRailVehiclePhysicsNode

## A MaSzyna rail vehicle, built from a legacy `.fiz` file.
##
## All it does is name the file and ask FizVehicleBuilder for the vehicle's controller -
## the same shape E3DModelInstance has towards E3DModelManager. Everything else about owning a
## vehicle (the handle, the components, freeing them) belongs to VehiclePhysicsNode, making it a
## rail one to RailVehiclePhysicsNode, and the parsing and its on-disk cache to the builder.

## Base MaSzyna data path used to resolve the FIZ file, matching E3DModelInstance.data_path.
@export var data_path:String = "":
    set(x):
        if not x == data_path:
            data_path = x
            # before the node enters the tree its vehicle is built from these as it enters
            _dirty = is_inside_tree()
            set_process(_dirty)

## FIZ file name, without the ".fiz" extension, matching E3DModelInstance.model_filename.
@export var fiz_filename:String = "":
    set(x):
        if not x == fiz_filename:
            fiz_filename = x
            _dirty = is_inside_tree()
            set_process(_dirty)

## The file changed while the vehicle stands: it is built again from it once, in the next frame
var _dirty:bool = false


## The vehicle's controller, built from the .fiz as the node enters the tree - once, before anything
## can see the vehicle
func _build_controller() -> VehicleController:
    return FizVehicleBuilder.build_description(data_path, fiz_filename) if fiz_filename else null


func _process(_delta:float) -> void:
    set_process(false)
    if not _dirty:
        return
    _dirty = false
    controller = _build_controller()
