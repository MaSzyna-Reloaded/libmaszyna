extends MaszynaGutTest

## Cab switching - original engine: TTrain::CabChange() (Train.cpp:10324-10351) stepping
## TMoverParameters::ChangeCab() (Mover.cpp:735-779) 1 -> 0 (machine room) -> -1.

const LOCOMOTIVE_PATH:String = "res://tests/fixtures/dynamic/pkp/ep09_v1/104e-039.fiz"

var train:VehicleController
var driver:RID


func before_each():
    train = build_vehicle("TestCabChangeTrain", null, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    # the active cab is the master controller's - a vehicle with a cab has one
    train.add_component(MoverRailVehicleMasterController.new())
    # the cab0definition: of the vehicle - the machine room lies between its two cabs
    RailVehicleServer.vehicle_add_machine_room(train.get_rid())
    driver = get_vehicle_driver(train.get_rid())


func _driver_cabin_kind(vehicle_rid:RID) -> RailVehicleCabinKind.Kind:
    return RailVehicleServer.cabin_get_kind(RailVehicleServer.vehicle_get_driver_cabin(vehicle_rid))


## No cab is active until the crew switches it on (CabActive = 0, MOVER.h:2090; Train.cpp:2430).
func test_unmanned_vehicle_starts_in_cab_one_with_inactive_cab():
    assert_eq(_driver_cabin_kind(train.get_rid()), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT)
    assert_eq(train.get_state()["cabin"], 0)

    train.send_command("cab_activation", true)
    train.update_state()
    assert_eq(train.get_state()["cabin"], 1)
    assert_true(train.get_state()["cabin_controleable"])

    train.send_command("cab_activation", false)
    train.update_state()
    assert_eq(train.get_state()["cabin"], 0)


func test_cab_change_backward_goes_through_machine_room():
    watch_signals(RailVehicleServer)
    RailVehicleServer.person_change_cabin(driver, RailVehicleServer.CABIN_CHANGE_BACKWARD)
    train.update_state()
    assert_eq(_driver_cabin_kind(train.get_rid()), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_MACHINE)
    assert_eq(train.get_state()["cabin"], 0)
    assert_signal_emitted_with_parameters(RailVehicleServer, "vehicle_driver_cabin_changed",
            [train.get_rid(), RailVehicleServer.vehicle_get_machine_room(train.get_rid())])

    RailVehicleServer.person_change_cabin(driver, RailVehicleServer.CABIN_CHANGE_BACKWARD)
    train.update_state()
    assert_eq(_driver_cabin_kind(train.get_rid()), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR)
    assert_eq(train.get_state()["cabin"], -1)
    assert_signal_emitted_with_parameters(RailVehicleServer, "vehicle_driver_cabin_changed",
            [train.get_rid(), RailVehicleServer.vehicle_get_rear_cabin(train.get_rid())])


func test_cab_change_stops_at_vehicle_end():
    RailVehicleServer.person_change_cabin(driver, RailVehicleServer.CABIN_CHANGE_FORWARD)
    train.update_state()
    assert_eq(_driver_cabin_kind(train.get_rid()), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT)

    for i in range(3):
        RailVehicleServer.person_change_cabin(driver, RailVehicleServer.CABIN_CHANGE_BACKWARD)
    train.update_state()
    assert_eq(_driver_cabin_kind(train.get_rid()), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR)

    RailVehicleServer.person_change_cabin(driver, RailVehicleServer.CABIN_CHANGE_FORWARD)
    RailVehicleServer.person_change_cabin(driver, RailVehicleServer.CABIN_CHANGE_FORWARD)
    train.update_state()
    assert_eq(_driver_cabin_kind(train.get_rid()), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT)
    assert_eq(train.get_state()["cabin"], 1)


func test_starts_in_cab_two_for_rear_driver():
    var rear_train:VehicleController = build_vehicle("TestCabChangeRearTrain", null, 0.0,
            MaszynaDynamicData.DriverType.DRIVER_REAR)
    rear_train.add_component(MoverRailVehicleMasterController.new())

    assert_eq(_driver_cabin_kind(rear_train.get_rid()), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR)
    assert_eq(rear_train.get_state()["cabin"], 0)

    rear_train.send_command("cab_activation", true)
    rear_train.update_state()
    assert_eq(rear_train.get_state()["cabin"], -1)


## The driver leaves the controls of the cab at rest as he goes: the brake handle at its neutral
## position, the controllers at zero (TMoverParameters::ChangeCab(), Mover.cpp:735-749)
func test_driver_cab_change_leaves_the_controls_at_rest():
    var locomotive:VehicleController = build_vehicle("TestCabChangeControls",
            FizVehicleBuilder.build_description_at(LOCOMOTIVE_PATH), 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    await wait_idle_frames(2)
    var vehicle_rid:RID = locomotive.get_rid()
    var master:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
            vehicle_rid, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
    var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
            vehicle_rid, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    var neutral:float = brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_CUTOFF)
    VehicleServer.vehicle_send_command(vehicle_rid, "cab_activation", true)
    VehicleServer.vehicle_send_command(vehicle_rid, "direction_increase")
    VehicleServer.vehicle_send_command(vehicle_rid, "main_controller_increase", 1)
    VehicleServer.vehicle_send_command(vehicle_rid, "second_controller_increase", 1)
    VehicleServer.vehicle_send_command(vehicle_rid, "brake_level_set", 1.0)
    assert_gt(master.get_main_position(), 0, "the main controller is notched up")
    assert_gt(master.get_second_position(), 0, "the second controller is notched up")
    assert_ne(brake.get_controller_position(), neutral, "the brake handle is out of its neutral")

    RailVehicleServer.person_change_cabin(get_vehicle_driver(vehicle_rid),
            RailVehicleServer.CABIN_CHANGE_BACKWARD)

    assert_eq(_driver_cabin_kind(vehicle_rid), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR)
    assert_eq(master.get_main_position(), 0, "the main controller at zero")
    assert_eq(master.get_second_position(), 0, "the second controller at zero")
    assert_eq(brake.get_controller_position(), neutral, "the brake handle at its neutral")
