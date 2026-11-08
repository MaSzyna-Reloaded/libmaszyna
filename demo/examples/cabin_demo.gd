extends Node3D
## The cab placed in this scene by hand sits in the vehicle placed beside it - in its front cab, as
## the original's cab 1. In the game CabinSystem.cabin_show() builds and seats a cab instead.

@onready var _vehicle: VehiclePhysicsNode = $SM42v1
@onready var _cabin: Cabin3D = $SM42Cabin


func _ready() -> void:
    _cabin.set_cabin(RailVehicleServer.vehicle_get_cabin(_vehicle.get_vehicle_rid(), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT))
