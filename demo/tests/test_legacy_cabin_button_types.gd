extends MaszynaGutTest

## A control whose original handler branches on the kind of switch (TGaugeType) behaves by the type
## the cab's MMD gives it (LegacyCabinControls.button_type()), as TTrain branches on ggX.type().

const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")

var train: VehicleController
var logic: LegacyCabinLogic
## The front cabin, whose controls the logic registers
var cabin:RID


func _build_cab(controls:Dictionary[StringName, CabinButton.ButtonType],
        model:VehicleController = SM42, components:Array[VehicleComponent] = [],
        fields:Dictionary[StringName, Dictionary] = {}) -> void:
    train = build_vehicle("TestButtonTypes", model)
    for component:VehicleComponent in components:
        train.add_component(component)
    # SM42 brings its own power supply, a bare vehicle needs one for the cab's low voltage
    if model == null:
        train.add_component(build_power_supply(110.0))
    train.apply_configuration()
    var cab_controls: LegacyCabinControls = LegacyCabinControls.new()
    for control_id:StringName in controls:
        cab_controls.add_control(control_id, CabinButton, fields.get(control_id, {}), controls[control_id])
    logic = LegacyCabinLogic.new(func(_cabin:RID) -> LegacyCabinControls: return cab_controls)
    cabin = RailVehicleServer.vehicle_get_front_cabin(train.get_rid())
    logic.register(train.get_rid(), cabin)
    await step(2)


func after_each():
    logic.unregister()


# Train.cpp:3914 - an impulse fuel pump switch runs the pump while it is held
func test_push_fuel_pump_runs_only_while_held():
    var controls:Dictionary[StringName, CabinButton.ButtonType] = {
        &"fuelpump_sw": CabinButton.ButtonType.PUSH}
    await _build_cab(controls)
    CabinSystem.act(cabin, &"fuelpump_sw", &"hold")
    assert_true(train.get_state()["fuel_pump_enabled"], "held")
    CabinSystem.act(cabin, &"fuelpump_sw", &"release")
    assert_false(train.get_state()["fuel_pump_enabled"], "released")


# Train.cpp:3889-3900 - a two-state one flips on a press and ignores the release
func test_two_state_fuel_pump_flips_on_a_press():
    var controls:Dictionary[StringName, CabinButton.ButtonType] = {
        &"fuelpump_sw": CabinButton.ButtonType.TOGGLE}
    await _build_cab(controls)
    CabinSystem.act(cabin, &"fuelpump_sw", &"hold")
    CabinSystem.act(cabin, &"fuelpump_sw", &"release")
    assert_true(train.get_state()["fuel_pump_enabled"], "stays on after the release")
    CabinSystem.act(cabin, &"fuelpump_sw", &"hold")
    assert_false(train.get_state()["fuel_pump_enabled"], "the next press turns it off")


# Train.cpp:2891, 2929 - an impulse battery switch flips the battery on a press, and its release
# only returns it to neutral
func test_push_battery_switch_flips_on_each_press():
    var controls:Dictionary[StringName, CabinButton.ButtonType] = {
        &"battery_sw": CabinButton.ButtonType.PUSH}
    await _build_cab(controls)
    CabinSystem.act(cabin, &"battery_sw", &"hold")
    CabinSystem.act(cabin, &"battery_sw", &"release")
    await step(2)
    assert_true(train.get_state()["battery_enabled"], "the first press switches it on")
    CabinSystem.act(cabin, &"battery_sw", &"hold")
    CabinSystem.act(cabin, &"battery_sw", &"release")
    await step(2)
    assert_false(train.get_state()["battery_enabled"], "the second one off")


# Train.cpp:3815-3824 - without main_on_bt the closing key moves an impulse main_sw up, and its
# release brings it back midway
func test_the_closing_key_moves_an_impulse_main_switch():
    var controls:Dictionary[StringName, CabinButton.ButtonType] = {
        &"main_sw": CabinButton.ButtonType.PUSH}
    await _build_cab(controls)
    CabinSystem.act(cabin, &"main_on_bt", &"hold")
    assert_eq(CabinSystem.get_control(cabin, &"main_sw"), LegacyCabinMainSwitch.LEVER_CLOSE)
    CabinSystem.act(cabin, &"main_on_bt", &"release")
    assert_eq(CabinSystem.get_control(cabin, &"main_sw"), LegacyCabinMainSwitch.LEVER_REST)


# Train.cpp:3474, 3455 - an impulse pantselected_sw goes up to raise and comes back midway
func test_an_impulse_pantograph_lever_goes_up_and_back_to_rest():
    var controls:Dictionary[StringName, CabinButton.ButtonType] = {
        &"pantselected_sw": CabinButton.ButtonType.PUSH}
    # the pantographs' valve the lever works is the power source's
    var components:Array[VehicleComponent] = [
        MoverRailVehicleElectricSeriesEngine.new(), MoverRailVehicleEnginePowerSource.new()]
    await _build_cab(controls, null, components)
    CabinSystem.act(cabin, &"pantselected_sw", &"hold")
    assert_eq(CabinSystem.get_control(cabin, &"pantselected_sw"), LegacyCabinPantographSelected.LEVER_UP)
    CabinSystem.act(cabin, &"pantselected_sw", &"release")
    assert_eq(CabinSystem.get_control(cabin, &"pantselected_sw"), LegacyCabinPantographSelected.LEVER_REST)


