extends MaszynaGutTest

## The cab mirrors (DynObj.cpp:4207-4236), the vehicle's doors component's own: a side unfolds, a
## full travel a second, while a cab is switched on and its doors are permitted to open; they fold
## when forbidden

## An ELF's head car: mirrors, and doors that need a permit (DoorNeedPermit)
const MOTOR_CAR_PATH:String = "res://tests/fixtures/dynamic/pkp/elf_v1/34we-a.fiz"
## A full travel takes a second
const TRAVEL_SECONDS:float = 1.0

var vehicle_rid:RID


func before_each() -> void:
    var description:VehicleController = FizVehicleBuilder.build_description_at(MOTOR_CAR_PATH)
    var train:VehicleController = build_vehicle("TestMirrors", description, 0.0,
            MaszynaVehicleData.DriverType.DRIVER_HEAD)
    vehicle_rid = train.get_rid()
    await step(2)
    VehicleServer.vehicle_send_command(vehicle_rid, "battery", true)
    VehicleServer.vehicle_send_command(vehicle_rid, "cab_activation", true)
    await step(2)


func _state() -> Dictionary:
    return VehicleServer.vehicle_get_controller(vehicle_rid).get_state()


func test_the_permitted_side_unfolds_and_the_other_stays_folded() -> void:
    VehicleServer.vehicle_send_command(vehicle_rid, "doors_left_permit", true)
    await step(ticks(TRAVEL_SECONDS) + 1)
    assert_eq(_state()["mirror_left_position"], 1.0, "the permitted side out")
    assert_eq(_state()["mirror_right_position"], 0.0, "the other one folded")


func test_forbidden_mirrors_fold() -> void:
    VehicleServer.vehicle_send_command(vehicle_rid, "doors_left_permit", true)
    await step(ticks(TRAVEL_SECONDS) + 1)
    VehicleServer.vehicle_send_command(vehicle_rid, "mirrors_forbid", true)
    await step(ticks(TRAVEL_SECONDS) + 1)
    assert_eq(_state()["mirror_left_position"], 0.0)
