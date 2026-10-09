extends MaszynaGutTest

## The cruise control switch (TTrain::OnCommand_tempomattoggle, Train.cpp:1486-1549) moves the
## controlled vehicle's second controller on and off; the door step switch
## (OnCommand_doorsteptoggle, Train.cpp:7692-7720) flips the step permit.

## An ELF's head car: a cruise control on its second controller (SCPN=1, CoupledCtrl=No)
const MOTOR_CAR_PATH:String = "res://tests/fixtures/dynamic/pkp/elf_v1/34we-a.fiz"

var vehicle_rid:RID
## The front cabin, the driver's
var cabin:RID
var behaviours:Array[RefCounted] = []


func before_each() -> void:
    var train:VehicleController = build_vehicle("TestTempomatDoorStep",
            FizVehicleBuilder.build_description_at(MOTOR_CAR_PATH), 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    vehicle_rid = train.get_rid()
    cabin = RailVehicleServer.vehicle_get_front_cabin(vehicle_rid)
    await wait_idle_frames(2)
    VehicleServer.vehicle_send_command(vehicle_rid, "battery", true)
    VehicleServer.vehicle_send_command(vehicle_rid, "cab_activation", true)
    await wait_idle_frames(2)


func after_each() -> void:
    for behaviour:RefCounted in behaviours:
        behaviour.unregister()
    behaviours.clear()


func _register(behaviour:RefCounted) -> void:
    behaviour.register(vehicle_rid, cabin)
    behaviours.append(behaviour)


func _state(key:String) -> Variant:
    return VehicleServer.vehicle_dump_state(vehicle_rid).get(key)


func test_the_door_step_switch_flips_the_step_permit() -> void:
    _register(LegacyCabinDoorStep.new(CabinButton.ButtonType.TOGGLE))
    var before:bool = bool(_state("doors_step_enabled"))
    CabinSystem.act(cabin, LegacyCabinDoorStep.SWITCH, &"toggle")
    await wait_idle_frames(2)
    assert_eq(bool(_state("doors_step_enabled")), not before)


func test_the_cruise_control_switch_moves_the_second_controller() -> void:
    _register(LegacyCabinTempomat.new(CabinButton.ButtonType.TOGGLE, false))
    CabinSystem.act(cabin, LegacyCabinTempomat.SWITCH, &"toggle")
    await wait_idle_frames(2)
    assert_eq(int(_state("controller_second_position")), LegacyCabinTempomat.ON_STEPS, "on: the first position")
    assert_true(CabinSystem.get_control(cabin, LegacyCabinTempomat.SWITCH), "the switch shows it on")
    CabinSystem.act(cabin, LegacyCabinTempomat.SWITCH, &"toggle")
    await wait_idle_frames(2)
    assert_eq(int(_state("controller_second_position")), 0, "off: back")
    assert_false(CabinSystem.get_control(cabin, LegacyCabinTempomat.SWITCH))
