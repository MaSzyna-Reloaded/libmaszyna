extends MaszynaGutTest

## A vehicle's configuration reaches its backend once, in the original's order
## (TDynamicObject::Init(), DynObj.cpp:2020-2075): every value of its FIZ, then its load, then
## CheckLocomotiveParameters() once. It ran three times, between configuration passes that undid
## part of it - a standing vehicle's spring brake released - and every component was applied again
## on the first step, over whatever had been done to the vehicle since its build.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
## E6ACT: a FIZ with a SpringBrake section (MBF=2.9)
const DATA_PATH:String = "dynamic/pkp/e6act_v1"
const FILE_NAME:String = "e6act-001"
const VEHICLE_ID:String = "E6ACT-001"

var _previous_game_dir:String
var _vehicle:MaszynaRailVehicle3D
## Configuration changes announced for the vehicle
var _configuration_changes:int = 0


func before_each() -> void:
    _configuration_changes = 0
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    if is_instance_valid(_vehicle):
        _vehicle.free()
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


## CheckLocomotiveParameters() engages the spring brake of a vehicle standing at load
## (Mover.cpp:8948-8950); nothing applied after it releases it again
func test_a_standing_vehicle_keeps_the_spring_brake_its_build_engaged() -> void:
    _vehicle = await spawn_maszyna_vehicle(DATA_PATH, FILE_NAME, "", VEHICLE_ID)
    var spring_brake:RailVehicleSpringBrake = RailVehicleServer.vehicle_component_get(
            _vehicle.get_rid(), RailVehicleComponentType.COMPONENT_SPRING_BRAKE) as RailVehicleSpringBrake
    assert_not_null(spring_brake, "the E6ACT has a spring brake")
    if not is_passing():
        return
    assert_true(spring_brake.get_active(), "a vehicle standing at load is on its spring brake")


## The build applied the configuration: the vehicle's first step applies none of it again
func test_the_first_step_applies_no_configuration_again() -> void:
    _vehicle = await spawn_maszyna_vehicle(DATA_PATH, FILE_NAME, "", VEHICLE_ID)
    VehicleServer.vehicle_config_changed.connect(_on_vehicle_config_changed)
    await step(1)
    VehicleServer.vehicle_config_changed.disconnect(_on_vehicle_config_changed)
    assert_eq(_configuration_changes, 0, "no configuration applied on the first step")


func _on_vehicle_config_changed(vehicle:RID) -> void:
    if vehicle == _vehicle.get_rid():
        _configuration_changes += 1
