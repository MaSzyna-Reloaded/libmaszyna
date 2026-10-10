extends RefCounted
class_name LegacyCabinDirectionKey

## The reverser key of a vehicle whose reverser steps past "forward" to the high start
## (RailVehicleElectricSeriesEngine.direction_switches_circuit_imin_high): the key shows the
## direction and, one position further, the high start - DirActive + (Imin == IminHi), as
## TTrain::Update feeds ggDirKey (Train.cpp:9451-9458).

const STATE_KEY:String = "direction"

var _vehicle_rid:RID
var _engine:RailVehicleElectricSeriesEngine


func register(vehicle_rid:RID, _cabin:RID) -> void:
    _vehicle_rid = vehicle_rid
    _engine = CabinSystem.vehicle_component(vehicle_rid, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleElectricSeriesEngine
    CabinSystem.state_computed_value_register(vehicle_rid, STATE_KEY, _direction)


func unregister() -> void:
    CabinSystem.state_computed_value_unregister(_vehicle_rid, STATE_KEY)


func _direction() -> int:
    # the controller is not the cab's to hold: the direction comes by its name, as the dirkey asks for it
    return int(CabinSystem.vehicle_state(_vehicle_rid).get(STATE_KEY, 0)) + int(_engine.get_circuit_imin_high_enabled())
