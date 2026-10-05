extends MaszynaGutTest

## The cab's door controls (LegacyCabinDoors - TTrain::OnCommand_doortoggleleft/right, dooropen*,
## doorclose*, doorcloseall, doorlocktoggle, doormodetoggle, departureannounce, Train.cpp:7087-7929),
## the door lamps, and the door permit switches' lamps (LegacyCabinDoorPermits, Train.cpp:8514-8520).

## An ELF's head car: doors opened from the cab or by the passengers (OpenCtrl=Mixed), closed from
## the cab (CloseCtrl=DriverCtrl), the departure signal held while closing (DoorClosureWarningAuto),
## a door lock (DoorBlocked), a permit needed (DoorNeedPermit)
const MOTOR_CAR_PATH:String = "res://tests/fixtures/dynamic/pkp/elf_v1/34we-a.fiz"
## A second of the clock's minute on which a blinking permit lamp is dark (wSecond % 2, Train.cpp:8515)
const ODD_SECOND:float = 1.0 / 3600.0
## And one on which it is lit
const EVEN_SECOND:float = 2.0 / 3600.0
const STEP_FRAMES:int = 3

var vehicle_rid:RID
var front_cabin:RID
var rear_cabin:RID
var behaviours:Array[RefCounted] = []
var _clock:float


func before_each() -> void:
    await _build("TestCabinDoors", true)


func after_each() -> void:
    for behaviour:RefCounted in behaviours:
        behaviour.unregister()
    behaviours.clear()
    SimulationServer.time_of_day = _clock


