extends Node
class_name LegacyCabinTrainsetPressures

## The pressures of the cars of the train a cab's `brakes: i j` gauges show (TTrain::fPress,
## Train.cpp:8670-8680, 12158-12166): car i (1 the first) counted from the end the occupied cab
## faces over the control couplings, value j of it - the brake cylinders, the brake pipe, the feed
## pipe or the control pipe. Answered as the cab's own state (CabinSystem.state_computed_value_register()).

## The cars the original counts (fPress[20], Train.cpp:921)
const CAR_COUNT:int = 20
## fPress[i][0..3] - the state keys of a car's pressures (Train.cpp:8675-8678)
const PRESSURES:PackedStringArray = [
    "brake_air_pressure", "pipe_pressure", "feed_pipe_pressure", "brake_control_pipe_pressure",
]

## The keys this cab's gauges read
var state_keys:PackedStringArray = []
var _vehicle_rid:RID


## The key of car `car`'s value `pressure`, clamped as the original reads them (Train.cpp:12165)
static func state_key(car:int, pressure:int) -> String:
    return "brakes/%d/%d" % [clampi(car, 1, CAR_COUNT), clampi(pressure, 0, PRESSURES.size() - 1)]


func set_vehicle_rid(vehicle_rid:RID) -> void:
    _vehicle_rid = vehicle_rid
    for key:String in state_keys:
        var parts:PackedStringArray = key.split("/")
        CabinSystem.state_computed_value_register(vehicle_rid, key, _pressure.bind(int(parts[1]), int(parts[2])))


func _exit_tree() -> void:
    for key:String in state_keys:
        CabinSystem.state_computed_value_unregister(_vehicle_rid, key)


func _pressure(car:int, pressure:int) -> float:
    var cab_end:RailVehicleController.CouplerEnd = (RailVehicleController.COUPLER_END_REAR
            if RailVehicleServer.cabin_get_kind(RailVehicleServer.vehicle_get_driver_cabin(_vehicle_rid))
                    == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR
            else RailVehicleController.COUPLER_END_FRONT)
    var cars:Array = RailVehicleServer.vehicle_get_coupled(_vehicle_rid, cab_end, RailVehicleController.COUPLING_FLAG_CONTROL)
    if car > cars.size():
        return 0.0
    return float(VehicleServer.vehicle_dump_state(cars[car - 1]).get(PRESSURES[pressure], 0.0))
