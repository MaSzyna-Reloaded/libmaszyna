extends MaszynaGutTest

## The cab's pantograph selector and the lever that sets the pantographs' valves to it
## (LegacyCabinPantographPresets; TTrain::change_pantograph_selection, update_pantograph_valves,
## OnCommand_pantographselectnext/previous, pantographvalvesupdate/off, Train.cpp:3375-3620). The
## cab is built here around the fixtures' EP07 for its two pantographs; its own cab has no selector.
## Its FIZ has no Switches: section, so the cab walks the default presets "0|1|3|2" (MOVER.h:1724):
## none, the pantograph over the cab's end, both, the other one.

const EP07_PATH:String = "res://tests/fixtures/dynamic/pkp/303e_v1/303e-ep-tv-pantselect.fiz"
## The same EP07 with no Switches: section
const EP07_WITHOUT_SWITCHES_PATH:String = "res://tests/fixtures/dynamic/pkp/303e_v1/303e-ep-tv.fiz"
const FIRST_VALVE:String = "current_collector/pantograph_first_valve_enabled"
const SECOND_VALVE:String = "current_collector/pantograph_second_valve_enabled"
## pantvalves_sw: down, at rest, up (Train.cpp:3575, 3587, 3610)
const LEVER_OFF:int = 0
const LEVER_REST:int = 1
const LEVER_UPDATE:int = 2

var train:VehicleController
var logic:LegacyCabinLogic
var cabin:RID


## The cab of that kind's logic on the fixtures' EP07
func _build_cab(kind:RailVehicleCabinKind.Kind, with_selector:bool, with_lever:bool,
        fiz_path:String = EP07_PATH) -> void:
    # a driven vehicle is simulated (FINDINGS, 09-23); the test drives it, no AI sits aboard
    train = build_vehicle("TestPantographPresets", FizVehicleBuilder.build_description_at(fiz_path), 0.0,
            MaszynaDynamicData.DriverType.DRIVER_HEAD)
    var controls:LegacyCabinControls = LegacyCabinControls.new()
    if with_selector:
        controls.add_control(LegacyCabinPantographPresets.SELECTOR, CabinSwitch, {})
    if with_lever:
        controls.add_control(LegacyCabinPantographPresets.VALVES_LEVER, CabinSwitch, {})
    cabin = (RailVehicleServer.vehicle_get_rear_cabin(train.get_rid())
            if kind == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR
            else RailVehicleServer.vehicle_get_front_cabin(train.get_rid()))
    logic = LegacyCabinLogic.new(func(_cabin:RID) -> LegacyCabinControls: return controls)
    logic.register(train.get_rid(), cabin)
    await wait_idle_frames(2)


func after_each() -> void:
    logic.unregister()


## Whether the first and the second pantograph's valve is open
func _valves() -> Array[bool]:
    var state:Dictionary = VehicleServer.vehicle_dump_state(train.get_rid())
    var valves:Array[bool] = [bool(state.get(FIRST_VALVE, false)), bool(state.get(SECOND_VALVE, false))]
    return valves


func _select(action:StringName) -> void:
    CabinSystem.act(cabin, LegacyCabinPantographPresets.SELECTOR, action)


func _lever(action:StringName, position:int) -> void:
    CabinSystem.act(cabin, LegacyCabinPantographPresets.VALVES_LEVER, action, position)


# Train.cpp:3545 - without the valves lever a new selection sets the valves at once
func test_selector_without_the_lever_sets_the_valves() -> void:
    await _build_cab(RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT, true, false)
    assert_eq(_valves(), [false, false] as Array[bool], "nothing selected")
    _select(&"increase")
    assert_eq(_valves(), [true, false] as Array[bool], "the pantograph over the cab's end")
    _select(&"increase")
    assert_eq(_valves(), [true, true] as Array[bool], "both")
    _select(&"increase")
    assert_eq(_valves(), [false, true] as Array[bool], "the other one")
    _select(&"increase")
    assert_eq(_valves(), [false, true] as Array[bool], "the last preset stays")
    _select(&"decrease")
    assert_eq(_valves(), [true, true] as Array[bool], "back to both")


# Train.cpp:3523-3526 - from the rear cab the ends swap
func test_rear_cab_selects_by_its_own_end() -> void:
    await _build_cab(RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR, true, false)
    _select(&"increase")
    assert_eq(_valves(), [false, true] as Array[bool], "the pantograph over the rear cab's end")


# Train.cpp:3545, 3551, 3592 - with the lever the selection waits for it
func test_lever_sets_and_closes_the_valves() -> void:
    await _build_cab(RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT, true, true)
    _select(&"increase")
    _select(&"increase")
    assert_eq(_valves(), [false, false] as Array[bool], "a selection alone leaves the valves")
    _lever(&"increase", LEVER_UPDATE)
    assert_eq(_valves(), [true, true] as Array[bool], "up sets them to the selection")
    _lever(&"decrease", LEVER_REST)
    assert_eq(_valves(), [true, true] as Array[bool], "its return to rest changes nothing")
    _lever(&"decrease", LEVER_OFF)
    assert_eq(_valves(), [false, false] as Array[bool], "down closes both")


# Train.cpp:3384 - the selector's keys do nothing in a cab without it
func test_selector_keys_without_the_selector_do_nothing() -> void:
    await _build_cab(RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT, false, false)
    _select(&"increase")
    assert_eq(_valves(), [false, false] as Array[bool])


# a vehicle without a Switches: section has no presets to select
func test_selector_without_switches_does_nothing() -> void:
    await _build_cab(RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT, true, false, EP07_WITHOUT_SWITCHES_PATH)
    _select(&"increase")
    assert_eq(_valves(), [false, false] as Array[bool])
