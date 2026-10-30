extends MaszynaStartupTest

## SA132-006a (pkp/21xm_v1, fixtures/scenery/startup_sa132-006a.scn) driven the way a player drives
## it: started and then handled by nothing but the driving aid's hints, each by the key the hints
## window shows beside it. Its master controller carries the train brake (UCList IntegratedBrakePN):
## a hint that moves one moves the other, and hints that want them in two places never end.

## Hints the player follows at most
const MAX_STEPS:int = 40
## Simulated seconds a hint's key is held, as a player holds it
const KEY_HOLD:float = 2.0
## Simulated seconds the driver is given to update its list after a step (its reaction time)
const DRIVER_UPDATE_SECONDS:float = 5.0
## Simulated seconds a cab button without a key is held at most, as a player holds it until its
## hint is done (the main pipe unlocked: LPOff=4.5 bar, 218ma_a_p4.fiz)
const BUTTON_HOLD_TIMEOUT:float = 120.0
## The light selector's position (LightsPos, from 1) of Pc1 in SA132's LightsList
const PC1_PRESET:int = 2
## Driver updates a player waits at most while only hints without a control are listed (the main
## reservoir filling)
const MAX_WAITS:int = 12
## Cab changes a player makes at most to reach the other car's far cab
const MAX_CAB_CHANGES:int = 8
## Notches of the master controller a player gives at most, and the simulated seconds the unit has
## to move off at full power [s], faster than this [km/h]
const MAX_NOTCHES:int = 10
const MOVE_OFF_SECONDS:float = 60.0
const MOVED_OFF:float = 2.0
## The scenery's order that makes the driver want to move off (ShuntVelocity, km/h)
const SHUNT_VELOCITY:float = 40.0
## What a failure shows of the vehicle
const STATE_SHOWN:Array[String] = [
    "battery_enabled", "power24_available", "cabin", "direction", "controller_main_position",
    "controller_second_position", "main_switch_enabled", "engine_rpm_count", "tractive_force", "speed",
    "brake_controller_position", "brake_local_position_normalized", "brake_air_pressure",
    "pipe_pressure", "brake_control_pressure", "main_pipe_locked", "cabin_controleable", "compressor_pressure", "doors_left_open_permit",
    "doors_right_open_permit", "doors_permit_preset", "eimic_real",
]

var _driver:RID
var _cab_logic:CabinLogic
var _diesel:RailVehicleDieselEngine
## What the player did, in order, with the list shown each time - the failure's story
var _followed:Array[String] = []


func test_the_hints_start_it_and_then_end() -> void:
    if not await _enter():
        return
    assert_lt(await _follow_hints(), MAX_STEPS, "the hints end - nothing left to do: %s" % _story())
    assert_true(_running(), "the engine runs after the hints: %s" % _story())
    # the lamp code is lit by the light selector's key
    assert_false(_listed(MaszynaDriverHints.Hint.HEADCODE_TB1), "Tb1 lit by its hint: %s" % _story())


## The light selector of the B car's cab lights its first column at the end driven from - Pc1
## (LightsList "21 34", 218ma_b_p4.fiz) three white lamps at the B car's outer end
func test_the_b_cars_cab_lights_pc1_ahead() -> void:
    if not await _enter():
        return
    await _follow_hints()
    var first:RID = VehicleServer.person_get_vehicle(_driver)
    await _walk(&"cabin_next", func() -> bool: return not VehicleServer.person_get_vehicle(_driver) == first \
            and _cab_kind() == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR)
    var other:RID = VehicleServer.person_get_vehicle(_driver)
    # the cab given the control: the A car's switched off, the B car's on
    await _walk(&"cabin_previous", func() -> bool: return VehicleServer.person_get_vehicle(_driver) == first \
            and _cab_kind() == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT)
    await _hold(&"cab_activation_toggle")
    await _walk(&"cabin_next", func() -> bool: return VehicleServer.person_get_vehicle(_driver) == other \
            and _cab_kind() == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR)
    await _hold(&"cab_activation_toggle")
    var lighting:RailVehicleLighting = VehicleServer.vehicle_component_get(
            other, VehicleComponentType.COMPONENT_LIGHTING) as RailVehicleLighting
    while lighting.get_position() > PC1_PRESET:
        await _hold(&"lights_preset_previous")
    while lighting.get_position() < PC1_PRESET:
        await _hold(&"lights_preset_next")
    for lamp:String in ["rear_headlight_left", "rear_headlight_upper", "rear_headlight_right"]:
        assert_true(lighting.light_is_enabled(lamp), "%s lit ahead of the B car" % lamp)
    assert_false(lighting.light_is_enabled("rear_redmarker_left") or lighting.light_is_enabled("rear_redmarker_right"),
            "no red ahead of the B car")


