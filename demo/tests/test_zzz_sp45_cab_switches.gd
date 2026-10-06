extends MaszynaGutTest

## The switches a player answers the driver's hints with, in the REAL SP45's cab logic (its own .fiz
## and .mmd): the traction motors' blowers and the cooling water's pump, its breaker and heater
## (OnCommand_motorblowerstogglefront, waterpumptoggle, waterpumpbreakertoggle, waterheatertoggle,
## Train.cpp:4069-4378, 4750-4946). The operator's report of 2026-10-06: the hints to switch them on
## hung for good, a player had no switch to do it with.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "startup_sp45_v1.scn"
const VEHICLE:String = "301db-152"

var _previous_game_dir:String = ""
var _scenery:MaszynaSceneryNode
var _vehicle:RID
var _cabin:RID
var _engine:RailVehicleDieselEngine


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)
    _scenery = MaszynaSceneryNode.new()
    _scenery.filename = SCENERY
    add_child(_scenery)
    # announced once its vehicles are built
    if not await wait_loaded(_scenery.scenery_loaded, SCENERY):
        return
    _vehicle = VehicleServer.vehicle_get_rid_by_name(VEHICLE)
    assert_true(VehicleServer.vehicle_is_simulation_ready(_vehicle), "%s is simulated once %s is loaded" % [VEHICLE, SCENERY])
    _cabin = RailVehicleServer.vehicle_get_driver_cabin(_vehicle)
    _engine = VehicleServer.vehicle_component_get(_vehicle, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleDieselEngine


func after_each() -> void:
    _scenery.free()
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


## A two-state switch flipped, as a key or a click does it
func _press(control:StringName) -> void:
    CabinSystem.act(_cabin, control, &"toggle")


func test_the_motor_blowers_are_switched_on_from_the_cab() -> void:
    assert_not_null(_engine, "the SP45 is a diesel")
    if not _engine:
        return
    assert_eq(_engine.motor_blowers_start_mode, RailVehicleController.START_MODE_MANUAL,
            "its blowers are switched by hand (MotorBlowersStart=Manual)")
    var front:RailVehicleController.CouplerEnd = RailVehicleController.COUPLER_END_FRONT
    var rear:RailVehicleController.CouplerEnd = RailVehicleController.COUPLER_END_REAR
    assert_false(_engine.get_motor_blowers_enabled(front), "cold, the front blowers are off")

    _press(&"motorblowersfront_sw")
    _press(&"motorblowersrear_sw")

    assert_true(_engine.get_motor_blowers_enabled(front), "the front blowers switched on")
    assert_true(_engine.get_motor_blowers_enabled(rear), "and the rear ones")

    _press(&"motorblowersalloff_sw")
    assert_true(_engine.get_motor_blowers_disabled(front), "the all-off switch holds the front ones off")
    assert_true(_engine.get_motor_blowers_disabled(rear), "and the rear ones")


func test_the_cooling_water_is_switched_from_the_cab() -> void:
    assert_not_null(_engine, "the SP45 is a diesel")
    if not _engine:
        return
    assert_false(_engine.get_water_pump_breaker(), "cold, the water pump's breaker is open")

    _press(&"waterpumpbreaker_sw")
    _press(&"waterpump_sw")
    _press(&"waterheaterbreaker_sw")
    _press(&"waterheater_sw")

    assert_true(_engine.get_water_pump_breaker(), "the water pump's breaker closed")
    assert_true(_engine.get_water_pump_enabled(), "the water pump switched on")
    assert_true(_engine.get_water_heater_breaker(), "the water heater's breaker closed")
    assert_true(_engine.get_water_heater_enabled(), "the water heater switched on")
