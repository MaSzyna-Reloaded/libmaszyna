extends MaszynaStartupTest

## SN61-02 (pkp/sn61_v2, fixtures/scenery/startup_sn61-02.scn, the rear cab of calkowo_sn61_zima.scn)
## driven the way a player drives it: started by nothing but the driving aid's hints, top to bottom,
## each by the key the hints window shows beside it (DriverHints); moved off by the master
## controller as a player moves off; then stopped by the independent brake alone. The SN61 starts
## only off position 0 (R = 0, dizel_StartupCheck(), Mover.cpp:7063-7069), so a list that leads to
## the line breaker with the controller at 0 never gets there.

## Hints the player follows at most before the engine has to run
const MAX_STEPS:int = 20
## Notches of the master controller the player gives at most to reach DRIVING_SPEED
const MAX_DRIVING_STEPS:int = 40
## Simulated seconds a hint's key is held, as a player holds it: a knob moves at the hand's speed
## while its key is down (CabinLogic, KNOB_KEY_SPEED), and the line breaker closes only after the
## SN61's InitialCtrlDelay (1.5 s) of holding (Train.cpp:8438-8472) - held over that
const KEY_HOLD:float = 2.0
## Simulated seconds the driver is given to update its list after a step (its reaction time)
const DRIVER_UPDATE_SECONDS:float = 5.0
## The speed the player drives up to before braking [km/h]
const DRIVING_SPEED:float = 20.0
## Simulated seconds the independent brake has to stop the vehicle from DRIVING_SPEED
const BRAKING_TIMEOUT:float = 60.0
## A braking cylinder: this share of the vehicle's MaxBP at least
const BRAKING_CYLINDER_SHARE:float = 0.5
## Stopped [km/h]
const STOPPED_SPEED:float = 0.5
## What a failure shows of the vehicle - what the cab's own gauges and the driving aid show
const STATE_SHOWN:Array[String] = [
    "battery_enabled", "power24_available", "cabin", "direction", "direction_absolute",
    "controller_main_position", "main_switch_enabled", "diesel_startup", "engine_rpm_count",
    "diesel_fill", "diesel_clutch_desired", "diesel_clutch_engagement", "tractive_force", "speed",
    "oil_pump_active", "fuel_pump_active", "oil_pump_pressure", "brake_local_position_normalized",
    "brake_loco_pressure", "brake_air_pressure", "brake_force", "pipe_pressure", "feed_pipe_pressure",
    "compressor_pressure",
]

var _driver:RID
var _cab_logic:CabinLogic
var _diesel:MaszynaMoverRailVehicleDieselEngine
## What the player did, in order, with the list shown each time - the failure's story
var _followed:Array[String] = []


func test_the_hints_start_it_and_the_independent_brake_stops_it() -> void:
    if not await _start_by_hints("startup_sn61-02.scn"):
        return
    # the next hint after the start, as the player follows it - for an SN61 standing it was
    # "neutral", which took the controller to 0 and the engine down
    var next:StringName = _hinted_action()
    if next:
        await _hold(next)
    assert_true(_running(), "the engine keeps running after the hint that follows the start: %s" % _story())
    if not is_passing():
        return

    # moving off as a player does - the standing driver's hints ask for neutral, the player drives
    # anyway: the independent brake the hints applied released, then the master controller a notch
    # up at a time; the engine must keep running all the way
    var brake:RailVehicleBrake = _brake(occupied)
    while brake.get_local_position_normalized() > 0.0:
        var before:float = brake.get_local_position_normalized()
        await _hold(&"local_brake_decrease")
        if brake.get_local_position_normalized() == before:
            break
    # the hand brake a standing vehicle is built with (CheckLocomotiveParameters(), Mover.cpp:8946)
    # is the player's to release: AutoRewident() releases it only on vehicles nobody drives by hand
    # (Driver.cpp:2193-2245) and no hint asks for it - the original cues no "manualbrakoff"
    while brake.get_manual_position() > 0:
        var before:int = brake.get_manual_position()
        await key_tap(&"manual_brake_decrease")
        if brake.get_manual_position() == before:
            break
    for _step:int in MAX_DRIVING_STEPS:
        if VehicleServer.vehicle_get_speed(occupied) >= DRIVING_SPEED or not _running():
            break
        await _hold(&"main_controller_increase")
    assert_true(_running(), "the engine keeps running while moving off: %s" % _story())
    assert_true(VehicleServer.vehicle_get_speed(occupied) >= DRIVING_SPEED,
            "SN61-02 reaches %.0f km/h: %s" % [DRIVING_SPEED, _story()])
    if not is_passing():
        return

    # the independent brake alone: the controller back to idle - never to 0, where the engine
    # stops - and the brake handle held to its end
    while _master(occupied).get_main_position() > 0:
        var before:int = _master(occupied).get_main_position()
        await _hold(&"main_controller_decrease")
        if _diesel.get_clutch_desired() == MaszynaMoverThrottlePositionItem.CLUTCH_BEHAVIOR_NONE:
            await _hold(&"main_controller_increase")
            break
        if _master(occupied).get_main_position() == before:
            break
    while brake.get_local_position_normalized() < 1.0:
        var before:float = brake.get_local_position_normalized()
        await _hold(&"local_brake_increase")
        if brake.get_local_position_normalized() == before:
            break
    # the highest cylinder pressure on the way - an array, as a lambda takes a local by value
    var cylinder_peak:Array[float] = [brake.get_air_pressure()]
    var stopped:Callable = func() -> bool:
        cylinder_peak[0] = maxf(cylinder_peak[0], brake.get_air_pressure())
        return VehicleServer.vehicle_get_speed(occupied) < STOPPED_SPEED
    if not await wait_simulated_until(stopped, BRAKING_TIMEOUT, "SN61-02 stopped by the independent brake"):
        return
    var cylinder_max:float = cylinder_peak[0]
    assert_gt(cylinder_max, BRAKING_CYLINDER_SHARE * brake.max_cylinder_pressure,
            "the independent brake fills the cylinders (max %.2f bar): %s" % [cylinder_max, _story()])
    assert_lt(VehicleServer.vehicle_get_speed(occupied), STOPPED_SPEED,
            "the independent brake stops SN61-02 within %.0f s: %s" % [BRAKING_TIMEOUT, _story()])