## The cab signal (SHP) set off on a vehicle whose one button acknowledges it and the vigilance
## (SA132's bt-ca_shp): the hints ask for that button, by its key, and it stops the flashing - the
## harness's own acknowledging (MaszynaStartupTest) held off meanwhile
func test_the_cab_signal_is_hinted_and_acknowledged() -> void:
    if not await _enter():
        return
    await _follow_hints()
    var vehicle:RID = VehicleServer.person_get_vehicle(_driver)
    var security:RailVehicleSecuritySystem = _security(vehicle)
    SimulationServer.simulation_advanced.disconnect(_acknowledge_security)
    VehicleServer.vehicle_send_command(vehicle, &"security_cabsignal_trigger")
    await step(ticks(DRIVER_UPDATE_SECONDS))
    assert_true(security.get_cabsignal_blinking(), "the cab signal flashes")
    var entry:Dictionary = _hinted([MaszynaDriverHints.Hint.SECURITY_SYSTEM_RESET,
            MaszynaDriverHints.Hint.SHP_SYSTEM_RESET])
    assert_ne(entry.size(), 0, "its acknowledgement is hinted: %s" % [_listed_texts()])
    if entry:
        var action:StringName = _cab_logic.get_action(entry["control"], entry["gesture"])
        assert_ne(action, &"", "by a key")
        if action:
            await _hold(action)
        assert_false(security.get_cabsignal_blinking(), "the hinted key acknowledges it: %s" % _story())
    SimulationServer.simulation_advanced.connect(_acknowledge_security)


## The player started it by the hints, then walks through the unit to the other car's far cab and
## follows the hints there - sent back to deactivate the cab left active, back and forth again: they
## end, the cab the player sits in drives
func test_the_hints_end_in_the_other_cars_cab() -> void:
    if not await _enter():
        return
    await _follow_hints()
    var first:RID = VehicleServer.person_get_vehicle(_driver)
    await _walk(&"cabin_next", func() -> bool: return not VehicleServer.person_get_vehicle(_driver) == first \
            and _cab_kind() == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR)
    var other:RID = VehicleServer.person_get_vehicle(_driver)
    assert_ne(other, first, "the player reached the other car")
    if not is_passing():
        return
    await step(ticks(DRIVER_UPDATE_SECONDS))
    if _listed(MaszynaDriverHints.Hint.CAB_DEACTIVATION_OTHER_END):
        var release:bool = _listed(MaszynaDriverHints.Hint.INDEPENDENT_BRAKE_RELEASE_OTHER_END)
        await _walk(&"cabin_previous", func() -> bool: return VehicleServer.person_get_vehicle(_driver) == first \
                and _cab_kind() == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT)
        await step(ticks(DRIVER_UPDATE_SECONDS))
        while release and _brake(first).get_local_position_normalized() > 0.0:
            var before:float = _brake(first).get_local_position_normalized()
            await _hold(&"local_brake_decrease")
            if _brake(first).get_local_position_normalized() == before:
                break
        await _hold(&"cab_activation_toggle")
        await _walk(&"cabin_next", func() -> bool: return VehicleServer.person_get_vehicle(_driver) == other \
                and _cab_kind() == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR)
        await step(ticks(DRIVER_UPDATE_SECONDS))
    assert_lt(await _follow_hints(), MAX_STEPS, "the hints end in the other car: %s" % _story())
    assert_true(_master(other).get_cabin_controleable(), "the player's cab drives: %s" % _story())
    if not is_passing():
        return
    # moving off as a player does: the independent brake let off, the controller up to its end
    var brake:RailVehicleBrake = _brake(other)
    while brake.get_local_position_normalized() > 0.0:
        var before:float = brake.get_local_position_normalized()
        await _hold(&"local_brake_decrease")
        if brake.get_local_position_normalized() == before:
            break
    for _notch:int in MAX_NOTCHES:
        var before:int = _master(other).get_main_position()
        await _hold(&"main_controller_increase")
        if _master(other).get_main_position() == before:
            break
    # the hints that keep the power off followed, not the standing driver's others - the door
    # permits a deactivated cab gave
    for _step:int in MAX_STEPS:
        var entry:Dictionary = _hinted([MaszynaDriverHints.Hint.DOOR_LEFT_PERMIT_OFF,
                MaszynaDriverHints.Hint.DOOR_RIGHT_PERMIT_OFF])
        if not entry:
            break
        await _hold(_cab_logic.get_action(entry["control"], entry["gesture"]))
    assert_false(_listed(MaszynaDriverHints.Hint.INDEPENDENT_BRAKE_RELEASE_OTHER_END),
            "nothing is left braked at the other end: %s" % _story())
    await wait_simulated_until(func() -> bool: return VehicleServer.vehicle_get_speed(other) > MOVED_OFF,
            MOVE_OFF_SECONDS, "moved off")
    var left:Dictionary = VehicleServer.vehicle_dump_state(first)
    assert_gt(VehicleServer.vehicle_get_speed(other), MOVED_OFF, "it moves off from the other car: %s\nthe car left: %s" % [
            _story(), " ".join(["brake_local_position_normalized", "brake_air_pressure", "spring_brake/active",
                "brake_manual_position", "tractive_force", "cabin", "direction"].map(func(key:String) -> String:
                    return "%s=%s" % [key, left.get(key)]))])


