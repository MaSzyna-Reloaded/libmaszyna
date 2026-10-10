extends MaszynaGutTest

## The keys of a cab are its logic's (LegacyCabinLogic.input()), as TTrain::OnCommand_* take them
## (Train.cpp:3156) - a few only with their gauge in the cab's MMD (`requires_gauge`): a control the cab's MMD models takes its key
## with no 3D cab built - the widgets only show. Before, only a widget took the key of a modelled
## control, so a cab without its model (a fixture, a model that failed to load) had dead keys.

const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")
## Frames a knob's key is held for
const KNOB_HOLD_FRAMES:int = 10

var train:VehicleController
var logic:LegacyCabinLogic
## The front cabin, whose controls the logic registers
var cabin:RID


func before_each() -> void:
    train = build_vehicle("TestCabinKeys", SM42, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    # the horn lever's commands (horn_low/horn_high), which the fixture has no horns for
    train.add_component(MoverRailVehicleHorns.new())
    train.apply_configuration()
    var cab_controls:LegacyCabinControls = LegacyCabinControls.new()
    for label:String in ["battery_sw", "mainctrl", "dirkey", "security_reset_bt", "brakectrl", "horn_bt"]:
        var entry:Dictionary = MmdSemanticCatalog.get_entry(label)
        cab_controls.add_control(StringName(label), entry["widget_class"], entry["fixed_fields"],
                CabinButton.ButtonType.TOGGLE, entry.get("target", CabinState.Target.OCCUPIED))
    logic = LegacyCabinLogic.new(func(_cabin:RID) -> LegacyCabinControls: return cab_controls)
    cabin = RailVehicleServer.vehicle_get_front_cabin(train.get_rid())
    logic.register(train.get_rid(), cabin)
    await step(2)


func after_each() -> void:
    logic.unregister()


func _action(action:StringName, pressed:bool) -> InputEventAction:
    var event:InputEventAction = InputEventAction.new()
    event.action = action
    event.pressed = pressed
    return event


func _key(action:String, echo:bool) -> InputEventKey:
    var source:InputEventKey = InputMap.action_get_events(action)[0] as InputEventKey
    var event:InputEventKey = InputEventKey.new()
    event.keycode = source.keycode
    event.physical_keycode = source.physical_keycode
    event.pressed = true
    event.echo = echo
    return event


func _power24() -> bool:
    return (RailVehicleServer.vehicle_component_get(train.get_rid(), RailVehicleComponentType.COMPONENT_POWER_SUPPLY)
            as RailVehiclePowerSupply).get_power24_available()


func test_a_modelled_switch_takes_its_key_without_a_widget() -> void:
    logic.input(_action(&"battery_toggle", true))
    logic.input(_action(&"battery_toggle", false))
    await step(2)
    assert_true(_power24(), "battery_sw is in the cab's MMD and no widget is built - its key switches it")


## A monostable button (the vigilance reset) is held by its key and let go on the key's release
func test_a_monostable_button_is_held_while_its_key_is() -> void:
    logic.input(_action(&"security_acknowledge", true))
    assert_true(CabinSystem.get_control(cabin, &"security_reset_bt"), "pressing the key holds it")
    logic.input(_action(&"security_acknowledge", false))
    assert_false(CabinSystem.get_control(cabin, &"security_reset_bt"), "releasing it lets it go")


## OnCommand_mastercontrollerincrease acts on key repeat too (Train.cpp:1096) - holding + keeps
## stepping the controller; a control without repeat_on_hold (the reverser) ignores the echo
func test_repeat_on_hold_steps_on_key_echo() -> void:
    VehicleServer.vehicle_send_command(train.get_rid(), "battery", true)
    VehicleServer.vehicle_send_command(train.get_rid(), "cab_activation", true)
    await step(2)
    logic.input(_key("direction_increase", false))
    logic.input(_key("direction_increase", true))
    await step(2)
    assert_eq(train.get_direction(), 1, "the reverser takes the press, not the key repeat")

    var master:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
            train.get_rid(), RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
    for echo:bool in [false, true, true]:
        logic.input(_key("main_controller_increase", echo))
        await step(1)
    assert_eq(master.get_main_position(), 3, "press + two key repeats step the controller three times")


## A knob held by its key moves while it is held and stays where it was let go
func test_a_knob_moves_while_its_key_is_held() -> void:
    var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
            train.get_rid(), RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    var before:float = brake.get_controller_position_normalized()
    logic.input(_action(&"brake_level_increase", true))
    await step(KNOB_HOLD_FRAMES)
    logic.input(_action(&"brake_level_increase", false))
    await step(2)
    var held:float = brake.get_controller_position_normalized()
    assert_gt(held, before, "the brake handle moved while the key was held")
    await step(KNOB_HOLD_FRAMES)
    assert_eq(brake.get_controller_position_normalized(), held, "and stays where it was let go")


## The widget is only a view: a key reaches the control once, through the logic, with the widget in
## the tree; a click on the widget is the logic's press too
func test_a_widget_does_not_take_the_key_a_second_time() -> void:
    # the widget finds the logic as the vehicle's, registered for its occupied cab
    logic.unregister()
    CabinSystem.vehicle_attach_cab_logic(train.get_rid(), logic)
    var widget:CabinButton = CabinButton.new()
    widget.control_id = &"battery_sw"
    add_child_autofree(widget)
    widget.set_vehicle_rid(train.get_rid())
    await step(1)

    Input.parse_input_event(_action(&"battery_toggle", true))
    Input.parse_input_event(_action(&"battery_toggle", false))
    logic.input(_action(&"battery_toggle", true))
    logic.input(_action(&"battery_toggle", false))
    await step(2)
    assert_true(_power24(), "one key, one switch - the widget takes no key")

    widget.press()
    widget.release()
    await step(2)
    assert_false(_power24(), "a click is the logic's press")
    CabinSystem.vehicle_attach_cab_logic(train.get_rid(), null)


# Train.cpp:7934, 7978 - a cab with only horn_bt takes both horn keys on it, as the original's
# low and high horn commands accept ggHornButton in place of their own button
func test_a_shared_horn_lever_takes_both_horn_keys() -> void:
    logic.input(_action(&"horn_low", true))
    assert_eq(CabinSystem.get_control(cabin, &"horn_bt"), 1, "low tone")
    logic.input(_action(&"horn_low", false))
    logic.input(_action(&"horn_high", true))
    assert_eq(CabinSystem.get_control(cabin, &"horn_bt"), -1, "high tone")
    logic.input(_action(&"horn_high", false))
