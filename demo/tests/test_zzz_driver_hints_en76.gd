extends MaszynaStartupTest

## EN76-005a (pkp/elf_v1, fixtures/scenery/startup_en76-005a.scn) as the player found it and handled by
## nothing but the driving aid's hints, each by the key the hints window shows beside it, or by the
## cab's button where it shows no key. Its main pipe locks while it is empty (LPOn=3.0 LPOff=4.5,
## 22we-a.fiz) and only the button of UBB1=2 charges it, as it has no HandlePipeUnlockPos.
## Given 0.1 km/h as in l053_poludnie.scn it starts ready with the pipe full, but its cab is inactive
## (AutomaticCabActivation=No) and InactiveCabFlag=3 vents the pipe within seconds, so it locks.
## Report reports#12: the main reservoir full, the pipe at 0, the handle at running, nothing hinted
## the button, and the player stood.

const SCENERY:String = "startup_en76-005a.scn"
const DRIVEN:String = "en76-005a"
## The velocity told to the driver, as the timetable tells it at the departure [km/h]
const DEPARTURE_VELOCITY:float = 40.0
## Simulated seconds the driver is given to list its hints for the departure
const DEPARTURE_SECONDS:float = 30.0
## What the player is not told while the pipe is locked: nothing of it acts on an empty pipe
const BRAKE_HINTS:Array[MaszynaDriverHints.Hint] = [
    MaszynaDriverHints.Hint.INDEPENDENT_BRAKE_APPLY, MaszynaDriverHints.Hint.BRAKING_FORCE_DECREASE,
    MaszynaDriverHints.Hint.BRAKING_FORCE_INCREASE, MaszynaDriverHints.Hint.BRAKING_FORCE_SET_ZERO,
    MaszynaDriverHints.Hint.BRAKING_FORCE_LAP, MaszynaDriverHints.Hint.TRAIN_BRAKE_RELEASE,
]
## Hints the player follows at most
const MAX_STEPS:int = 40
## Simulated seconds a hint's key is held, as a player holds it
const KEY_HOLD:float = 2.0
## Simulated seconds the driver is given to update its list after a step (its reaction time)
const DRIVER_UPDATE_SECONDS:float = 5.0
## Simulated seconds a cab button without a key is held at most
const BUTTON_HOLD_TIMEOUT:float = 180.0
## Driver updates a player waits at most while only hints without a control are listed
const MAX_WAITS:int = 24
## The pressure the lock lets go above [bar] (LPOff=4.5, 22we-a.fiz)
const UNLOCKED_PIPE:float = 4.5
## The cab's button of UBB1=2 (bt-napelnianie_pg)
const UNLOCK_BUTTON:StringName = &"universalbrake1_bt"
## What a failure shows of the vehicle
const STATE_SHOWN:Array[String] = [
    "battery_enabled", "cabin", "direction", "main_switch_enabled", "converter_enabled",
    "brake_controller_position", "brake_air_pressure", "pipe_pressure", "brake_control_pressure",
    "main_pipe_locked", "cabin_controleable", "compressor_pressure",
]

var _driver:RID
var _cab_logic:CabinLogic
## What the player did, in order, with the list shown each time - the failure's story
var _followed:Array[String] = []
var _unlock_hinted:bool = false


func test_the_hints_alone_charge_the_empty_main_pipe() -> void:
    if not await enter_vehicle(SCENERY, DRIVEN):
        return
    _driver = DriverServer.vehicle_get_driver(occupied)
    assert_true(_driver.is_valid(), "the scenery's driver is seated in EN76")
    if not _driver.is_valid():
        return
    _cab_logic = CabinSystem.vehicle_get_cab_logic(occupied)
    await step(ticks(DRIVER_UPDATE_SECONDS))
    _followed.append("start: %s; listed %s" % [_vehicle_text(), _listed_texts()])
    await _follow_hints()
    gut.p("\n".join(_followed))
    gut.p("now: " + _vehicle_text())
    var brake:RailVehicleBrake = _brake(occupied)
    assert_true(_unlock_hinted, "the locked pipe's button hinted")
    assert_false(brake.get_main_pipe_locked(), "the main pipe unlocked")
    assert_gt(brake.get_pipe_pressure(), UNLOCKED_PIPE, "the main pipe charged by the hints alone")