## "Release train brakes" with the handle at running and the independent brake on: what is left is
## the independent brake (driverhints.cpp:863-871), so its key is shown, and following it ends the
## hint - not Kp6, which no longer moves anything (the player stood on it)
func test_releasing_the_train_brakes_shows_the_independent_brakes_key() -> void:
    if not await _enter():
        return
    await _follow_hints()
    var vehicle:RID = VehicleServer.person_get_vehicle(_driver)
    var brake:RailVehicleBrake = _brake(vehicle)
    while brake.get_local_position_normalized() < 1.0:
        var before:float = brake.get_local_position_normalized()
        await _hold(&"local_brake_increase")
        if brake.get_local_position_normalized() == before:
            break
    DriverServer.driver_send_command(_driver, "ShuntVelocity", SHUNT_VELOCITY, 0.0)
    await wait_simulated_until(func() -> bool:
            return _hinted([MaszynaDriverHints.Hint.BRAKING_FORCE_SET_ZERO]).size() > 0,
            DRIVER_UPDATE_SECONDS, "the train brakes' release hinted")
    var entry:Dictionary = _hinted([MaszynaDriverHints.Hint.BRAKING_FORCE_SET_ZERO])
    assert_ne(entry.size(), 0, "the train brakes' release is hinted: %s" % _story())
    if not entry:
        return
    var action:StringName = _cab_logic.get_action(entry["control"], entry["gesture"])
    assert_eq(action, &"local_brake_decrease", "by the independent brake's key: %s" % _story())
    for _step:int in MAX_STEPS:
        entry = _hinted([MaszynaDriverHints.Hint.BRAKING_FORCE_SET_ZERO])
        if not entry:
            break
        await _hold(_cab_logic.get_action(entry["control"], entry["gesture"]))
    assert_false(_listed(MaszynaDriverHints.Hint.BRAKING_FORCE_SET_ZERO),
            "the hinted keys release the train brakes: %s" % _story())


## The player walks through the cabs by `action` until `there`; the cab logic is the one of the
## vehicle reached
func _walk(action:StringName, there:Callable) -> void:
    for _change:int in MAX_CAB_CHANGES:
        if there.call():
            break
        await key_tap(action)
        await step(ticks(KEY_HOLD))
    var vehicle:RID = VehicleServer.person_get_vehicle(_driver)
    _cab_logic = CabinSystem.vehicle_get_cab_logic(vehicle)
    _followed.append("%s -> %s cab %d" % [action, vehicle, _cab_kind()])


func _cab_kind() -> int:
    return RailVehicleServer.cabin_get_kind(VehicleServer.person_get_cabin(_driver))