## A driver who wants to move off asks for the releaser while the independent brake holds the
## cylinders full (Driver.cpp:8221-8224). Let off, the cylinders empty, the reason is gone: the
## original keeps "Actuate" listed until the releaser is on, the hints take it back. Without that
## the hint, cued while the brake was on, was still listed here.
func test_the_releaser_is_not_asked_for_once_the_independent_brake_is_let_off() -> void:
    if not await _start_by_hints("startup_sn61-02_shunting.scn"):
        return
    var brake:RailVehicleBrake = _brake(occupied)
    while brake.get_local_position_normalized() < 1.0:
        var before:float = brake.get_local_position_normalized()
        await _hold(&"local_brake_increase")
        if brake.get_local_position_normalized() == before:
            break
    while brake.get_local_position_normalized() > 0.0:
        var before:float = brake.get_local_position_normalized()
        await _hold(&"local_brake_decrease")
        if brake.get_local_position_normalized() == before:
            break
    # the cylinders empty within the brake's longest delay (BDelay1-4 of its FIZ)
    var release_seconds:float = maxf(maxf(brake.cntrl_brake_delay_1, brake.cntrl_brake_delay_2),
            maxf(brake.cntrl_brake_delay_3, brake.cntrl_brake_delay_4))
    if not await wait_simulated_until(func() -> bool:
            return brake.get_air_pressure() <= MaszynaDriverBraking.RELEASED_BRAKE_PRESSURE,
            release_seconds, "the cylinders emptied"):
        return
    await step(ticks(DRIVER_UPDATE_SECONDS))
    assert_false(_listed(MaszynaDriverHints.Hint.RELEASER_ON),
            "no releaser asked for with the cylinders empty: %s" % _story())


## SN61-02 of the scenery entered and started by nothing but the hints, top to bottom; true when its
## engine runs
func _start_by_hints(scenery:String) -> bool:
    if not await enter_vehicle(scenery, "sn61-02"):
        return false
    _driver = DriverServer.vehicle_get_driver(occupied)
    assert_true(_driver.is_valid(), "the scenery's driver is seated in SN61-02")
    if not _driver.is_valid():
        return false
    _diesel = _engine(powered) as MaszynaMoverRailVehicleDieselEngine
    _cab_logic = CabinSystem.vehicle_get_cab_logic(occupied)
    await step(ticks(DRIVER_UPDATE_SECONDS))

    for _step:int in MAX_STEPS:
        if _running():
            break
        var action:StringName = _hinted_action()
        # nothing hinted on a cold vehicle: the player switches on what the screenshot shows on -
        # the battery, then the cab
        if not action and not _power_supply(occupied).get_power24_available():
            action = &"battery_toggle"
        elif not action and _master(occupied).get_cabin() == 0:
            action = &"cab_activation_toggle"
        elif not action:
            break
        await _hold(action)
    assert_true(_running(), "the engine runs after the hints: %s" % _story())
    return is_passing()


func _listed(hint:MaszynaDriverHints.Hint) -> bool:
    for entry:Dictionary in get_driver_internal_state(_driver).get("hints", []):
        if entry["hint"] == hint:
            return true
    return false


func _running() -> bool:
    return _diesel.get_main_switch_enabled() and _diesel.get_rpm() > 0.0


## The key beside the first listed hint not done yet that has one, or none
func _hinted_action() -> StringName:
    for entry:Dictionary in get_driver_internal_state(_driver).get("hints", []):
        if entry["done"] or not entry["control"]:
            continue
        var action:StringName = _cab_logic.get_action(entry["control"], entry["gesture"])
        if action:
            return action
    return &""


## The key held as a player holds it - real seconds - then the driver's reaction time; the step
## goes into the story with what the window listed and what the vehicle showed
func _hold(action:StringName) -> void:
    var listed:Array[String] = []
    for entry:Dictionary in get_driver_internal_state(_driver).get("hints", []):
        listed.append(entry["text"])
    await key_press(action)
    await step(ticks(KEY_HOLD))
    await key_release(action)
    await step(ticks(DRIVER_UPDATE_SECONDS))
    _followed.append("%s of %s -> %s" % [action, listed, _vehicle_text()])


func _vehicle_text() -> String:
    var state:Dictionary = VehicleServer.vehicle_dump_state(occupied)
    var shown:Array[String] = []
    for key:String in STATE_SHOWN:
        shown.append("%s=%s" % [key, state.get(key)])
    return " ".join(shown)


func _story() -> String:
    return "\n".join(_followed.slice(-6)) + "\nnow: " + _vehicle_text()