## The report's own state: told to go with the pipe locked and the button never held
func test_a_locked_pipe_is_the_only_brake_hint_of_a_train_told_to_go() -> void:
    if not await enter_vehicle(SCENERY, DRIVEN):
        return
    _driver = DriverServer.vehicle_get_driver(occupied)
    _cab_logic = CabinSystem.vehicle_get_cab_logic(occupied)
    await step(ticks(DRIVER_UPDATE_SECONDS))
    await _hold(&"cab_activation_toggle")
    DriverServer.driver_send_command(_driver, "SetVelocity", DEPARTURE_VELOCITY, DEPARTURE_VELOCITY)
    await step(ticks(DEPARTURE_SECONDS))
    assert_true(_brake(occupied).get_main_pipe_locked(), "the pipe still locked")
    var listed:Array = []
    for entry:Dictionary in get_driver_internal_state(_driver).get("hints", []):
        if not entry["done"]:
            listed.append(entry["hint"])
    assert_has(listed, MaszynaDriverHints.Hint.MAIN_PIPE_UNLOCK, "the button hinted: %s" % [_listed_texts()])
    for hint:MaszynaDriverHints.Hint in BRAKE_HINTS:
        assert_does_not_have(listed, hint, "no brake hint beside the locked pipe: %s" % [_listed_texts()])


## The hints followed, top to bottom, until none is left to do
func _follow_hints() -> void:
    var waits:int = 0
    for _step:int in MAX_STEPS:
        var entry:Dictionary = _hinted()
        if entry:
            if entry["hint"] == MaszynaDriverHints.Hint.MAIN_PIPE_UNLOCK:
                _unlock_hinted = true
                assert_eq(entry["control"], UNLOCK_BUTTON, "the hint shows the button that unlocks the pipe")
            var action:StringName = _cab_logic.get_action(entry["control"], entry["gesture"])
            if action:
                await _hold(action)
            else:
                await _hold_button(entry)
        elif not _power_supply(occupied).get_power24_available():
            await _hold(&"battery_toggle")
        elif _master(occupied).get_cabin() == 0:
            await _hold(&"cab_activation_toggle")
        elif waits < MAX_WAITS and _brake(occupied).get_main_pipe_locked():
            waits += 1
            await step(ticks(DRIVER_UPDATE_SECONDS))
            _followed.append("waited: %s; listed %s" % [_vehicle_text(), _listed_texts()])
        else:
            break


## The first listed hint not done yet that shows a control, or none
func _hinted() -> Dictionary:
    for entry:Dictionary in get_driver_internal_state(_driver).get("hints", []):
        if not entry["done"] and entry["control"]:
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


## The key held as a player holds it, then the driver's reaction time
func _hold(action:StringName) -> void:
    var listed:Array[String] = _listed_texts()
    await key_press(action)
    await step(ticks(KEY_HOLD))
    await key_release(action)
    await step(ticks(DRIVER_UPDATE_SECONDS))
    _followed.append("%s of %s -> %s" % [action, listed, _vehicle_text()])


func _listed_texts() -> Array[String]:
    var listed:Array[String] = []
    for entry:Dictionary in get_driver_internal_state(_driver).get("hints", []):
        if not entry["done"]:
            listed.append(entry["text"])
    return listed


func _vehicle_text() -> String:
    var state:Dictionary = VehicleServer.vehicle_dump_state(occupied)
    var shown:Array[String] = []
    for key:String in STATE_SHOWN:
        shown.append("%s=%s" % [key, state.get(key)])
    return " ".join(shown)