func _listed(hint:MaszynaDriverHints.Hint) -> bool:
    for entry:Dictionary in get_driver_internal_state(_driver).get("hints", []):
        if entry["hint"] == hint and not entry["done"]:
            return true
    return false


func _enter() -> bool:
    if not await enter_vehicle("startup_sa132-006a.scn", "sa132-006a"):
        return false
    _driver = DriverServer.vehicle_get_driver(occupied)
    assert_true(_driver.is_valid(), "the scenery's driver is seated in SA132")
    if not _driver.is_valid():
        return false
    _diesel = _engine(powered) as RailVehicleDieselEngine
    _cab_logic = CabinSystem.vehicle_get_cab_logic(occupied)
    await step(ticks(DRIVER_UPDATE_SECONDS))
    return true


## The hints followed, top to bottom, until none is left to do; the steps it took
func _follow_hints() -> int:
    var steps:int = 0
    var waits:int = 0
    for _step:int in MAX_STEPS:
        var vehicle:RID = VehicleServer.person_get_vehicle(_driver)
        var entry:Dictionary = _hinted()
        if entry:
            var action:StringName = _cab_logic.get_action(entry["control"], entry["gesture"])
            if action:
                await _hold(action)
            else:
                await _hold_button(entry)
        elif not _power_supply(vehicle).get_power24_available():
            await _hold(&"battery_toggle")
        elif _master(vehicle).get_cabin() == 0:
            await _hold(&"cab_activation_toggle")
        elif _waiting() and waits < MAX_WAITS:
            waits += 1
            await step(ticks(DRIVER_UPDATE_SECONDS))
            continue
        else:
            break
        steps += 1
    gut.p(_story(_followed.size()))
    return steps


func _running() -> bool:
    return _diesel.get_main_switch_enabled() and _diesel.get_rpm() > 0.0


## The first listed hint not done yet that shows a control - of `only`, when given - or none
func _hinted(only:Array = []) -> Dictionary:
    for entry:Dictionary in get_driver_internal_state(_driver).get("hints", []):
        if not entry["done"] and entry["control"] and (not only or entry["hint"] in only):
            return entry
    return {}


## A cab button without a key held by the hand until its hint is done
func _hold_button(entry:Dictionary) -> void:
    var listed:Array[String] = _listed_texts()
    _cab_logic.press(entry["control"])
    await wait_simulated_until(func() -> bool:
            for shown:Dictionary in get_driver_internal_state(_driver).get("hints", []):
                if shown["hint"] == entry["hint"]:
                    return shown["done"]
            return true, BUTTON_HOLD_TIMEOUT, "%s held" % entry["control"])
    _cab_logic.release(entry["control"])
    await step(ticks(DRIVER_UPDATE_SECONDS))
    _followed.append("%s of %s -> %s" % [entry["control"], listed, _vehicle_text()])


## Whether a hint the player can only wait for is listed (the hint's text says so)
func _waiting() -> bool:
    for entry:Dictionary in get_driver_internal_state(_driver).get("hints", []):
        if not entry["done"] and entry["hint"] in [MaszynaDriverHints.Hint.WAIT_PRESSURE_TOO_LOW]:
            return true
    return false


func _listed_texts() -> Array[String]:
    var listed:Array[String] = []
    for entry:Dictionary in get_driver_internal_state(_driver).get("hints", []):
        listed.append(entry["text"])
    return listed


## The key held as a player holds it, then the driver's reaction time; the step goes into the
## story with what the window listed and what the vehicle showed
func _hold(action:StringName) -> void:
    var listed:Array[String] = _listed_texts()
    await key_press(action)
    await step(ticks(KEY_HOLD))
    await key_release(action)
    await step(ticks(DRIVER_UPDATE_SECONDS))
    _followed.append("%s of %s -> %s" % [action, listed, _vehicle_text()])


func _vehicle_text() -> String:
    var state:Dictionary = VehicleServer.vehicle_dump_state(VehicleServer.person_get_vehicle(_driver))
    var shown:Array[String] = []
    for key:String in STATE_SHOWN:
        shown.append("%s=%s" % [key, state.get(key)])
    return " ".join(shown)


func _story(last:int = 8) -> String:
    return "\n".join(_followed.slice(-last)) + "\nnow: " + _vehicle_text()