func _build(train_id:String, auto_warning:bool) -> void:
    _clock = SimulationServer.time_of_day
    var description:VehicleController = FizVehicleBuilder.build_description_at(MOTOR_CAR_PATH)
    var doors:RailVehicleDoors = description.get_component(VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    doors.close_auto_close_warning = auto_warning
    var train:VehicleController = build_vehicle(train_id, description, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    vehicle_rid = train.get_rid()
    front_cabin = RailVehicleServer.vehicle_get_front_cabin(vehicle_rid)
    rear_cabin = RailVehicleServer.vehicle_get_rear_cabin(vehicle_rid)
    await step(2)
    VehicleServer.vehicle_send_command(vehicle_rid, "battery", true)
    VehicleServer.vehicle_send_command(vehicle_rid, "cab_activation", true)
    VehicleServer.vehicle_send_command(vehicle_rid, "doors_left_permit", true)
    VehicleServer.vehicle_send_command(vehicle_rid, "doors_right_permit", true)
    await step(2)


func _doors_behaviour(cabin:RID, present_controls:Array[StringName],
        all_close_type:CabinButton.ButtonType = CabinButton.ButtonType.PUSH) -> void:
    var present:Dictionary[StringName, bool] = {}
    for control_id:StringName in LegacyCabinDoors.CONTROLS:
        present[control_id] = control_id in present_controls
    var behaviour:LegacyCabinDoors = LegacyCabinDoors.new(present, all_close_type)
    behaviour.register(vehicle_rid, cabin)
    behaviours.append(behaviour)


func _state(key:String) -> Variant:
    return VehicleServer.vehicle_dump_state(vehicle_rid).get(key)


func _click(cabin:RID, control_id:StringName) -> void:
    CabinSystem.act(cabin, control_id, &"hold")
    await step(STEP_FRAMES)
    CabinSystem.act(cabin, control_id, &"release")
    await step(STEP_FRAMES)


func test_the_toggle_opens_the_cab_side() -> void:
    var controls:Array[StringName] = [LegacyCabinDoors.LEFT_TOGGLE]
    _doors_behaviour(front_cabin, controls)
    await _click(front_cabin, LegacyCabinDoors.LEFT_TOGGLE)
    assert_true(bool(_state("doors_left_remote_open")), "the left doors are opened")
    assert_false(bool(_state("doors_right_remote_open")))
    assert_eq(CabinSystem.get_control(front_cabin, LegacyCabinDoors.LEFT_TOGGLE), 1.0, "the toggle shows them open")


func test_from_the_rear_cab_the_toggle_opens_the_other_side_of_the_vehicle() -> void:
    var controls:Array[StringName] = [LegacyCabinDoors.LEFT_TOGGLE]
    _doors_behaviour(rear_cabin, controls)
    await _click(rear_cabin, LegacyCabinDoors.LEFT_TOGGLE)
    assert_true(bool(_state("doors_right_remote_open")), "the cab's left is the vehicle's right")
    assert_false(bool(_state("doors_left_remote_open")))


func test_the_automatic_departure_signal_sounds_while_closing_is_held() -> void:
    var controls:Array[StringName] = [LegacyCabinDoors.LEFT_TOGGLE]
    _doors_behaviour(front_cabin, controls)
    await _click(front_cabin, LegacyCabinDoors.LEFT_TOGGLE)
    CabinSystem.act(front_cabin, LegacyCabinDoors.LEFT_TOGGLE, &"hold")
    await step(STEP_FRAMES)
    assert_true(bool(_state("doors_departure_signal")), "held: the signal is given")
    assert_true(bool(_state("doors_left_remote_open")), "held: the doors stay open")
    CabinSystem.act(front_cabin, LegacyCabinDoors.LEFT_TOGGLE, &"release")
    await step(STEP_FRAMES)
    assert_false(bool(_state("doors_departure_signal")), "released: the signal stops")
    assert_false(bool(_state("doors_left_remote_open")), "released: the doors close")


func test_without_the_automatic_signal_the_doors_close_on_the_press() -> void:
    await _build("TestCabinDoorsWithoutAutoWarning", false)
    var controls:Array[StringName] = [LegacyCabinDoors.LEFT_TOGGLE]
    _doors_behaviour(front_cabin, controls)
    await _click(front_cabin, LegacyCabinDoors.LEFT_TOGGLE)
    CabinSystem.act(front_cabin, LegacyCabinDoors.LEFT_TOGGLE, &"hold")
    await step(STEP_FRAMES)
    assert_false(bool(_state("doors_departure_signal")))
    assert_false(bool(_state("doors_left_remote_open")), "closed as it is pressed")


func test_a_two_button_cab_closes_only_with_the_close_all_button() -> void:
    var controls:Array[StringName] = [LegacyCabinDoors.LEFT_OPEN, LegacyCabinDoors.ALL_CLOSE]
    _doors_behaviour(front_cabin, controls)
    await _click(front_cabin, LegacyCabinDoors.LEFT_TOGGLE)
    assert_true(bool(_state("doors_left_remote_open")), "the toggle key opens through the open button")
    assert_eq(CabinSystem.get_control(front_cabin, LegacyCabinDoors.LEFT_OPEN), 0.0, "which springs back")
    await _click(front_cabin, LegacyCabinDoors.LEFT_TOGGLE)
    assert_true(bool(_state("doors_left_remote_open")), "no close button of its own: the toggle does not close")
    await _click(front_cabin, LegacyCabinDoors.ALL_CLOSE)
    assert_false(bool(_state("doors_left_remote_open")), "the close-all button does")


func test_a_delayed_close_all_button_closes_on_its_release() -> void:
    var controls:Array[StringName] = [LegacyCabinDoors.ALL_OPEN, LegacyCabinDoors.ALL_CLOSE]
    _doors_behaviour(front_cabin, controls, CabinButton.ButtonType.PUSH_DELAYED)
    await _click(front_cabin, LegacyCabinDoors.ALL_OPEN)
    assert_true(bool(_state("doors_left_remote_open")) and bool(_state("doors_right_remote_open")), "both sides open")
    CabinSystem.act(front_cabin, LegacyCabinDoors.ALL_CLOSE, &"hold")
    await step(STEP_FRAMES)
    assert_true(bool(_state("doors_left_remote_open")), "held: still open")
    assert_true(bool(_state("doors_departure_signal")), "held: the signal is given")
    CabinSystem.act(front_cabin, LegacyCabinDoors.ALL_CLOSE, &"release")
    await step(STEP_FRAMES)
    assert_false(bool(_state("doors_left_remote_open")) or bool(_state("doors_right_remote_open")), "released: closed")
    assert_false(bool(_state("doors_departure_signal")))


func test_a_control_the_cab_has_no_gauge_for_does_nothing() -> void:
    var controls:Array[StringName] = []
    _doors_behaviour(front_cabin, controls)
    var locked:bool = bool(_state("doors_lock_enabled"))
    await _click(front_cabin, LegacyCabinDoors.LEFT_TOGGLE)
    await _click(front_cabin, LegacyCabinDoors.DEPARTURE_SIGNAL)
    await _click(front_cabin, LegacyCabinDoors.LOCK)
    assert_false(bool(_state("doors_left_remote_open")))
    assert_false(bool(_state("doors_departure_signal")))
    assert_eq(bool(_state("doors_lock_enabled")), locked)


func test_the_departure_signal_button_gives_the_signal_while_held() -> void:
    var controls:Array[StringName] = [LegacyCabinDoors.DEPARTURE_SIGNAL]
    _doors_behaviour(front_cabin, controls)
    CabinSystem.act(front_cabin, LegacyCabinDoors.DEPARTURE_SIGNAL, &"hold")
    await step(STEP_FRAMES)
    assert_true(bool(_state("doors_departure_signal")))
    assert_eq(CabinSystem.get_control(front_cabin, LegacyCabinDoors.DEPARTURE_SIGNAL), 1.0)
    CabinSystem.act(front_cabin, LegacyCabinDoors.DEPARTURE_SIGNAL, &"release")
    await step(STEP_FRAMES)
    assert_false(bool(_state("doors_departure_signal")))


func test_the_lock_switch_flips_the_door_lock() -> void:
    var controls:Array[StringName] = [LegacyCabinDoors.LOCK]
    _doors_behaviour(front_cabin, controls)
    var locked:bool = bool(_state("doors_lock_enabled"))
    await _click(front_cabin, LegacyCabinDoors.LOCK)
    assert_eq(bool(_state("doors_lock_enabled")), not locked, "flipped")
    assert_eq(CabinSystem.get_control(front_cabin, LegacyCabinDoors.LOCK), 0.0 if locked else 1.0, "the switch shows it")
    await _click(front_cabin, LegacyCabinDoors.LOCK)
    assert_eq(bool(_state("doors_lock_enabled")), locked, "and back")


func test_the_mode_switch_flips_the_remote_only_control_without_a_gauge() -> void:
    var controls:Array[StringName] = []
    _doors_behaviour(front_cabin, controls)
    var before:bool = bool(_state("doors_remote_only"))
    await _click(front_cabin, LegacyCabinDoors.REMOTE_MODE)
    assert_eq(bool(_state("doors_remote_only")), not before)


func test_the_door_lamps_follow_the_open_doors_on_the_cab_side() -> void:
    var controls:Array[StringName] = [LegacyCabinDoors.LEFT_TOGGLE]
    _doors_behaviour(rear_cabin, controls)
    assert_false(bool(CabinSystem.vehicle_state_value(vehicle_rid, LegacyCabinDoors.DOORS_OPEN_LAMP)), "all closed")
    await _click(rear_cabin, LegacyCabinDoors.LEFT_TOGGLE)
    await step(STEP_FRAMES)
    assert_true(bool(CabinSystem.vehicle_state_value(vehicle_rid, LegacyCabinDoors.DOORS_OPEN_LAMP)), "a door is open")
    assert_true(bool(CabinSystem.vehicle_state_value(
            vehicle_rid, LegacyCabinDoors.SIDE_OPEN_LAMPS[RailVehicleDoors.SIDE_LEFT])), "on the rear cab's left")
    assert_false(bool(CabinSystem.vehicle_state_value(
            vehicle_rid, LegacyCabinDoors.SIDE_OPEN_LAMPS[RailVehicleDoors.SIDE_RIGHT])))
    assert_true(bool(CabinSystem.vehicle_state_value(vehicle_rid, LegacyCabinDoors.PERMITS_LAMP)), "permits given")


func _permit_lamp(side:RailVehicleDoors.Side) -> bool:
    return bool(CabinSystem.vehicle_state_value(vehicle_rid, LegacyCabinDoorPermits.LAMP_KEYS[side]))


func _permits_behaviour(cabin:RID) -> void:
    var behaviour:LegacyCabinDoorPermits = LegacyCabinDoorPermits.new(
            CabinButton.ButtonType.TOGGLE, CabinButton.ButtonType.TOGGLE)
    behaviour.register(vehicle_rid, cabin)
    behaviours.append(behaviour)


func test_a_permit_lamp_is_lit_through_by_a_steady_light() -> void:
    VehicleServer.vehicle_send_command(vehicle_rid, "doors_right_permit", false)
    await step(STEP_FRAMES)
    _permits_behaviour(front_cabin)
    var doors:RailVehicleDoors = VehicleServer.vehicle_component_get(vehicle_rid, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    doors.permit_light_blinking = RailVehicleDoors.PERMIT_LIGHT_CONTINUOUS
    SimulationServer.time_of_day = ODD_SECOND
    assert_true(_permit_lamp(RailVehicleDoors.SIDE_LEFT), "permitted: lit on an odd second too")
    assert_false(_permit_lamp(RailVehicleDoors.SIDE_RIGHT), "not permitted: dark")


func test_a_blinking_permit_lamp_is_dark_on_the_odd_seconds_while_the_doors_are_closed() -> void:
    _permits_behaviour(front_cabin)
    var doors:RailVehicleDoors = VehicleServer.vehicle_component_get(vehicle_rid, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    doors.permit_light_blinking = RailVehicleDoors.PERMIT_LIGHT_FLASHING_ALWAYS
    SimulationServer.time_of_day = EVEN_SECOND
    assert_true(_permit_lamp(RailVehicleDoors.SIDE_LEFT), "an even second: lit")
    SimulationServer.time_of_day = ODD_SECOND
    assert_false(_permit_lamp(RailVehicleDoors.SIDE_LEFT), "an odd second: dark")


func test_the_rear_cab_permit_lamps_show_the_other_side_of_the_vehicle() -> void:
    VehicleServer.vehicle_send_command(vehicle_rid, "doors_right_permit", false)
    await step(STEP_FRAMES)
    _permits_behaviour(rear_cabin)
    var doors:RailVehicleDoors = VehicleServer.vehicle_component_get(vehicle_rid, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    doors.permit_light_blinking = RailVehicleDoors.PERMIT_LIGHT_CONTINUOUS
    assert_true(_permit_lamp(RailVehicleDoors.SIDE_RIGHT), "the vehicle's left is the rear cab's right")
    assert_false(_permit_lamp(RailVehicleDoors.SIDE_LEFT))
