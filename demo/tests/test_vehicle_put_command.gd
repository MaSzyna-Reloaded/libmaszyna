extends MaszynaGutTest

## An order put to a vehicle nobody drives (MaszynaScenarioEventServer.vehicle_put_command()): the
## vehicle carries it out itself, again on every step until it is done (TMoverParameters::PutCommand(),
## RunInternalCommand(), Mover.cpp:12097-12106, 12724-12746)

const BATTERY_VOLTAGE:float = 110.0
const TRACK_NAME:String = "put_command_test"
const TRACK_LENGTH:float = 200.0
const TRACK_OFFSET:float = 50.0

var _track:RID


func before_each() -> void:
    _track = build_track(TRACK_NAME, TRACK_LENGTH)


func after_each() -> void:
    TrackServer.track_free(_track)
    TrackServer.topology_rebuild()


func test_a_vehicle_nobody_drives_carries_an_order_out_itself() -> void:
    var model:RailVehicleController = MaszynaMoverRailVehicleController.new()
    model.add_component(build_power_supply(BATTERY_VOLTAGE))
    var train:VehicleController = build_vehicle("PutCommandTest", model)
    var vehicle:RID = train.get_rid()
    RailVehicleServer.vehicle_set_track(vehicle, _track, TRACK_OFFSET, TrackServer.DIRECTION_NORMAL)
    assert_false(train.get_state()["battery_enabled"])

    MaszynaScenarioEventServer.vehicle_put_command(vehicle, "BatterySwitch", 1.0, 0.0, Vector3.ZERO)
    await step(1)

    assert_true(train.get_state()["battery_enabled"], "BatterySwitch (Mover.cpp:12471-12477)")