func _electric_components(impulse:bool) -> Array[VehicleComponent]:
    var engine := MoverRailVehicleElectricSeriesEngine.new()
    var power_source := MoverRailVehicleEnginePowerSource.new()
    power_source.source_type = RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR
    power_source.current_collector_number_of_collectors = 2
    var switches := MoverRailVehicleSwitches.new()
    switches.pantograph_impulse = impulse
    var components:Array[VehicleComponent] = [engine, power_source, switches]
    return components


# Train.cpp:3218-3300 - with impulse pantograph switches a press opens one side of the valve and the
# release lets go; lowering needs the lowering button, which a cab may declare with no submodel
func test_impulse_pantograph_switch_raises_and_lowers_through_its_valve():
    var controls:Dictionary[StringName, CabinButton.ButtonType] = {
        &"pantfront_sw": CabinButton.ButtonType.TOGGLE, &"pantfrontoff_sw": CabinButton.ButtonType.TOGGLE}
    await _build_cab(controls, null, _electric_components(true))
    CabinSystem.act(cabin, &"pantfront_sw", &"hold")
    assert_true(train.get_state()["current_collector/pantograph_first_valve_enabled"], "held up")
    CabinSystem.act(cabin, &"pantfront_sw", &"release")
    assert_false(train.get_state()["current_collector/pantograph_first_valve_enabled"], "let go")
    CabinSystem.act(cabin, &"pantfrontoff_sw", &"hold")
    assert_false(train.get_state()["current_collector/pantograph_first_valve_enabled"])


# Train.cpp:3285 - an impulse type without the lowering button cannot lower from the cab
func test_impulse_pantograph_cannot_be_lowered_without_its_lowering_button():
    var controls:Dictionary[StringName, CabinButton.ButtonType] = {
        &"pantfront_sw": CabinButton.ButtonType.TOGGLE}
    await _build_cab(controls, null, _electric_components(true))
    assert_null(CabinSystem.act(cabin, &"pantfrontoff_sw", &"hold"))


# Train.cpp:3239 - a two-state switch sets the valve and keeps it
func test_two_state_pantograph_switch_keeps_its_valve():
    var controls:Dictionary[StringName, CabinButton.ButtonType] = {
        &"pantfront_sw": CabinButton.ButtonType.TOGGLE}
    await _build_cab(controls, null, _electric_components(false))
    CabinSystem.act(cabin, &"pantfront_sw", &"toggle", true)
    assert_true(train.get_state()["current_collector/pantograph_first_valve_enabled"])
    CabinSystem.act(cabin, &"pantfront_sw", &"toggle", false)
    assert_false(train.get_state()["current_collector/pantograph_first_valve_enabled"])


# Train.cpp:2939-3070 - batteryon_sw/batteryoff_sw switch the battery on their press
func test_battery_on_and_off_buttons_switch_the_battery():
    var controls:Dictionary[StringName, CabinButton.ButtonType] = {
        &"batteryon_sw": CabinButton.ButtonType.PUSH, &"batteryoff_sw": CabinButton.ButtonType.PUSH}
    await _build_cab(controls)
    CabinSystem.act(cabin, &"batteryon_sw", &"hold")
    CabinSystem.act(cabin, &"batteryon_sw", &"release")
    await step(2)
    assert_true(train.get_state()["battery_enabled"], "on")
    CabinSystem.act(cabin, &"batteryoff_sw", &"hold")
    CabinSystem.act(cabin, &"batteryoff_sw", &"release")
    await step(2)
    assert_false(train.get_state()["battery_enabled"], "off")


var _sent:Array = []


func _on_command(_vehicle:RID, command:String, p1:Variant, _p2:Variant) -> void:
    _sent.append([command, p1])


# Train.cpp:6955 - speedbuttonN picks speed N on its press, and nothing on the release
func test_speed_button_sends_its_number_on_the_press():
    var controls:Dictionary[StringName, CabinButton.ButtonType] = {&"speedbutton3": CabinButton.ButtonType.PUSH}
    var fields:Dictionary[StringName, Dictionary] = {
        &"speedbutton3": MmdSemanticCatalog.get_entry("speedbutton3")["fixed_fields"]}
    var components:Array[VehicleComponent] = [MoverRailVehicleSpeedControl.new()]
    await _build_cab(controls, SM42, components, fields)
    VehicleServer.vehicle_command_received.connect(_on_command)
    CabinSystem.act(cabin, &"speedbutton3", &"hold")
    CabinSystem.act(cabin, &"speedbutton3", &"release")
    VehicleServer.vehicle_command_received.disconnect(_on_command)
    assert_eq(_sent.filter(func(sent:Array) -> bool: return sent[0] == "speed_control_button"), [["speed_control_button", 3]])

