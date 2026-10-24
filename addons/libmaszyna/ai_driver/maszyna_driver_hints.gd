@tool
extends RefCounted
class_name MaszynaDriverHints

## The original's driver hints (driver_hint, driverhints_def.h; TController::cue_action(), hint(),
## update_hints(), driverhints.cpp:16-1350): every step the driver decides on is cued - a driver
## the computer is (AIControllFlag) takes it, and either way the step is kept in the driver's list
## until the vehicle shows it done, so a player who drives sees what the driver would do. The list
## is the driver's own (MaszynaAIDriver.AIDriverState.hints), changed only by cue() and
## update(). The original's AI sets the Mover directly; here it operates the cab's control, as a
## player does (CabinSystem.act()), or gives the vehicle its command.

## driverhints_def.h, in its order
enum Hint {
    BATTERY_ON,
    BATTERY_OFF,
    RADIO_ON,
    RADIO_OFF,
    RADIO_CHANNEL,
    OIL_PUMP_ON,
    OIL_PUMP_OFF,
    FUEL_PUMP_ON,
    FUEL_PUMP_OFF,
    PANTOGRAPH_AIR_SOURCE_SET_MAIN,
    PANTOGRAPH_AIR_SOURCE_SET_AUXILIARY,
    PANTOGRAPH_COMPRESSOR_ON,
    PANTOGRAPH_COMPRESSOR_OFF,
    PANTOGRAPHS_VALVE_ON,
    PANTOGRAPHS_VALVE_OFF,
    FRONT_PANTOGRAPH_VALVE_ON,
    FRONT_PANTOGRAPH_VALVE_OFF,
    REAR_PANTOGRAPH_VALVE_ON,
    REAR_PANTOGRAPH_VALVE_OFF,
    CONVERTER_ON,
    CONVERTER_OFF,
    PRIMARY_CONVERTER_OVERLOAD_RESET,
    MAIN_CIRCUIT_GROUND_RESET,
    TRACTION_MOTOR_OVERLOAD_RESET,
    LINE_BREAKER_CLOSE,
    LINE_BREAKER_OPEN,
    COMPRESSOR_ON,
    COMPRESSOR_OFF,
    FRONT_MOTOR_BLOWERS_ON,
    FRONT_MOTOR_BLOWERS_OFF,
    REAR_MOTOR_BLOWERS_ON,
    REAR_MOTOR_BLOWERS_OFF,
    SPRING_BRAKE_ON,
    SPRING_BRAKE_OFF,
    MANUAL_BRAKE_ON,
    MANUAL_BRAKE_OFF,
    MASTER_CONTROLLER_SET_IDLE,
    MASTER_CONTROLLER_SET_SERIES_MODE,
    WATER_HEATER_ON,
    WATER_HEATER_OFF,
    WATER_HEATER_BREAKER_ON,
    WATER_HEATER_BREAKER_OFF,
    WATER_PUMP_ON,
    WATER_PUMP_OFF,
    WATER_PUMP_BREAKER_ON,
    WATER_PUMP_BREAKER_OFF,
    WATER_CIRCUITS_LINK_ON,
    WATER_CIRCUITS_LINK_OFF,
    WAIT_TEMPERATURE_TOO_LOW,
    MASTER_CONTROLLER_SET_ZERO_SPEED,
    MASTER_CONTROLLER_SET_REVERSER_UNLOCK,
    TRAIN_BRAKE_SET_PIPE_UNLOCK,
    ## Not the original's: it holds the button with no hint (control_main_pipe(), Driver.cpp:8252)
    MAIN_PIPE_UNLOCK,
    TRAIN_BRAKE_RELEASE,
    TRAIN_BRAKE_APPLY,
    DIRECTION_FORWARD,
    DIRECTION_BACKWARD,
    DIRECTION_OTHER,
    DIRECTION_NONE,
    WAIT_PRESSURE_TOO_LOW,
    WAIT_PANTOGRAPH_PRESSURE_TOO_LOW,
    SANDING_ON,
    SANDING_OFF,
    CONSIST_DOOR_LOCKS_ON,
    DEPARTURE_SIGNAL_ON,
    DEPARTURE_SIGNAL_OFF,
    DOOR_RIGHT_OPEN,
    DOOR_RIGHT_CLOSE,
    DOOR_LEFT_OPEN,
    DOOR_LEFT_CLOSE,
    DOOR_RIGHT_PERMIT_ON,
    DOOR_RIGHT_PERMIT_OFF,
    DOOR_LEFT_PERMIT_ON,
    DOOR_LEFT_PERMIT_OFF,
    HORN_ON,
    HORN_OFF,
    CONSIST_LIGHTS_ON,
    CONSIST_LIGHTS_OFF,
    CONSIST_HEATING_ON,
    CONSIST_HEATING_OFF,
    SECURITY_SYSTEM_RESET,
    SHP_SYSTEM_RESET,
    COUPLING_ADAPTER_ATTACH,
    COUPLING_ADAPTER_REMOVE,
    SECOND_CONTROLLER_SET_ZERO,
    TRACTIVE_FORCE_DECREASE,
    TRACTIVE_FORCE_INCREASE,
    BRAKING_FORCE_DECREASE,
    BRAKING_FORCE_INCREASE,
    BRAKING_FORCE_SET_ZERO,
    BRAKING_FORCE_LAP,
    INDEPENDENT_BRAKE_APPLY,
    INDEPENDENT_BRAKE_RELEASE,
    ## Not the original's: a car of the trainset left with its own independent brake applied holds
    ## it, released only from that car's cab
    INDEPENDENT_BRAKE_RELEASE_OTHER_END,
    ANTISLIP,
    WAIT_LOAD_EXCHANGE,
    WAIT_DEPARTURE_TIME,
    HEADCODE_PC1,
    HEADCODE_PC2,
    HEADCODE_PC5,
    HEADCODE_TB1,
    LIGHTS_OFF,
    RELEASER_ON,
    RELEASER_OFF,
    BUFFERS_COMPRESS,
    CAB_ACTIVATION,
    CAB_DEACTIVATION,
    ## Not the original's: it asks for the deactivation in a cab that cannot do it - only the cab
    ## with the control can (CabDeactivisation(), IsCabMaster(), Mover.cpp:2684)
    CAB_DEACTIVATION_OTHER_END,
}

## The vehicle of the driver's trainset a hint's device belongs to: the one it sits in
## (mvOccupied), the engine its controls drive (mvControlling) and the one whose pantographs it
## raises (mvPantographUnit)
enum Device {
    OCCUPIED,
    CONTROLLING,
    PANTOGRAPH_UNIT,
}

## The text of each hint (driverhints_def.h), the msgid the game's catalogue translates - kept to
## the letter, the original's typo too; `%.0f` takes the hint's parameter
const TEXTS:Dictionary[Hint, String] = {
    Hint.BATTERY_ON: "Switch on battery",
    Hint.BATTERY_OFF: "Switch off battery",
    Hint.RADIO_ON: "Switch on radio",
    Hint.RADIO_OFF: "Switch off radio",
    Hint.RADIO_CHANNEL: "Tune into channel %.0f",
    Hint.OIL_PUMP_ON: "Switch on oil pump",
    Hint.OIL_PUMP_OFF: "Switch off oil pump",
    Hint.FUEL_PUMP_ON: "Switch on fuel pump",
    Hint.FUEL_PUMP_OFF: "Switch off fuel pump",
    Hint.PANTOGRAPH_AIR_SOURCE_SET_MAIN: "Switch pantograph 3-way valve to primary air source",
    Hint.PANTOGRAPH_AIR_SOURCE_SET_AUXILIARY: "Switch pantograph 3-way valve to auxiliary air source",
    Hint.PANTOGRAPH_COMPRESSOR_ON: "Switch on pantograph compressor",
    Hint.PANTOGRAPH_COMPRESSOR_OFF: "Switch off pantograph compressor",
    Hint.PANTOGRAPHS_VALVE_ON: "Enable pantographs valve",
    Hint.PANTOGRAPHS_VALVE_OFF: "Disable pantographcs valve",
    Hint.FRONT_PANTOGRAPH_VALVE_ON: "Raise pantograph A",
    Hint.FRONT_PANTOGRAPH_VALVE_OFF: "Lower pantograph A",
    Hint.REAR_PANTOGRAPH_VALVE_ON: "Raise pantograph B",
    Hint.REAR_PANTOGRAPH_VALVE_OFF: "Lower pantograph B",
    Hint.CONVERTER_ON: "Switch on converter",
    Hint.CONVERTER_OFF: "Switch off converter",
    Hint.PRIMARY_CONVERTER_OVERLOAD_RESET: "Reset converter overload relay",
    Hint.MAIN_CIRCUIT_GROUND_RESET: "Reset main circuit ground relay",
    Hint.TRACTION_MOTOR_OVERLOAD_RESET: "Reset traction motors overload relay",
    Hint.LINE_BREAKER_CLOSE: "Close line breaker",
    Hint.LINE_BREAKER_OPEN: "Open line breaker",
    Hint.COMPRESSOR_ON: "Switch on compressor",
    Hint.COMPRESSOR_OFF: "Switch off compressor",
    Hint.FRONT_MOTOR_BLOWERS_ON: "Switch on front motor blowers",
    Hint.FRONT_MOTOR_BLOWERS_OFF: "Switch off front motor blowers",
    Hint.REAR_MOTOR_BLOWERS_ON: "Switch on rear motor blowers",
    Hint.REAR_MOTOR_BLOWERS_OFF: "Switch off rear motor blowers",
    Hint.SPRING_BRAKE_ON: "Apply spring brake",
    Hint.SPRING_BRAKE_OFF: "Release spring brake",
    Hint.MANUAL_BRAKE_ON: "Apply manual brake",
    Hint.MANUAL_BRAKE_OFF: "Release manual brake",
    Hint.MASTER_CONTROLLER_SET_IDLE: "Set engine to idle",
    Hint.MASTER_CONTROLLER_SET_SERIES_MODE: "Set master controller to series mode",
    Hint.WATER_HEATER_ON: "Switch on water heater",
    Hint.WATER_HEATER_OFF: "Switch off water heater",
    Hint.WATER_HEATER_BREAKER_ON: "Switch on water heater breaker",
    Hint.WATER_HEATER_BREAKER_OFF: "Switch off water heater breaker",
    Hint.WATER_PUMP_ON: "Switch on water pump",
    Hint.WATER_PUMP_OFF: "Switch off water pump",
    Hint.WATER_PUMP_BREAKER_ON: "Switch on water pump breaker",
    Hint.WATER_PUMP_BREAKER_OFF: "Switch off water pump breaker",
    Hint.WATER_CIRCUITS_LINK_ON: "Link water circuits",
    Hint.WATER_CIRCUITS_LINK_OFF: "Unlink water circuits",
    Hint.WAIT_TEMPERATURE_TOO_LOW: "Wait for warm up to complete",
    Hint.MASTER_CONTROLLER_SET_ZERO_SPEED: "Set master controller to neutral",
    Hint.MASTER_CONTROLLER_SET_REVERSER_UNLOCK: "Set master controller to position %.0f",
    Hint.TRAIN_BRAKE_SET_PIPE_UNLOCK: "Set brake controller to pipe filling mode",
    Hint.MAIN_PIPE_UNLOCK: "Hold the main pipe unlock button",
    Hint.TRAIN_BRAKE_RELEASE: "Release train brake",
    Hint.TRAIN_BRAKE_APPLY: "Apply train brake",
    Hint.DIRECTION_FORWARD: "Set reverser to forward",
    Hint.DIRECTION_BACKWARD: "Set reverser to reverse",
    Hint.DIRECTION_OTHER: "Switch reverser to opposite direction",
    Hint.DIRECTION_NONE: "Set reverser to neutral",
    Hint.WAIT_PRESSURE_TOO_LOW: "Wait for main reservoir to fill",
    Hint.WAIT_PANTOGRAPH_PRESSURE_TOO_LOW: "Wait for sufficient air pressure in pantograph subsystem",
    Hint.SANDING_ON: "Switch on sanding",
    Hint.SANDING_OFF: "Switch off sanding",
    Hint.CONSIST_DOOR_LOCKS_ON: "Switch on door locks",
    Hint.DEPARTURE_SIGNAL_ON: "Switch on departure signal",
    Hint.DEPARTURE_SIGNAL_OFF: "Switch off departure signal",
    Hint.DOOR_RIGHT_OPEN: "Open doors",
    Hint.DOOR_RIGHT_CLOSE: "Close doors",
    Hint.DOOR_LEFT_OPEN: "Open doors",
    Hint.DOOR_LEFT_CLOSE: "Close doors",
    Hint.DOOR_RIGHT_PERMIT_ON: "Grant permit to open doors",
    Hint.DOOR_RIGHT_PERMIT_OFF: "Revoke permit to open doors",
    Hint.DOOR_LEFT_PERMIT_ON: "Grant permit to open doors",
    Hint.DOOR_LEFT_PERMIT_OFF: "Revoke permit to open doors",
    Hint.HORN_ON: "Sound the horn",
    Hint.HORN_OFF: "Switch off horn",
    Hint.CONSIST_LIGHTS_ON: "Switch on consist lights",
    Hint.CONSIST_LIGHTS_OFF: "Switch off consist lights",
    Hint.CONSIST_HEATING_ON: "Switch on consist heating",
    Hint.CONSIST_HEATING_OFF: "Switch off consist heating",
    Hint.SECURITY_SYSTEM_RESET: "Acknowledge alerter",
    Hint.SHP_SYSTEM_RESET: "Acknowledge SHP",
    Hint.COUPLING_ADAPTER_ATTACH: "Attach coupling adapter",
    Hint.COUPLING_ADAPTER_REMOVE: "Remove coupling adapter",
    Hint.SECOND_CONTROLLER_SET_ZERO: "Switch off field shunting",
    Hint.TRACTIVE_FORCE_DECREASE: "Reduce tractive force",
    Hint.TRACTIVE_FORCE_INCREASE: "Increase tractive force",
    Hint.BRAKING_FORCE_DECREASE: "Reduce braking force",
    Hint.BRAKING_FORCE_INCREASE: "Increase braking force",
    Hint.BRAKING_FORCE_SET_ZERO: "Release train brakes",
    Hint.BRAKING_FORCE_LAP: "Lap train brake",
    Hint.INDEPENDENT_BRAKE_APPLY: "Apply independent brake",
    Hint.INDEPENDENT_BRAKE_RELEASE: "Release independent brake",
    Hint.INDEPENDENT_BRAKE_RELEASE_OTHER_END: "Release the independent brake at the other end",
    Hint.ANTISLIP: "Apply anti slip brake",
    Hint.WAIT_LOAD_EXCHANGE: "Wait for passenger exchange to complete",
    Hint.WAIT_DEPARTURE_TIME: "Wait for departure time",
    Hint.HEADCODE_PC1: "Switch on Pc 1 head lamp code",
    Hint.HEADCODE_PC2: "Switch on Pc 2 head lamp code",
    Hint.HEADCODE_PC5: "Switch on Pc 5 tail lamp code",
    Hint.HEADCODE_TB1: "Switch on Tb 1 head lamp code",
    Hint.LIGHTS_OFF: "Switch off lights",
    Hint.RELEASER_ON: "Actuate",
    Hint.RELEASER_OFF: "Stop actuating",
    Hint.BUFFERS_COMPRESS: "Apply tractive force to compress buffers",
    Hint.CAB_ACTIVATION: "Activate cabin",
    Hint.CAB_DEACTIVATION: "Deactivate cabin",
    Hint.CAB_DEACTIVATION_OTHER_END: "Deactivate the cabin at the other end",
}

## A switch of the cab: the switch of a hint and the position it wants
const SWITCHES:Dictionary = {
    Hint.BATTERY_ON: [&"battery_sw", true],
    Hint.BATTERY_OFF: [&"battery_sw", false],
    Hint.CAB_ACTIVATION: [&"cabactivation_sw", true],
    Hint.RADIO_ON: [&"radio_sw", true],
    Hint.RADIO_OFF: [&"radio_sw", false],
    Hint.OIL_PUMP_ON: [&"oilpump_sw", true],
    Hint.OIL_PUMP_OFF: [&"oilpump_sw", false],
    Hint.FUEL_PUMP_ON: [&"fuelpump_sw", true],
    Hint.FUEL_PUMP_OFF: [&"fuelpump_sw", false],
    Hint.CONVERTER_ON: [&"converter_sw", true],
    Hint.CONVERTER_OFF: [&"converter_sw", false],
    Hint.COMPRESSOR_ON: [&"compressor_sw", true],
    Hint.COMPRESSOR_OFF: [&"compressor_sw", false],
}
## A button of the cab pressed and let go: the relays' resets
const BUTTONS:Dictionary[Hint, StringName] = {
    Hint.PRIMARY_CONVERTER_OVERLOAD_RESET: &"converterfuse_bt",
    Hint.TRACTION_MOTOR_OVERLOAD_RESET: &"fuse_bt",
}
## The pantographs' valves are operated directly, as the original's driver does
## (`mvOccupied->OperatePantographValve(end::front, operation_t::enable)`, driverhints.cpp:269-313):
## through the cab they could not be - a cab with a pantograph selector (pantselect_sw: E186,
## ES64F4) has no pantfront_sw/pantrear_sw, and its selector takes the valves over (Train.cpp:3154),
## so its driver never raised them (docs/findings-archive.md, 2026-10-03 scenarios that did not
## start). The valve and the operation of a hint
const PANTOGRAPH_VALVES:Dictionary = {
    Hint.FRONT_PANTOGRAPH_VALVE_ON: [RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, RailVehicleEnginePowerSource.VALVE_OPERATION_ENABLE],
    Hint.FRONT_PANTOGRAPH_VALVE_OFF: [RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, RailVehicleEnginePowerSource.VALVE_OPERATION_DISABLE],
    Hint.REAR_PANTOGRAPH_VALVE_ON: [RailVehicleEnginePowerSource.PANTOGRAPH_SECOND, RailVehicleEnginePowerSource.VALVE_OPERATION_ENABLE],
    Hint.REAR_PANTOGRAPH_VALVE_OFF: [RailVehicleEnginePowerSource.PANTOGRAPH_SECOND, RailVehicleEnginePowerSource.VALVE_OPERATION_DISABLE],
}
## A hint the original takes outside the cab, by the vehicle's command: the vehicle, the command and
## its value (null for none)
const COMMANDS:Dictionary = {
    Hint.PANTOGRAPH_AIR_SOURCE_SET_MAIN: [Device.PANTOGRAPH_UNIT, &"pantograph_compressor_valve", false],
    Hint.PANTOGRAPH_AIR_SOURCE_SET_AUXILIARY: [Device.PANTOGRAPH_UNIT, &"pantograph_compressor_valve", true],
    Hint.PANTOGRAPH_COMPRESSOR_ON: [Device.PANTOGRAPH_UNIT, &"pantograph_compressor", true],
    Hint.PANTOGRAPH_COMPRESSOR_OFF: [Device.PANTOGRAPH_UNIT, &"pantograph_compressor", false],
    Hint.PANTOGRAPHS_VALVE_ON: [Device.PANTOGRAPH_UNIT, &"pantographs_valve", true],
    Hint.WATER_PUMP_ON: [Device.CONTROLLING, &"water_pump", true],
    Hint.WATER_PUMP_OFF: [Device.CONTROLLING, &"water_pump", false],
    Hint.WATER_PUMP_BREAKER_ON: [Device.CONTROLLING, &"water_pump_breaker", true],
    Hint.WATER_PUMP_BREAKER_OFF: [Device.CONTROLLING, &"water_pump_breaker", false],
    Hint.WATER_HEATER_ON: [Device.CONTROLLING, &"water_heater", true],
    Hint.WATER_HEATER_OFF: [Device.CONTROLLING, &"water_heater", false],
    Hint.WATER_HEATER_BREAKER_ON: [Device.CONTROLLING, &"water_heater_breaker", true],
    Hint.WATER_HEATER_BREAKER_OFF: [Device.CONTROLLING, &"water_heater_breaker", false],
    Hint.WATER_CIRCUITS_LINK_ON: [Device.CONTROLLING, &"water_circuits_link", true],
    Hint.WATER_CIRCUITS_LINK_OFF: [Device.CONTROLLING, &"water_circuits_link", false],
    Hint.SPRING_BRAKE_ON: [Device.OCCUPIED, &"set_spring_brake_active", true],
    Hint.CONSIST_HEATING_ON: [Device.OCCUPIED, &"heating", true],
    Hint.CONSIST_HEATING_OFF: [Device.OCCUPIED, &"heating", false],
    Hint.CONSIST_DOOR_LOCKS_ON: [Device.OCCUPIED, &"doors_lock", true],
    Hint.DEPARTURE_SIGNAL_ON: [Device.OCCUPIED, &"doors_departure_signal", true],
    Hint.DEPARTURE_SIGNAL_OFF: [Device.OCCUPIED, &"doors_departure_signal", false],
    Hint.CAB_DEACTIVATION: [Device.OCCUPIED, &"cab_activation", false],
    Hint.MAIN_CIRCUIT_GROUND_RESET: [Device.OCCUPIED, &"ground_relay_reset", null],
    Hint.SPRING_BRAKE_OFF: [Device.OCCUPIED, &"set_spring_brake_active", false],
    Hint.SANDING_ON: [Device.CONTROLLING, &"sand", true],
    Hint.SANDING_OFF: [Device.CONTROLLING, &"sand", false],
    Hint.ANTISLIP: [Device.CONTROLLING, &"antislip", null],
    Hint.DOOR_RIGHT_OPEN: [Device.OCCUPIED, &"doors_right", true],
    Hint.DOOR_RIGHT_CLOSE: [Device.OCCUPIED, &"doors_right", false],
    Hint.DOOR_LEFT_OPEN: [Device.OCCUPIED, &"doors_left", true],
    Hint.DOOR_LEFT_CLOSE: [Device.OCCUPIED, &"doors_left", false],
    Hint.DOOR_RIGHT_PERMIT_ON: [Device.OCCUPIED, &"doors_right_permit", true],
    Hint.DOOR_RIGHT_PERMIT_OFF: [Device.OCCUPIED, &"doors_right_permit", false],
    Hint.DOOR_LEFT_PERMIT_ON: [Device.OCCUPIED, &"doors_left_permit", true],
    Hint.DOOR_LEFT_PERMIT_OFF: [Device.OCCUPIED, &"doors_left_permit", false],
}
## The doors a conductor works open and close for a player too (driverhints.cpp:1056-1093)
const DOOR_OPERATIONS:Array[Hint] = [Hint.DOOR_RIGHT_OPEN, Hint.DOOR_RIGHT_CLOSE, Hint.DOOR_LEFT_OPEN, Hint.DOOR_LEFT_CLOSE]
## The traction motors' blowers at an end: the hint and the end's commands, switched on and its
## "off" switch let go (MotorBlowersSwitchOff(false), MotorBlowersSwitch(true), driverhints.cpp:416-441)
const MOTOR_BLOWERS:Dictionary = {
    Hint.FRONT_MOTOR_BLOWERS_ON: [RailVehicleController.COUPLER_END_FRONT, &"motor_blowers_front", &"motor_blowers_front_switch_off"],
    Hint.REAR_MOTOR_BLOWERS_ON: [RailVehicleController.COUPLER_END_REAR, &"motor_blowers_rear", &"motor_blowers_rear_switch_off"],
}

## The control a hint is about and the gesture of the player's hand that does it - what the hints
## window shows its key by (MaszynaCabinLogic.get_action()). A hint whose step goes either way
## (DIRECTION_NONE), is let go (a horn's, a releaser's end) or has no
## key of its own (sanding, heating, the headcodes) is not here
const CONTROLS:Dictionary = {
    Hint.BATTERY_ON: [&"battery_sw", CabinLogic.Gesture.PRESS],
    Hint.BATTERY_OFF: [&"battery_sw", CabinLogic.Gesture.PRESS],
    Hint.RADIO_ON: [&"radio_sw", CabinLogic.Gesture.PRESS],
    Hint.RADIO_OFF: [&"radio_sw", CabinLogic.Gesture.PRESS],
    Hint.OIL_PUMP_ON: [&"oilpump_sw", CabinLogic.Gesture.PRESS],
    Hint.OIL_PUMP_OFF: [&"oilpump_sw", CabinLogic.Gesture.PRESS],
    Hint.FUEL_PUMP_ON: [&"fuelpump_sw", CabinLogic.Gesture.PRESS],
    Hint.FUEL_PUMP_OFF: [&"fuelpump_sw", CabinLogic.Gesture.PRESS],
    Hint.PANTOGRAPH_AIR_SOURCE_SET_MAIN: [&"pantcompressorvalve_sw", CabinLogic.Gesture.PRESS],
    Hint.PANTOGRAPH_AIR_SOURCE_SET_AUXILIARY: [&"pantcompressorvalve_sw", CabinLogic.Gesture.PRESS],
    Hint.PANTOGRAPH_COMPRESSOR_ON: [&"pantcompressor_sw", CabinLogic.Gesture.PRESS],
    Hint.FRONT_PANTOGRAPH_VALVE_ON: [&"pantfront_sw", CabinLogic.Gesture.PRESS],
    Hint.FRONT_PANTOGRAPH_VALVE_OFF: [&"pantfront_sw", CabinLogic.Gesture.PRESS],
    Hint.REAR_PANTOGRAPH_VALVE_ON: [&"pantrear_sw", CabinLogic.Gesture.PRESS],
    Hint.REAR_PANTOGRAPH_VALVE_OFF: [&"pantrear_sw", CabinLogic.Gesture.PRESS],
    Hint.CONVERTER_ON: [&"converter_sw", CabinLogic.Gesture.PRESS],
    Hint.CONVERTER_OFF: [&"converter_sw", CabinLogic.Gesture.PRESS],
    Hint.PRIMARY_CONVERTER_OVERLOAD_RESET: [&"converterfuse_bt", CabinLogic.Gesture.PRESS],
    Hint.TRACTION_MOTOR_OVERLOAD_RESET: [&"fuse_bt", CabinLogic.Gesture.PRESS],
    Hint.LINE_BREAKER_CLOSE: [MaszynaCabinMainSwitch.KEY, CabinLogic.Gesture.PRESS],
    Hint.LINE_BREAKER_OPEN: [MaszynaCabinMainSwitch.KEY, CabinLogic.Gesture.PRESS],
    Hint.COMPRESSOR_ON: [&"compressor_sw", CabinLogic.Gesture.PRESS],
    Hint.COMPRESSOR_OFF: [&"compressor_sw", CabinLogic.Gesture.PRESS],
    Hint.FRONT_MOTOR_BLOWERS_ON: [&"motorblowersfront_sw", CabinLogic.Gesture.PRESS],
    Hint.FRONT_MOTOR_BLOWERS_OFF: [&"motorblowersfront_sw", CabinLogic.Gesture.PRESS],
    Hint.REAR_MOTOR_BLOWERS_ON: [&"motorblowersrear_sw", CabinLogic.Gesture.PRESS],
    Hint.REAR_MOTOR_BLOWERS_OFF: [&"motorblowersrear_sw", CabinLogic.Gesture.PRESS],
    Hint.SPRING_BRAKE_ON: [&"springbraketoggle_bt", CabinLogic.Gesture.PRESS],
    Hint.SPRING_BRAKE_OFF: [&"springbraketoggle_bt", CabinLogic.Gesture.PRESS],
    Hint.MANUAL_BRAKE_ON: [MaszynaCabinManualBrake.CONTROL, CabinLogic.Gesture.INCREASE],
    Hint.MANUAL_BRAKE_OFF: [MaszynaCabinManualBrake.CONTROL, CabinLogic.Gesture.DECREASE],
    Hint.MASTER_CONTROLLER_SET_SERIES_MODE: [MASTER_CONTROLLER, CabinLogic.Gesture.DECREASE],
    # only up: the original steps up while the position has no clutch in (driverhints.cpp:491-494)
    Hint.MASTER_CONTROLLER_SET_IDLE: [MASTER_CONTROLLER, CabinLogic.Gesture.INCREASE],
    Hint.WATER_HEATER_ON: [&"waterheater_sw", CabinLogic.Gesture.PRESS],
    Hint.WATER_HEATER_OFF: [&"waterheater_sw", CabinLogic.Gesture.PRESS],
    Hint.WATER_HEATER_BREAKER_ON: [&"waterheaterbreaker_sw", CabinLogic.Gesture.PRESS],
    Hint.WATER_HEATER_BREAKER_OFF: [&"waterheaterbreaker_sw", CabinLogic.Gesture.PRESS],
    Hint.WATER_PUMP_ON: [&"waterpump_sw", CabinLogic.Gesture.PRESS],
    Hint.WATER_PUMP_OFF: [&"waterpump_sw", CabinLogic.Gesture.PRESS],
    Hint.WATER_PUMP_BREAKER_ON: [&"waterpumpbreaker_sw", CabinLogic.Gesture.PRESS],
    Hint.WATER_PUMP_BREAKER_OFF: [&"waterpumpbreaker_sw", CabinLogic.Gesture.PRESS],
    Hint.WATER_CIRCUITS_LINK_ON: [&"watercircuitslink_sw", CabinLogic.Gesture.PRESS],
    Hint.WATER_CIRCUITS_LINK_OFF: [&"watercircuitslink_sw", CabinLogic.Gesture.PRESS],
    Hint.MASTER_CONTROLLER_SET_ZERO_SPEED: [MASTER_CONTROLLER, CabinLogic.Gesture.DECREASE],
    Hint.MASTER_CONTROLLER_SET_REVERSER_UNLOCK: [MASTER_CONTROLLER, CabinLogic.Gesture.DECREASE],
    Hint.TRAIN_BRAKE_RELEASE: [TRAIN_BRAKE_RELEASE, CabinLogic.Gesture.PRESS],
    Hint.TRAIN_BRAKE_APPLY: [TRAIN_BRAKE, CabinLogic.Gesture.INCREASE],
    Hint.DIRECTION_FORWARD: [REVERSER, CabinLogic.Gesture.INCREASE],
    Hint.DIRECTION_BACKWARD: [REVERSER, CabinLogic.Gesture.DECREASE],
    Hint.CONSIST_DOOR_LOCKS_ON: [&"door_signalling_sw", CabinLogic.Gesture.PRESS],
    Hint.DEPARTURE_SIGNAL_ON: [&"departure_signal_bt", CabinLogic.Gesture.PRESS],
    Hint.DOOR_RIGHT_OPEN: [&"door_right_sw", CabinLogic.Gesture.PRESS],
    Hint.DOOR_RIGHT_CLOSE: [&"door_right_sw", CabinLogic.Gesture.PRESS],
    Hint.DOOR_LEFT_OPEN: [&"door_left_sw", CabinLogic.Gesture.PRESS],
    Hint.DOOR_LEFT_CLOSE: [&"door_left_sw", CabinLogic.Gesture.PRESS],
    # a permit is revoked by closing the doors (Mover.cpp:8745-8749) - a push permit button only
    # grants (Train.cpp:7213-7220), so the revoking hints have no key; but the preset switch's,
    # where the vehicle has one (get_list())
    Hint.DOOR_RIGHT_PERMIT_ON: [&"doorrightpermit_sw", CabinLogic.Gesture.PRESS],
    Hint.DOOR_LEFT_PERMIT_ON: [&"doorleftpermit_sw", CabinLogic.Gesture.PRESS],
    Hint.HORN_ON: [&"hornlow_bt", CabinLogic.Gesture.PRESS],
    Hint.CONSIST_HEATING_ON: [&"trainheating_sw", CabinLogic.Gesture.PRESS],
    Hint.CONSIST_HEATING_OFF: [&"trainheating_sw", CabinLogic.Gesture.PRESS],
    Hint.SECURITY_SYSTEM_RESET: [SECURITY_RESET, CabinLogic.Gesture.PRESS],
    Hint.SHP_SYSTEM_RESET: [CABSIGNAL_RESET, CabinLogic.Gesture.PRESS],
    Hint.SECOND_CONTROLLER_SET_ZERO: [SECOND_CONTROLLER, CabinLogic.Gesture.DECREASE],
    Hint.TRACTIVE_FORCE_DECREASE: [MASTER_CONTROLLER, CabinLogic.Gesture.DECREASE],
    Hint.TRACTIVE_FORCE_INCREASE: [MASTER_CONTROLLER, CabinLogic.Gesture.INCREASE],
    Hint.BRAKING_FORCE_DECREASE: [TRAIN_BRAKE, CabinLogic.Gesture.DECREASE],
    Hint.BRAKING_FORCE_INCREASE: [TRAIN_BRAKE, CabinLogic.Gesture.INCREASE],
    Hint.BRAKING_FORCE_SET_ZERO: [TRAIN_BRAKE_RELEASE, CabinLogic.Gesture.PRESS],
    Hint.INDEPENDENT_BRAKE_APPLY: [INDEPENDENT_BRAKE, CabinLogic.Gesture.INCREASE],
    Hint.INDEPENDENT_BRAKE_RELEASE: [INDEPENDENT_BRAKE, CabinLogic.Gesture.DECREASE],
    Hint.RELEASER_ON: [RELEASER, CabinLogic.Gesture.PRESS],
    Hint.BUFFERS_COMPRESS: [MASTER_CONTROLLER, CabinLogic.Gesture.INCREASE],
    Hint.CAB_ACTIVATION: [&"cabactivation_sw", CabinLogic.Gesture.PRESS],
    Hint.CAB_DEACTIVATION: [&"cabactivation_sw", CabinLogic.Gesture.PRESS],
}

## The hints that rule each other out (remove_train_brake_hints(), remove_master_controller_hints(),
## remove_reverser_hints(), driverhints.cpp:46-76)
const TRAIN_BRAKE_HINTS:Array[Hint] = [
    Hint.TRAIN_BRAKE_SET_PIPE_UNLOCK, Hint.TRAIN_BRAKE_RELEASE, Hint.TRAIN_BRAKE_APPLY, Hint.BRAKING_FORCE_DECREASE,
    Hint.BRAKING_FORCE_INCREASE, Hint.BRAKING_FORCE_SET_ZERO, Hint.BRAKING_FORCE_LAP,
]
const MASTER_CONTROLLER_HINTS:Array[Hint] = [
    Hint.MASTER_CONTROLLER_SET_IDLE, Hint.MASTER_CONTROLLER_SET_SERIES_MODE, Hint.MASTER_CONTROLLER_SET_ZERO_SPEED,
    Hint.MASTER_CONTROLLER_SET_REVERSER_UNLOCK, Hint.TRACTIVE_FORCE_DECREASE, Hint.TRACTIVE_FORCE_INCREASE,
    Hint.BUFFERS_COMPRESS,
]
## The reverser hint of the vehicle's one way and the other - what a rear cab shows instead
const REVERSER_SIDES:Dictionary[Hint, Hint] = {
    Hint.DIRECTION_FORWARD: Hint.DIRECTION_BACKWARD,
    Hint.DIRECTION_BACKWARD: Hint.DIRECTION_FORWARD,
}
const REVERSER_HINTS:Array[Hint] = [Hint.DIRECTION_FORWARD, Hint.DIRECTION_BACKWARD, Hint.DIRECTION_OTHER, Hint.DIRECTION_NONE]
## The group of hints a hint's cue takes away - the others of it, the hint keeping its place
const GROUPS:Dictionary = {
    Hint.MASTER_CONTROLLER_SET_IDLE: MASTER_CONTROLLER_HINTS,
    Hint.MASTER_CONTROLLER_SET_SERIES_MODE: MASTER_CONTROLLER_HINTS,
    Hint.MASTER_CONTROLLER_SET_ZERO_SPEED: MASTER_CONTROLLER_HINTS,
    Hint.MASTER_CONTROLLER_SET_REVERSER_UNLOCK: MASTER_CONTROLLER_HINTS,
    Hint.SECOND_CONTROLLER_SET_ZERO: MASTER_CONTROLLER_HINTS,
    Hint.TRACTIVE_FORCE_DECREASE: MASTER_CONTROLLER_HINTS,
    Hint.TRACTIVE_FORCE_INCREASE: MASTER_CONTROLLER_HINTS,
    Hint.BUFFERS_COMPRESS: MASTER_CONTROLLER_HINTS,
    Hint.TRAIN_BRAKE_SET_PIPE_UNLOCK: TRAIN_BRAKE_HINTS,
    Hint.TRAIN_BRAKE_RELEASE: TRAIN_BRAKE_HINTS,
    Hint.TRAIN_BRAKE_APPLY: TRAIN_BRAKE_HINTS,
    Hint.BRAKING_FORCE_DECREASE: TRAIN_BRAKE_HINTS,
    Hint.BRAKING_FORCE_INCREASE: TRAIN_BRAKE_HINTS,
    Hint.BRAKING_FORCE_SET_ZERO: TRAIN_BRAKE_HINTS,
    Hint.BRAKING_FORCE_LAP: TRAIN_BRAKE_HINTS,
    Hint.DIRECTION_FORWARD: REVERSER_HINTS,
    Hint.DIRECTION_BACKWARD: REVERSER_HINTS,
    Hint.DIRECTION_OTHER: REVERSER_HINTS,
    Hint.DIRECTION_NONE: REVERSER_HINTS,
}
## The other hints a hint's cue takes away. Quirk kept: the auxiliary air source takes away the
## pantograph compressor's hint, not the main air source's (driverhints.cpp:229)
const OPPOSITES:Dictionary = {
    Hint.BATTERY_ON: [Hint.BATTERY_OFF],
    Hint.BATTERY_OFF: [Hint.BATTERY_ON],
    Hint.CAB_ACTIVATION: [Hint.CAB_DEACTIVATION, Hint.CAB_DEACTIVATION_OTHER_END],
    Hint.CAB_DEACTIVATION: [Hint.CAB_ACTIVATION, Hint.CAB_DEACTIVATION_OTHER_END],
    Hint.CAB_DEACTIVATION_OTHER_END: [Hint.CAB_ACTIVATION, Hint.CAB_DEACTIVATION],
    Hint.RADIO_ON: [Hint.RADIO_OFF],
    Hint.RADIO_OFF: [Hint.RADIO_ON],
    Hint.OIL_PUMP_ON: [Hint.OIL_PUMP_OFF],
    Hint.OIL_PUMP_OFF: [Hint.OIL_PUMP_ON],
    Hint.FUEL_PUMP_ON: [Hint.FUEL_PUMP_OFF],
    Hint.FUEL_PUMP_OFF: [Hint.FUEL_PUMP_ON],
    Hint.PANTOGRAPH_AIR_SOURCE_SET_MAIN: [Hint.PANTOGRAPH_AIR_SOURCE_SET_AUXILIARY],
    Hint.PANTOGRAPH_AIR_SOURCE_SET_AUXILIARY: [Hint.PANTOGRAPH_COMPRESSOR_ON],
    Hint.PANTOGRAPH_COMPRESSOR_ON: [Hint.PANTOGRAPH_COMPRESSOR_OFF],
    Hint.PANTOGRAPH_COMPRESSOR_OFF: [Hint.PANTOGRAPH_COMPRESSOR_ON],
    Hint.PANTOGRAPHS_VALVE_ON: [Hint.PANTOGRAPHS_VALVE_OFF],
    Hint.FRONT_PANTOGRAPH_VALVE_ON: [Hint.FRONT_PANTOGRAPH_VALVE_OFF],
    Hint.FRONT_PANTOGRAPH_VALVE_OFF: [Hint.FRONT_PANTOGRAPH_VALVE_ON],
    Hint.REAR_PANTOGRAPH_VALVE_ON: [Hint.REAR_PANTOGRAPH_VALVE_OFF],
    Hint.REAR_PANTOGRAPH_VALVE_OFF: [Hint.REAR_PANTOGRAPH_VALVE_ON],
    Hint.CONVERTER_ON: [Hint.CONVERTER_OFF],
    Hint.CONVERTER_OFF: [Hint.CONVERTER_ON],
    Hint.LINE_BREAKER_CLOSE: [Hint.LINE_BREAKER_OPEN],
    Hint.LINE_BREAKER_OPEN: [Hint.LINE_BREAKER_CLOSE],
    Hint.COMPRESSOR_ON: [Hint.COMPRESSOR_OFF],
    Hint.COMPRESSOR_OFF: [Hint.COMPRESSOR_ON],
    Hint.FRONT_MOTOR_BLOWERS_ON: [Hint.FRONT_MOTOR_BLOWERS_OFF],
    Hint.REAR_MOTOR_BLOWERS_ON: [Hint.REAR_MOTOR_BLOWERS_OFF],
    Hint.SPRING_BRAKE_ON: [Hint.SPRING_BRAKE_OFF],
    Hint.SPRING_BRAKE_OFF: [Hint.SPRING_BRAKE_ON],
    Hint.MANUAL_BRAKE_ON: [Hint.MANUAL_BRAKE_OFF],
    Hint.MANUAL_BRAKE_OFF: [Hint.MANUAL_BRAKE_ON],
    Hint.WATER_PUMP_ON: [Hint.WATER_PUMP_OFF],
    Hint.WATER_PUMP_OFF: [Hint.WATER_PUMP_ON],
    Hint.WATER_PUMP_BREAKER_ON: [Hint.WATER_PUMP_BREAKER_OFF],
    Hint.WATER_PUMP_BREAKER_OFF: [Hint.WATER_PUMP_BREAKER_ON],
    Hint.WATER_HEATER_ON: [Hint.WATER_HEATER_OFF],
    Hint.WATER_HEATER_OFF: [Hint.WATER_HEATER_ON],
    Hint.WATER_HEATER_BREAKER_ON: [Hint.WATER_HEATER_BREAKER_OFF],
    Hint.WATER_HEATER_BREAKER_OFF: [Hint.WATER_HEATER_BREAKER_ON],
    Hint.WATER_CIRCUITS_LINK_ON: [Hint.WATER_CIRCUITS_LINK_OFF],
    Hint.WATER_CIRCUITS_LINK_OFF: [Hint.WATER_CIRCUITS_LINK_ON],
    # the tractive force taken up clears what braked it (driverhints.cpp:571-576)
    Hint.TRACTIVE_FORCE_INCREASE: [Hint.BRAKING_FORCE_INCREASE, Hint.INDEPENDENT_BRAKE_APPLY],
    Hint.INDEPENDENT_BRAKE_APPLY: [Hint.INDEPENDENT_BRAKE_RELEASE],
    Hint.INDEPENDENT_BRAKE_RELEASE: [Hint.INDEPENDENT_BRAKE_APPLY],
    Hint.SANDING_ON: [Hint.SANDING_OFF],
    Hint.SANDING_OFF: [Hint.SANDING_ON],
    Hint.HORN_ON: [Hint.HORN_OFF],
    Hint.DEPARTURE_SIGNAL_ON: [Hint.DEPARTURE_SIGNAL_OFF],
    Hint.DEPARTURE_SIGNAL_OFF: [Hint.DEPARTURE_SIGNAL_ON],
    Hint.DOOR_RIGHT_OPEN: [Hint.DOOR_RIGHT_CLOSE],
    Hint.DOOR_RIGHT_CLOSE: [Hint.DOOR_RIGHT_OPEN],
    Hint.DOOR_LEFT_OPEN: [Hint.DOOR_LEFT_CLOSE],
    Hint.DOOR_LEFT_CLOSE: [Hint.DOOR_LEFT_OPEN],
    Hint.DOOR_RIGHT_PERMIT_ON: [Hint.DOOR_RIGHT_PERMIT_OFF],
    Hint.DOOR_RIGHT_PERMIT_OFF: [Hint.DOOR_RIGHT_PERMIT_ON],
    Hint.DOOR_LEFT_PERMIT_ON: [Hint.DOOR_LEFT_PERMIT_OFF],
    Hint.DOOR_LEFT_PERMIT_OFF: [Hint.DOOR_LEFT_PERMIT_ON],
    Hint.CONSIST_LIGHTS_ON: [Hint.CONSIST_LIGHTS_OFF],
    Hint.CONSIST_LIGHTS_OFF: [Hint.CONSIST_LIGHTS_ON],
    Hint.CONSIST_HEATING_ON: [Hint.CONSIST_HEATING_OFF],
    Hint.CONSIST_HEATING_OFF: [Hint.CONSIST_HEATING_ON],
    Hint.HEADCODE_PC1: [Hint.HEADCODE_PC2, Hint.HEADCODE_TB1, Hint.LIGHTS_OFF],
    Hint.HEADCODE_PC2: [Hint.HEADCODE_PC1, Hint.HEADCODE_TB1, Hint.LIGHTS_OFF],
    Hint.HEADCODE_TB1: [Hint.HEADCODE_PC1, Hint.HEADCODE_PC2, Hint.HEADCODE_PC5, Hint.LIGHTS_OFF],
    Hint.HEADCODE_PC5: [Hint.HEADCODE_TB1, Hint.LIGHTS_OFF],
    Hint.LIGHTS_OFF: [Hint.HEADCODE_PC1, Hint.HEADCODE_PC2, Hint.HEADCODE_PC5, Hint.HEADCODE_TB1],
    Hint.RELEASER_ON: [Hint.RELEASER_OFF],
    Hint.RELEASER_OFF: [Hint.RELEASER_ON],
}
## Hints the original takes but never keeps: the lapped brake and the horn off (driverhints.cpp:874,
## 1005); the horn's own hint goes by itself
const UNKEPT:Array[Hint] = [Hint.BRAKING_FORCE_LAP, Hint.HORN_OFF]

const LINE_BREAKER_CLOSE:StringName = MaszynaCabinMainSwitch.ON_BUTTON
const LINE_BREAKER_OPEN:StringName = MaszynaCabinMainSwitch.OFF_BUTTON
const MASTER_CONTROLLER:StringName = &"mainctrl"
const SECOND_CONTROLLER:StringName = &"scndctrl"
const REVERSER:StringName = &"dirkey"
const TRAIN_BRAKE_RELEASE:StringName = MaszynaCabinControls.BRAKE_LEVEL_DRIVE
const SECURITY_RESET:StringName = &"security_reset_bt"
const CABSIGNAL_RESET:StringName = &"shp_reset_bt"
const RELEASER:StringName = &"releaser_bt"
const TRAIN_BRAKE:StringName = &"brakectrl"
## The light selector (lights_sw, Train.cpp:5220) and the hints it lights
const LIGHT_SELECTOR:StringName = &"lights_sw"
const LIGHT_CODES:Array[Hint] = [Hint.HEADCODE_PC1, Hint.HEADCODE_PC2, Hint.HEADCODE_PC5, Hint.HEADCODE_TB1, Hint.LIGHTS_OFF]
## The door permit preset switch (Train.cpp:11853) and its preset of no doors (DoorPermitList=)
const DOOR_PERMIT_PRESET:StringName = &"doorpermitpreset_sw"
const NO_DOORS_PERMITTED:int = 0
## The universal brake buttons' cab controls, by their number 1-3 (Train.cpp:11838)
const UNIVERSAL_BRAKE_BUTTON:String = "universalbrake%d_bt"
## Releasing an EP brake: the handle down to its EP releasing position, below the driving one,
## which only holds the EP brake applied (TFVel6 bh_EPN = bh_RP, hamulce.cpp:34)
const EP_BRAKE_RELEASE_CONTROL:Array = [TRAIN_BRAKE, CabinLogic.Gesture.DECREASE]
const INDEPENDENT_BRAKE:StringName = &"localbrake"

## ManualBrakePosNo (MOVER.h:111): the hand brake's wheel turned all the way
const MANUAL_BRAKE_POSITIONS:int = 20
## HandleUnlock with no position the pipe is unlocked at (HandlePipeUnlockPos= absent, Mover.cpp:10507)
const PIPE_UNLOCK_NONE:float = -3.0
## The light level the compartment lights are wanted under, and kept on under (driverhints.cpp:1158,
## 1171; control_compartment_lights(), Driver.cpp:6530-6535)
const COMPARTMENT_LIGHTS_ON_LEVEL:float = 0.35
const COMPARTMENT_LIGHTS_OFF_LEVEL:float = 0.40
## The horn's hint lasts this much longer than the horn is to sound [s], and goes once the time is
## this close to gone (fWarningDuration + 5.0 < 0.05, driverhints.cpp:1002)
const HORN_HINT_EXTRA_TIME:float = 5.0
const HORN_HINT_END:float = 0.05
## WarningSignal's horns (MaszynaMoverRailVehicleHorns): the low and the high tone
const HORN_LOW:int = 1
const HORN_HIGH:int = 2
## The departure signal no longer matters past this speed [km/h] (driverhints.cpp:1040)
const DEPARTURE_SIGNAL_MAX_SPEED:float = 5.0
## No power comes from an EIM controller at or under this share (IsPower, Mover.cpp:7273)
const NO_POWER_EIMIC:float = 0.005
## The train brake handle counts as at a position within this (is_equal(..., 0.2),
## driverhints.cpp:805, 856, 868)
const HANDLE_TOLERANCE:float = 0.2
## A handle held by time counts as running once its control pressure is this close under the
## pipe's highest (HighPipePress - 0.05, driverhints.cpp:806)
const HANDLE_CONTROL_MARGIN:float = 0.05
## The share of the tractive or braking force a decrease is to take off, and what it takes at least
## [N] (driverhints.cpp:557-561, 834); an increase is to add BRAKING_INCREASE_SHARE or at least
## BRAKING_INCREASE_MIN [N] (driverhints.cpp:851-855)
const DECREASE_SHARE:float = 0.95
const DECREASE_MIN:float = 5.0
const BRAKING_INCREASE_SHARE:float = 1.05
const BRAKING_INCREASE_MIN:float = 5.0
## The acceleration still missing that keeps the tractive force's hint [m/s2] (driverhints.cpp:582)
const TRACTIVE_ACCELERATION_MARGIN:float = 0.05
## An EIM controller asks for its full power at this (eimic_real >= 1.0, driverhints.cpp:583)
const EIM_FULL_POWER:float = 1.0
## Pressing the buffers is done once the force is over this [N] (driverhints.cpp:591)
const PRESSED_BUFFERS_FORCE:float = 30.0
## The train's brakes released: no cylinder over this [bar] (fReady < 0.4, driverhints.cpp:868)
const RELEASED_BRAKE_PRESSURE:float = 0.4
## The local brake counts as released under this share (LocalBrakePosA < 0.05, driverhints.cpp:908)
const RELEASED_LOCAL_BRAKE:float = 0.05
## The main reservoir counts as full over this [bar] (ScndPipePress > 4.5, driverhints.cpp:744)
const FULL_MAIN_RESERVOIR:float = 4.5
## The pantographs' tank counts as full at this [bar], an EMU's at EMU_FULL_PANTOGRAPH_TANK
## (driverhints.cpp:752; PantPressLockActive's 4.6 of an EMU is not published - TODO.md)
const FULL_PANTOGRAPH_TANK:float = 4.2
const EMU_FULL_PANTOGRAPH_TANK:float = 2.6


## One hint in the driver's list: what it is and its parameter - the speed over which it no
## longer matters, a position or a channel to show, the force to reach
class Queued:
    var hint:Hint
    var parameter:float

    func _init(p_hint:Hint, p_parameter:float) -> void:
        hint = p_hint
        parameter = p_parameter


## A vehicle command the driver gives, sent only when the vehicle has it; null when it has not.
## The original's driver calls the Mover, where a device the vehicle lacks does nothing
## (Sandbox() switches nothing on without sand, Mover.cpp:3070); here the component that registers
## the command is missing (an EN96 has no RailVehicleSwitches, so no "sand").
static func send(vehicle:RID, command:StringName, p1:Variant = null, p2:Variant = null) -> Variant:
    if not VehicleServer.vehicle_has_command(vehicle, command):
        return null
    return VehicleServer.vehicle_send_command(vehicle, command, p1, p2)


## cue_action() (driverhints.cpp:79-1350): a driver the computer is takes the step - by `action`
## when the step is the driver's own traction's or braking's, else by the hint's control or
## command - and the step is kept in the driver's list until the vehicle shows it done, the hints
## it rules out taken away. `parameter`: the original's Actionparameter. True when it is done.
## A switch, a button or a command is given only while the step is not done: the original sets the
## Mover's value again on every cue, here a command would renew the vehicle's state for nothing.
static func cue(situation:MaszynaDriverTraction.Situation, hint:Hint, parameter:float = 0.0,
        action:Callable = Callable()) -> bool:
    var vehicle:RID = situation.vehicle
    var cabin:RID = situation.cabin
    var acting:bool = DriverServer.vehicle_is_control_active(vehicle)
    if hint in DOOR_OPERATIONS:
        var doors:RailVehicleDoors = VehicleServer.vehicle_component_get(
                vehicle, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
        acting = acting or (doors != null and doors.open_method == RailVehicleDoors.CONTROLS_CONDUCTOR)
    if acting and action.is_valid():
        action.call()
    elif acting:
        # the device where the step wants it - the hint's own check, but for the checks that look
        # past the device: a pantograph to raise wanted whatever the speed, the oil pump whatever
        # the engine, the sand whatever the wheels, the battery whatever feeds the low voltage
        var done:bool = is_done(situation, hint, 0.0 if hint in PANTOGRAPH_VALVES else parameter)
        match hint:
            Hint.BATTERY_OFF:
                var power_supply:RailVehiclePowerSupply = _power_supply(vehicle)
                done = power_supply == null or not power_supply.get_battery_enabled()
            Hint.OIL_PUMP_ON:
                var diesel:RailVehicleDieselEngine = _diesel(vehicle)
                done = diesel == null or diesel.get_oil_pump_enabled()
            Hint.SANDING_ON:
                var switches:RailVehicleSwitches = _switches(situation.controlling)
                done = switches == null or switches.get_sand_active()
            Hint.WATER_PUMP_ON:
                var diesel:RailVehicleDieselEngine = _diesel(situation.controlling)
                done = diesel == null or diesel.get_water_pump_enabled()
            Hint.WATER_CIRCUITS_LINK_ON, Hint.WATER_CIRCUITS_LINK_OFF:
                var diesel:RailVehicleDieselEngine = _diesel(situation.controlling)
                done = diesel == null or diesel.get_water_circuits_link() == (hint == Hint.WATER_CIRCUITS_LINK_ON)
            Hint.CONSIST_LIGHTS_ON, Hint.CONSIST_LIGHTS_OFF:
                done = (VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController) \
                        .get_compartment_lights_enabled() == (hint == Hint.CONSIST_LIGHTS_ON)
            Hint.DEPARTURE_SIGNAL_ON:
                var doors:RailVehicleDoors = _doors(vehicle)
                done = doors == null or doors.get_departure_signal()
        if not done and SWITCHES.has(hint):
            # a cab activated only with the low voltage on (driverhints.cpp:107)
            if not hint == Hint.CAB_ACTIVATION or _power24_available(vehicle):
                CabinSystem.act(cabin, SWITCHES[hint][0], &"toggle", SWITCHES[hint][1])
        elif not done and BUTTONS.has(hint):
            CabinSystem.act(cabin, BUTTONS[hint], &"hold")
            CabinSystem.act(cabin, BUTTONS[hint], &"release")
        elif not done and PANTOGRAPH_VALVES.has(hint):
            send(situation.trainset.pantograph_unit, &"pantograph_valve_operate",
                    PANTOGRAPH_VALVES[hint][0], PANTOGRAPH_VALVES[hint][1])
        elif not done and COMMANDS.has(hint):
            send(device(situation, COMMANDS[hint][0]), COMMANDS[hint][1], COMMANDS[hint][2])
        match hint:
            Hint.FRONT_MOTOR_BLOWERS_ON, Hint.REAR_MOTOR_BLOWERS_ON:
                if not done:
                    send(vehicle, MOTOR_BLOWERS[hint][2], false)
                    send(vehicle, MOTOR_BLOWERS[hint][1], true)
            # the hand brake's wheel turned all the way (IncManualBrakeLevel(ManualBrakePosNo),
            # driverhints.cpp:466-487)
            Hint.MANUAL_BRAKE_ON, Hint.MANUAL_BRAKE_OFF:
                if not done:
                    for _turn:int in MANUAL_BRAKE_POSITIONS:
                        send(vehicle, &"manual_brake_increase" if hint == Hint.MANUAL_BRAKE_ON else &"manual_brake_decrease")
            # the switch and its "off" switch (driverhints.cpp:1148-1173)
            Hint.CONSIST_LIGHTS_ON, Hint.CONSIST_LIGHTS_OFF:
                if not done:
                    var on:bool = hint == Hint.CONSIST_LIGHTS_ON
                    send(vehicle, &"compartment_lights", on)
                    send(vehicle, &"compartment_lights_switch_off", not on)
            # the horns of WarningSignal's bits (driverhints.cpp:993-1004)
            Hint.HORN_ON:
                send(vehicle, &"horn_low", bool(int(parameter) & HORN_LOW))
                send(vehicle, &"horn_high", bool(int(parameter) & HORN_HIGH))
            Hint.HORN_OFF:
                send(vehicle, &"horn_low", false)
                send(vehicle, &"horn_high", false)
            # main_on_bt held down on one update and let go on the next - the cab closes the breaker
            # after InitialCtrlDelay of holding, or on the release (MaszynaCabinMainSwitch), and a
            # driver's update comes after its reaction time, longer than the delay (PrepareTime,
            # Driver.cpp:158)
            Hint.LINE_BREAKER_CLOSE:
                if CabinSystem.get_control(cabin, LINE_BREAKER_CLOSE):
                    CabinSystem.act(cabin, LINE_BREAKER_CLOSE, &"release")
                elif not _main_switch_enabled(vehicle):
                    CabinSystem.act(cabin, LINE_BREAKER_CLOSE, &"hold")
            Hint.LINE_BREAKER_OPEN:
                if _main_switch_enabled(vehicle):
                    CabinSystem.act(cabin, LINE_BREAKER_OPEN, &"hold")
                    CabinSystem.act(cabin, LINE_BREAKER_OPEN, &"release")
            # the vigilance's button pressed while the system flashes - also for a cab signal that
            # has no button of its own (driverhints.cpp:1198-1218)
            Hint.SECURITY_SYSTEM_RESET:
                var security:RailVehicleSecuritySystem = _security(vehicle)
                if security and security.get_blinking():
                    CabinSystem.act(cabin, SECURITY_RESET, &"hold")
                    CabinSystem.act(cabin, SECURITY_RESET, &"release")
            Hint.SHP_SYSTEM_RESET:
                if not done:
                    CabinSystem.act(cabin, CABSIGNAL_RESET, &"hold")
                    CabinSystem.act(cabin, CABSIGNAL_RESET, &"release")
            Hint.TRAIN_BRAKE_APPLY:
                situation.braking.apply_train_brake()
            Hint.INDEPENDENT_BRAKE_RELEASE:
                situation.braking.release_local_brake(vehicle, cabin)
            Hint.RELEASER_ON:
                CabinSystem.act(cabin, RELEASER, &"hold")
            Hint.RELEASER_OFF:
                CabinSystem.act(cabin, RELEASER, &"release")
            Hint.MASTER_CONTROLLER_SET_ZERO_SPEED:
                situation.traction.zero(situation)
            Hint.MASTER_CONTROLLER_SET_SERIES_MODE:
                situation.traction.set_series_mode(situation)
            # a diesel's master controller up to its first position with the clutch in (RList[].Mn),
            # so that it does not stall - SN61's idle (driverhints.cpp:489-501)
            Hint.MASTER_CONTROLLER_SET_IDLE:
                var configuration:MaszynaMoverDieselConfiguration = MaszynaDriverTraction.diesel_configuration(
                        VehicleServer.vehicle_component_get(situation.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine)
                var positions:Array = configuration.throttle_table_positions if configuration else []
                while MaszynaDriverTraction.main_controller_position(situation) < positions.size() \
                        and (positions[MaszynaDriverTraction.main_controller_position(situation)] \
                            as MaszynaMoverThrottlePositionItem).clutch_behavior == MaszynaMoverThrottlePositionItem.CLUTCH_BEHAVIOR_NONE \
                        and MaszynaDriverTraction.step_main(situation, 1):
                    pass
            # the master controller down until the reverser may move (driverhints.cpp:534-547)
            Hint.MASTER_CONTROLLER_SET_REVERSER_UNLOCK:
                var master:RailVehicleMasterController = MaszynaDriverTraction.master_controller(situation.controlling)
                while master and master.get_main_position() > master.direction_change_max_position \
                        and MaszynaDriverTraction.step_main(situation, -1):
                    pass
            # the reverser, relative to the cab, stepped to where it is wanted (OrderDirectionChange(),
            # ZeroDirection(), Driver.cpp:5756-5791); a step the vehicle refuses ends the stepping
            Hint.DIRECTION_FORWARD, Hint.DIRECTION_BACKWARD, Hint.DIRECTION_OTHER, Hint.DIRECTION_NONE:
                var cab:int = _active_cab(vehicle)
                var wanted:int = 0
                match hint:
                    Hint.DIRECTION_FORWARD:
                        wanted = cab
                    Hint.DIRECTION_BACKWARD:
                        wanted = -cab
                    Hint.DIRECTION_OTHER:
                        wanted = situation.state.direction_order * cab
                var controller:VehicleController = VehicleServer.vehicle_get_controller(vehicle)
                var current:int = controller.get_direction()
                while not current == wanted:
                    CabinSystem.act(cabin, REVERSER, &"increase" if wanted > current else &"decrease")
                    var stepped:int = controller.get_direction()
                    if stepped == current:
                        break
                    current = stepped
    var hints:Array[Queued] = situation.state.hints
    var ruled_out:Array = GROUPS.get(hint, []) + OPPOSITES.get(hint, [])
    # the hint's own parameter, where the original works it out (driverhints.cpp:557-561, 834,
    # 851-855, 897, 546)
    var tractive_force:float = absf(_tractive_force(situation.controlling))
    var braking_force:float = absf(_braking_force(situation.controlling))
    match hint:
        # the horn's hint keeps no parameter of its own (driverhints.cpp:998)
        Hint.HORN_ON:
            parameter = 0.0
        # quirk kept: min(0, ...) makes the force to reach 0 - the hint stays until no force is left
        Hint.TRACTIVE_FORCE_DECREASE:
            parameter = minf(0.0, maxf(tractive_force * DECREASE_SHARE, tractive_force - DECREASE_MIN))
        Hint.BRAKING_FORCE_DECREASE:
            parameter = minf(0.0, DECREASE_SHARE * braking_force)
        Hint.BRAKING_FORCE_INCREASE:
            parameter = maxf(0.0, maxf(braking_force * BRAKING_INCREASE_SHARE, braking_force + BRAKING_INCREASE_MIN))
        Hint.INDEPENDENT_BRAKE_APPLY:
            var configuration:MaszynaMoverEngineConfiguration = MaszynaDriverTraction.engine_configuration(
                    VehicleServer.vehicle_component_get(vehicle, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine)
            var positions:float = MaszynaDriverBraking.LOCAL_BRAKE_POSITIONS
            parameter = (positions - (1.0 if configuration and configuration.cntrl_eim_control_emergency else 0.0)) / positions
        Hint.MASTER_CONTROLLER_SET_REVERSER_UNLOCK:
            var master:RailVehicleMasterController = MaszynaDriverTraction.master_controller(situation.controlling)
            parameter = master.direction_change_max_position if master else 0.0
    var unkept:bool = hint in UNKEPT
    if not unkept and is_done(situation, hint, parameter):
        return true
    # the list keeps the order the steps came in, which is the order a player follows it: a hint
    # rules out the others of its group, not itself, and a kept one only when it is listed itself.
    # The original removes the whole group, the hint with it, before its check
    # (remove_master_controller_hints(), remove_reverser_hints(), driverhints.cpp): every update
    # put the idle and the reverser behind the line breaker, which a player following the list
    # closes with an SN61's controller at 0, where its engine does not start
    for index:int in range(hints.size() - 1, -1, -1):
        if hints[index].hint in ruled_out and not hints[index].hint == hint:
            hints.remove_at(index)
    if unkept:
        return true
    for queued:Queued in hints:
        if queued.hint == hint:
            queued.parameter = parameter
            return false
    hints.append(Queued.new(hint, parameter))
    return false


## A hint whose reason has gone leaves the list undone. The original has no such step: a cued hint
## stays until the vehicle shows it done or a hint of its group replaces it (driverhints.cpp:30-35).
static func withdraw(situation:MaszynaDriverTraction.Situation, hint:Hint) -> void:
    var hints:Array[Queued] = situation.state.hints
    for index:int in range(hints.size() - 1, -1, -1):
        if hints[index].hint == hint:
            hints.remove_at(index)


## update_hints() (driverhints.cpp:30-35): the hints the vehicle shows done leave the list
static func update(situation:MaszynaDriverTraction.Situation) -> void:
    var hints:Array[Queued] = situation.state.hints
    for index:int in range(hints.size() - 1, -1, -1):
        if is_done(situation, hints[index].hint, hints[index].parameter):
            hints.remove_at(index)


## The driver's list as it reads now, in the order the hints came: each hint, its text, its
## parameter and whether the vehicle already shows it done (shown green until the next update,
## driveruipanels.cpp:280-294)
static func get_list(situation:MaszynaDriverTraction.Situation) -> Array[Dictionary]:
    var listed:Array[Dictionary] = []
    # the reverser hints count along the vehicle (DirActive * CabActive, driverhints.cpp:921, 932),
    # the reverser and its keys from the cab: from a rear cab the vehicle's backward is the
    # reverser's forward, so the player is shown that one - its words and its key
    var rear_cab:bool = RailVehicleServer.cabin_get_kind(RailVehicleServer.vehicle_get_driver_cabin(situation.vehicle)) \
            == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR
    for queued:Queued in situation.state.hints:
        var shown:Hint = queued.hint
        if rear_cab and shown in REVERSER_SIDES:
            shown = REVERSER_SIDES[shown]
        var control:Array = CONTROLS[shown] if CONTROLS.has(shown) else [&"", CabinLogic.Gesture.PRESS]
        # releasing the train brakes releases the independent one too (DecBrake(),
        # driverhints.cpp:863-871): once the handle stands released, the cylinders the independent
        # brake keeps filled are what is left - its key, not the handle's that no longer moves
        if shown == Hint.BRAKING_FORCE_SET_ZERO:
            var ep:bool = _released_by_ep(situation.vehicle)
            var brake:RailVehicleBrake = _brake(situation.vehicle)
            if brake and brake.get_local_position_normalized() > 0.0 and absf(brake.get_controller_position()
                    - brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_EP_RELEASE if ep
                        else RailVehicleBrake.HANDLE_POSITION_DRIVE)) <= HANDLE_TOLERANCE:
                control = CONTROLS[Hint.INDEPENDENT_BRAKE_RELEASE]
            elif ep:
                control = EP_BRAKE_RELEASE_CONTROL
        # a permit the preset switch gave is taken by it (ChangeDoorPermitPreset(), Mover.cpp:7762-7777):
        # towards the nearest preset of no doors - the switch applies a preset only as it moves, so
        # from that very preset through its neighbour (the permits the cab's deactivation gave,
        # InactiveCabFlag doorpermition, Mover.cpp:2690-2694)
        if shown in [Hint.DOOR_LEFT_PERMIT_OFF, Hint.DOOR_RIGHT_PERMIT_OFF]:
            var doors:RailVehicleDoors = _doors(situation.vehicle)
            if doors and doors.permit_list:
                var current:int = doors.get_permit_preset()
                var target:int = current
                for index:int in doors.permit_list.size():
                    if int(doors.permit_list[index]) == NO_DOORS_PERMITTED \
                            and (target == current or absi(index - current) < absi(target - current)):
                        target = index
                var upwards:bool = target > current or (target == current and current < doors.permit_list.size() - 1)
                control = [DOOR_PERMIT_PRESET, CabinLogic.Gesture.INCREASE if upwards else CabinLogic.Gesture.DECREASE]
        # a lamp code set by the light selector, where the vehicle has one (LightsList): its key
        # towards the preset that shows it - in an active cab: an inactive one shows its red
        # markers whatever the preset (InactiveCabFlag redmarkers, SetLights(), DynObj.cpp:7324)
        if shown in LIGHT_CODES and not _active_cab(situation.vehicle) == 0:
            var preset:int = MaszynaDriverLights.light_preset(situation, shown)
            var lighting:RailVehicleLighting = VehicleServer.vehicle_component_get(
                    situation.vehicle, VehicleComponentType.COMPONENT_LIGHTING) as RailVehicleLighting
            if preset >= 0:
                control = [LIGHT_SELECTOR, CabinLogic.Gesture.INCREASE if preset > lighting.get_position() - 1
                        else CabinLogic.Gesture.DECREASE]
        # the universal brake button the vehicle unlocks its main pipe with (UniversalBrakeButtonFlag,
        # ub_UnlockPipe)
        if shown == Hint.MAIN_PIPE_UNLOCK:
            var brake:RailVehicleBrake = _brake(situation.vehicle)
            var buttons:Array[int] = [brake.universal_brake_button_1, brake.universal_brake_button_2,
                    brake.universal_brake_button_3]
            for index:int in buttons.size():
                if buttons[index] & MaszynaDriverBraking.UNIVERSAL_UNLOCK_PIPE:
                    control = [StringName(UNIVERSAL_BRAKE_BUTTON % (index + 1)), CabinLogic.Gesture.PRESS]
                    break
        listed.append({
            "hint": queued.hint,
            "text": TEXTS[shown],
            "parameter": queued.parameter,
            "done": is_done(situation, queued.hint, queued.parameter),
            "control": control[0],
            "gesture": control[1],
        })
    return listed


## A hint's check (the predicates of cue_action(), driverhints.cpp:79-1350): true when the vehicle
## shows the step done, or has nothing it is about
static func is_done(situation:MaszynaDriverTraction.Situation, hint:Hint, parameter:float) -> bool:
    var vehicle:RID = situation.vehicle
    var controlling:RID = situation.controlling
    var unit:RID = situation.trainset.pantograph_unit
    match hint:
        # a battery, a pump that is not switched by hand needs no hint (driverhints.cpp:91, 102,
        # 173, 185, 198, 210)
        Hint.BATTERY_ON, Hint.BATTERY_OFF:
            var power_supply:RailVehiclePowerSupply = _power_supply(vehicle)
            return power_supply == null \
                    or not power_supply.cntrl_battery_start_mode == RailVehicleController.START_MODE_MANUAL \
                    or power_supply.get_power24_available() == (hint == Hint.BATTERY_ON)
        Hint.CAB_ACTIVATION:
            var master:RailVehicleMasterController = MaszynaDriverTraction.master_controller(vehicle)
            return master == null or master.get_cabin_controleable()
        Hint.CAB_DEACTIVATION, Hint.CAB_DEACTIVATION_OTHER_END:
            return _active_cab(vehicle) == 0
        Hint.RADIO_ON, Hint.RADIO_OFF:
            var radio:RailVehicleRadio = VehicleServer.vehicle_component_get(
                    vehicle, VehicleComponentType.COMPONENT_RADIO) as RailVehicleRadio
            return radio == null or radio.get_enabled() == (hint == Hint.RADIO_ON)
        # the cab radio's channel - the original compares the driver's own (iRadioChannel), which a
        # player tuning the radio never changes
        Hint.RADIO_CHANNEL:
            var radio:RailVehicleRadio = VehicleServer.vehicle_component_get(
                    vehicle, VehicleComponentType.COMPONENT_RADIO) as RailVehicleRadio
            return radio == null or radio.get_channel() == int(parameter)
        # an oil pump counts as on with the engine running too (driverhints.cpp:173)
        Hint.OIL_PUMP_ON:
            var diesel:RailVehicleDieselEngine = _diesel(vehicle)
            return diesel == null or not diesel.oil_pump_start_mode == RailVehicleController.START_MODE_MANUAL \
                    or diesel.get_oil_pump_enabled() or diesel.get_oil_pump_active() or diesel.get_main_switch_enabled()
        Hint.OIL_PUMP_OFF:
            var diesel:RailVehicleDieselEngine = _diesel(vehicle)
            return diesel == null or not diesel.oil_pump_start_mode == RailVehicleController.START_MODE_MANUAL \
                    or not (diesel.get_oil_pump_enabled() or diesel.get_oil_pump_active())
        Hint.FUEL_PUMP_ON:
            var diesel:RailVehicleDieselEngine = _diesel(vehicle)
            return diesel == null or not diesel.fuel_pump_start_mode == RailVehicleController.START_MODE_MANUAL \
                    or diesel.get_fuel_pump_enabled() or diesel.get_fuel_pump_active()
        Hint.FUEL_PUMP_OFF:
            var diesel:RailVehicleDieselEngine = _diesel(vehicle)
            return diesel == null or not diesel.fuel_pump_start_mode == RailVehicleController.START_MODE_MANUAL \
                    or not (diesel.get_fuel_pump_enabled() or diesel.get_fuel_pump_active())
        Hint.PANTOGRAPH_AIR_SOURCE_SET_MAIN, Hint.PANTOGRAPH_AIR_SOURCE_SET_AUXILIARY:
            var power_source:RailVehicleEnginePowerSource = _power_source(unit)
            return power_source == null or power_source.get_collector_pantograph_compressor_valve() \
                    == (hint == Hint.PANTOGRAPH_AIR_SOURCE_SET_AUXILIARY)
        Hint.PANTOGRAPH_COMPRESSOR_ON, Hint.PANTOGRAPH_COMPRESSOR_OFF:
            var power_source:RailVehicleEnginePowerSource = _power_source(unit)
            return power_source == null or power_source.get_collector_pantograph_compressor_enabled() \
                    == (hint == Hint.PANTOGRAPH_COMPRESSOR_ON)
        Hint.PANTOGRAPHS_VALVE_ON, Hint.PANTOGRAPHS_VALVE_OFF:
            var power_source:RailVehicleEnginePowerSource = _power_source(unit)
            return power_source == null or power_source.get_collector_valve_active() == (hint == Hint.PANTOGRAPHS_VALVE_ON)
        # a pantograph is the pantograph unit's own valve of it, which a vehicle without that
        # pantograph has as well - the unit's other car raises it (driverhints.cpp:269-310); one to
        # raise no longer matters past the speed the hint was given (driverhints.cpp:277, 300)
        Hint.FRONT_PANTOGRAPH_VALVE_ON, Hint.REAR_PANTOGRAPH_VALVE_ON:
            var power_source:RailVehicleEnginePowerSource = _power_source(unit)
            if power_source == null or (parameter > 0.0 and VehicleServer.vehicle_get_speed(vehicle) > parameter):
                return true
            return power_source.get_collector_pantograph_first_valve_active() if hint == Hint.FRONT_PANTOGRAPH_VALVE_ON \
                    else power_source.get_collector_pantograph_second_valve_active()
        Hint.FRONT_PANTOGRAPH_VALVE_OFF:
            var power_source:RailVehicleEnginePowerSource = _power_source(unit)
            return power_source == null or not power_source.get_collector_pantograph_first_valve_active()
        Hint.REAR_PANTOGRAPH_VALVE_OFF:
            var power_source:RailVehicleEnginePowerSource = _power_source(unit)
            return power_source == null or not power_source.get_collector_pantograph_second_valve_active()
        Hint.CONVERTER_ON, Hint.CONVERTER_OFF:
            var power_supply:RailVehiclePowerSupply = _power_supply(controlling)
            return power_supply == null or power_supply.get_converter_enabled() == (hint == Hint.CONVERTER_ON)
        Hint.PRIMARY_CONVERTER_OVERLOAD_RESET:
            var electric:RailVehicleElectricEngine = _engine(controlling) as RailVehicleElectricEngine
            return electric == null or not electric.get_converter_overload()
        Hint.MAIN_CIRCUIT_GROUND_RESET:
            var engine:RailVehicleEngine = _engine(controlling)
            return engine == null or engine.get_relay_ground()
        Hint.TRACTION_MOTOR_OVERLOAD_RESET:
            var electric:RailVehicleElectricEngine = _engine(controlling) as RailVehicleElectricEngine
            var diesel_electric:RailVehicleDieselElectricEngine = _engine(controlling) as RailVehicleDieselElectricEngine
            return not ((electric != null and electric.get_fuse_active())
                    or (diesel_electric != null and diesel_electric.get_fuse_active()))
        Hint.LINE_BREAKER_CLOSE, Hint.LINE_BREAKER_OPEN:
            var engine:RailVehicleEngine = _engine(controlling)
            return engine == null or engine.get_main_switch_enabled() == (hint == Hint.LINE_BREAKER_CLOSE)
        # any vehicle under control's - an EMU's compressor is in another car than its motors - allowed
        # to run, not running: it stops itself once the reservoir is full (driverhints.cpp:394-414)
        Hint.COMPRESSOR_ON:
            return situation.trainset.compressor_enabled
        Hint.COMPRESSOR_OFF:
            return not situation.trainset.compressor_explicitly_enabled
        # a pump not switched by hand needs no hint (driverhints.cpp:613-636)
        Hint.WATER_PUMP_ON:
            var diesel:RailVehicleDieselEngine = _diesel(controlling)
            return diesel == null or not diesel.water_pump_start_mode == RailVehicleController.START_MODE_MANUAL \
                    or diesel.get_water_pump_enabled() or diesel.get_water_pump_active()
        Hint.WATER_PUMP_OFF:
            var diesel:RailVehicleDieselEngine = _diesel(controlling)
            return diesel == null or not diesel.water_pump_start_mode == RailVehicleController.START_MODE_MANUAL \
                    or not (diesel.get_water_pump_enabled() or diesel.get_water_pump_active())
        Hint.WATER_PUMP_BREAKER_ON, Hint.WATER_PUMP_BREAKER_OFF:
            var diesel:RailVehicleDieselEngine = _diesel(controlling)
            return diesel == null or diesel.get_water_pump_breaker() == (hint == Hint.WATER_PUMP_BREAKER_ON)
        Hint.WATER_HEATER_ON, Hint.WATER_HEATER_OFF:
            var diesel:RailVehicleDieselEngine = _diesel(controlling)
            return diesel == null or diesel.get_water_heater_enabled() == (hint == Hint.WATER_HEATER_ON)
        Hint.WATER_HEATER_BREAKER_ON, Hint.WATER_HEATER_BREAKER_OFF:
            var diesel:RailVehicleDieselEngine = _diesel(controlling)
            return diesel == null or diesel.get_water_heater_breaker() == (hint == Hint.WATER_HEATER_BREAKER_ON)
        # an engine with one water circuit has nothing to link (driverhints.cpp:717, 728)
        Hint.WATER_CIRCUITS_LINK_ON, Hint.WATER_CIRCUITS_LINK_OFF:
            var diesel:RailVehicleDieselEngine = _diesel(controlling)
            return diesel == null or not diesel.cooling_water_aux_circuit \
                    or diesel.get_water_circuits_link() == (hint == Hint.WATER_CIRCUITS_LINK_ON)
        Hint.WAIT_TEMPERATURE_TOO_LOW:
            return not situation.state.heating_temperature_too_low
        Hint.FRONT_MOTOR_BLOWERS_ON, Hint.REAR_MOTOR_BLOWERS_ON:
            var engine:RailVehicleEngine = _engine(vehicle)
            var end:RailVehicleController.CouplerEnd = MOTOR_BLOWERS[hint][0]
            return engine == null or not engine.motor_blowers_start_mode == RailVehicleController.START_MODE_MANUAL \
                    or (engine.get_motor_blowers_enabled(end) and not engine.get_motor_blowers_disabled(end))
        # only a hand brake that is the vehicle's local brake (driverhints.cpp:474, 485)
        Hint.MANUAL_BRAKE_ON, Hint.MANUAL_BRAKE_OFF:
            var brake:RailVehicleBrake = _brake(vehicle)
            return brake == null or not brake.cntrl_local_brake_type == RailVehicleBrake.LOCAL_BRAKE_TYPE_MANUAL \
                    or not brake.cntrl_manual_brake_present \
                    or brake.get_manual_position() == (MANUAL_BRAKE_POSITIONS if hint == Hint.MANUAL_BRAKE_ON else 0)
        Hint.CONSIST_HEATING_ON, Hint.CONSIST_HEATING_OFF:
            var heating:RailVehicleHeating = VehicleServer.vehicle_component_get(
                    vehicle, VehicleComponentType.COMPONENT_HEATING) as RailVehicleHeating
            return heating == null or heating.get_allowed() == (hint == Hint.CONSIST_HEATING_ON)
        # light enough, or lights not switched by hand (driverhints.cpp:1158, 1171); a consist's
        # shade (ConsistShade) is not published - the scenery's light level alone
        Hint.CONSIST_LIGHTS_ON, Hint.CONSIST_LIGHTS_OFF:
            var controller:RailVehicleController = VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController
            var level:float = SimulationServer.get_light_level()
            if not controller.cntrl_compartment_lights_start_mode == RailVehicleController.START_MODE_MANUAL:
                return true
            if hint == Hint.CONSIST_LIGHTS_ON:
                return level > COMPARTMENT_LIGHTS_OFF_LEVEL or controller.get_compartment_lights_enabled() \
                        or controller.get_compartment_lights_active()
            return level < COMPARTMENT_LIGHTS_ON_LEVEL \
                    or not (controller.get_compartment_lights_enabled() or controller.get_compartment_lights_active())
        Hint.CONSIST_DOOR_LOCKS_ON:
            var doors:RailVehicleDoors = _doors(vehicle)
            return doors == null or not doors.has_lock or doors.get_lock_enabled()
        # a door without a warning, or warning by itself, needs no signal (driverhints.cpp:1040, 1051)
        Hint.DEPARTURE_SIGNAL_ON, Hint.DEPARTURE_SIGNAL_OFF:
            var doors:RailVehicleDoors = _doors(vehicle)
            if doors == null or not doors.close_warning or doors.close_auto_close_warning:
                return true
            if hint == Hint.DEPARTURE_SIGNAL_ON:
                return doors.get_departure_signal() or VehicleServer.vehicle_get_speed(vehicle) > DEPARTURE_SIGNAL_MAX_SPEED
            return not doors.get_departure_signal()
        Hint.HORN_ON:
            var horns:MaszynaMoverRailVehicleHorns = VehicleServer.vehicle_component_get(
                    vehicle, VehicleComponentType.COMPONENT_HORNS) as MaszynaMoverRailVehicleHorns
            return situation.state.warning_duration + HORN_HINT_EXTRA_TIME < HORN_HINT_END \
                    or horns == null or not horns.get_horn() == 0
        Hint.WAIT_LOAD_EXCHANGE:
            return not StationServer.dispatch_get_step(vehicle) == StationServer.DISPATCH_STEP_EXCHANGE
        Hint.WAIT_DEPARTURE_TIME:
            return not situation.route.at_passenger_stop
        Hint.TRAIN_BRAKE_SET_PIPE_UNLOCK:
            var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle) as MaszynaMoverRailVehicleBrake
            return brake == null or brake.main_pipe_minimum_unblocking_handle_position == PIPE_UNLOCK_NONE \
                    or situation.braking.position == brake.main_pipe_minimum_unblocking_handle_position
        Hint.MAIN_PIPE_UNLOCK:
            var brake:RailVehicleBrake = _brake(vehicle)
            return brake == null or not brake.get_main_pipe_locked()
        Hint.SPRING_BRAKE_ON, Hint.SPRING_BRAKE_OFF:
            var spring_brake:RailVehicleSpringBrake = RailVehicleServer.vehicle_component_get(
                    vehicle, RailVehicleComponentType.COMPONENT_SPRING_BRAKE) as RailVehicleSpringBrake
            return spring_brake == null or spring_brake.get_active() == (hint == Hint.SPRING_BRAKE_ON)
        Hint.MASTER_CONTROLLER_SET_IDLE:
            var configuration:MaszynaMoverDieselConfiguration = MaszynaDriverTraction.diesel_configuration(_diesel(controlling))
            var position:int = MaszynaDriverTraction.main_controller_position(situation)
            return configuration == null or position >= configuration.throttle_table_positions.size() \
                    or not (configuration.throttle_table_positions[position] as MaszynaMoverThrottlePositionItem).clutch_behavior \
                        == MaszynaMoverThrottlePositionItem.CLUTCH_BEHAVIOR_NONE
        Hint.MASTER_CONTROLLER_SET_SERIES_MODE:
            var engine:MaszynaMoverRailVehicleElectricSeriesEngine = _engine(controlling) as MaszynaMoverRailVehicleElectricSeriesEngine
            var position:int = MaszynaDriverTraction.main_controller_position(situation)
            return engine == null or position >= engine.relay_list.size() \
                    or (engine.relay_list[position] as MaszynaMoverRelayListItem).branch_count < 2
        Hint.MASTER_CONTROLLER_SET_ZERO_SPEED:
            var master:RailVehicleMasterController = MaszynaDriverTraction.master_controller(controlling)
            if master == null:
                return true
            # the second controller is off power at 0, or whatever its position under the speed
            # control (IsScndCtrlNoPowerPos(), Mover.cpp:2639-2641)
            var control:RailVehicleSpeedControl = RailVehicleServer.vehicle_component_get(
                    controlling, RailVehicleComponentType.COMPONENT_SPEED_CONTROL) as RailVehicleSpeedControl
            var second_off:bool = master.get_second_position() == 0 or (control != null and control.speed_control_enabled)
            # a diesel with a gearbox is off power where its own ZeroSpeed() stops - on a position
            # without the clutch in (DecSpeed(), Driver.cpp:3740-3749), never at 0, where its engine
            # gets no fuel; the original's check (IsMainCtrlNoPowerPos()) sent a player there. A
            # universal controller is off power up to UniCtrlNoPowerPos (Mover.cpp:2496-2498): its
            # lower positions are the train brake's (UCList IntegratedBrakePN, SA132), and a
            # "neutral" asked there could never let the brake be released
            var diesel:RailVehicleDieselEngine = _diesel(controlling)
            var position:int = master.get_main_position()
            # differs from the original: a universal controller is off power on its holding positions
            # too while no power comes (eimic_real, IsPower, Mover.cpp:7273) - the SA132's 4, where
            # its train brake releases and the original's own driver puts it (CheckTimeControllers()
            # 5.1, DecBrakeEIM(), Driver.cpp:3396-3400, 4304-4321). Only up to UniCtrlNoPowerPos, the
            # player was sent to lap and "release" the brake from there for ever
            var universal:MaszynaMoverRailVehicleUniversalController = RailVehicleServer.vehicle_component_get(
                    controlling, RailVehicleComponentType.COMPONENT_UNIVERSAL_CONTROLLER) as MaszynaMoverRailVehicleUniversalController
            var engine:RailVehicleEngine = _engine(controlling)
            if universal and engine and position < universal.positions.size() and second_off \
                    and (universal.positions[position] as MaszynaMoverUniversalControllerListItem).target_value <= 0.0 \
                    and _eimic_real(engine) <= NO_POWER_EIMIC:
                return true
            var engine_configuration:MaszynaMoverEngineConfiguration = MaszynaDriverTraction.engine_configuration(diesel)
            var diesel_configuration:MaszynaMoverDieselConfiguration = MaszynaDriverTraction.diesel_configuration(diesel)
            if diesel and diesel.get_type() == RailVehicleEngine.DIESEL and engine_configuration and diesel_configuration \
                    and engine_configuration.cntrl_eim_control_type == MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_0 \
                    and position < diesel_configuration.throttle_table_positions.size():
                return (diesel_configuration.throttle_table_positions[position] as MaszynaMoverThrottlePositionItem).clutch_behavior \
                        == MaszynaMoverThrottlePositionItem.CLUTCH_BEHAVIOR_NONE and second_off
            return position <= master.get_main_no_power_position() and second_off
        Hint.MASTER_CONTROLLER_SET_REVERSER_UNLOCK:
            var master:RailVehicleMasterController = MaszynaDriverTraction.master_controller(controlling)
            return master == null or master.get_main_position() <= master.direction_change_max_position
        Hint.TRAIN_BRAKE_RELEASE:
            var brake:RailVehicleBrake = _brake(vehicle)
            return brake == null or absf(brake.get_controller_position()
                    - brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_DRIVE)) <= HANDLE_TOLERANCE \
                    or (brake.get_handle_time_controlled()
                        and brake.get_handle_control_pressure() > brake.pipe_pressure_max - HANDLE_CONTROL_MARGIN)
        Hint.TRAIN_BRAKE_APPLY:
            return situation.trainset.braked
        # DirActive * CabActive (driverhints.cpp:921, 932) is the vehicle's own direction,
        # DirAbsolute (Mover.cpp:669)
        Hint.DIRECTION_FORWARD:
            return (VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController).get_direction_absolute() \
                    == VehicleController.DIRECTION_FORWARD
        Hint.DIRECTION_BACKWARD:
            return (VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController).get_direction_absolute() \
                    == VehicleController.DIRECTION_BACKWARD
        Hint.DIRECTION_NONE:
            return VehicleServer.vehicle_get_controller(vehicle).get_direction() == VehicleController.DIRECTION_NEUTRAL
        Hint.DIRECTION_OTHER:
            return situation.state.direction == situation.state.direction_order
        Hint.WAIT_PRESSURE_TOO_LOW:
            var brake:RailVehicleBrake = _brake(controlling)
            return brake == null or brake.get_compressor_pressure() > FULL_MAIN_RESERVOIR \
                    or is_zero_approx(brake.tank_volume_main)
        Hint.WAIT_PANTOGRAPH_PRESSURE_TOO_LOW:
            var power_source:RailVehicleEnginePowerSource = _power_source(unit)
            return power_source == null or power_source.get_collector_pantograph_tank_pressure() >= (
                    EMU_FULL_PANTOGRAPH_TANK if MaszynaDriverBraking.is_emu(vehicle) else FULL_PANTOGRAPH_TANK)
        Hint.SANDING_ON:
            var switches:RailVehicleSwitches = _switches(controlling)
            return switches == null or switches.get_sand_active() or not _slipping(controlling)
        Hint.SANDING_OFF:
            var switches:RailVehicleSwitches = _switches(controlling)
            return switches == null or not switches.get_sand_active()
        Hint.ANTISLIP:
            return not _slipping(controlling)
        Hint.DOOR_RIGHT_OPEN, Hint.DOOR_RIGHT_CLOSE, Hint.DOOR_LEFT_OPEN, Hint.DOOR_LEFT_CLOSE:
            # IsAnyDoorOpen[]: a door of the trainset open on that side (Driver.cpp:6042-6046)
            var left:bool = hint == Hint.DOOR_LEFT_OPEN or hint == Hint.DOOR_LEFT_CLOSE
            var any_open:bool = false
            for car:RID in situation.trainset.vehicles:
                var doors:RailVehicleDoors = VehicleServer.vehicle_component_get(
                        car, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
                if doors and (doors.get_left_open() if left else doors.get_right_open()):
                    any_open = true
            return any_open == (hint == Hint.DOOR_RIGHT_OPEN or hint == Hint.DOOR_LEFT_OPEN)
        Hint.DOOR_RIGHT_PERMIT_ON, Hint.DOOR_RIGHT_PERMIT_OFF, Hint.DOOR_LEFT_PERMIT_ON, Hint.DOOR_LEFT_PERMIT_OFF:
            var doors:RailVehicleDoors = VehicleServer.vehicle_component_get(
                    vehicle, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
            var left:bool = hint == Hint.DOOR_LEFT_PERMIT_ON or hint == Hint.DOOR_LEFT_PERMIT_OFF
            var permitted:bool = doors != null and (doors.get_left_open_permit() if left else doors.get_right_open_permit())
            return doors == null or permitted == (hint == Hint.DOOR_RIGHT_PERMIT_ON or hint == Hint.DOOR_LEFT_PERMIT_ON)
        # differs from the original, done once the vigilance alone stops (driverhints.cpp:1206): it is
        # cued for the cab signal as well (is_blinking(), Driver.cpp:6397), and with no button of
        # its own the cab signal is acknowledged by this one - an SHP flashing alone hinted nothing
        Hint.SECURITY_SYSTEM_RESET:
            var security:RailVehicleSecuritySystem = _security(vehicle)
            return security == null or not (security.get_vigilance_blinking()
                    or (security.get_cabsignal_blinking() and not security.get_separate_acknowledge()))
        Hint.SHP_SYSTEM_RESET:
            var security:RailVehicleSecuritySystem = _security(vehicle)
            return security == null or not security.get_cabsignal_blinking()
        Hint.COUPLING_ADAPTER_ATTACH:
            var coupling:RID = situation.state.coupling_vehicle
            return not coupling.is_valid() or RailVehicleServer.vehicle_is_coupler_automatic(
                    coupling, int(parameter) as RailVehicleController.CouplerEnd)
        Hint.COUPLING_ADAPTER_REMOVE:
            return not situation.trainset.vehicles or not RailVehicleServer.vehicle_get_coupler_adapter_model(
                    situation.trainset.vehicles[0], int(parameter) as RailVehicleController.CouplerEnd)
        Hint.TRACTIVE_FORCE_DECREASE:
            return absf(_tractive_force(controlling)) <= parameter
        # the acceleration wanted reached, or the controllers at their full power
        # (driverhints.cpp:577-585); a shunting mode is not published - the train's readiness decides
        Hint.TRACTIVE_FORCE_INCREASE:
            var acceleration_desired:float = situation.speed.acceleration_desired
            var master:RailVehicleMasterController = MaszynaDriverTraction.master_controller(controlling)
            var full_power:bool
            if not MaszynaDriverTraction.eim_control_type(situation) == MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_0:
                var engine:RailVehicleEngine = _engine(controlling)
                full_power = engine != null and _eimic_real(engine) >= EIM_FULL_POWER
            else:
                full_power = master == null or (master.get_main_position() >= master.main_position_count
                        and master.get_second_position() >= master.second_position_count)
            return acceleration_desired <= MaszynaDriverSpeed.NO_ACCELERATION or not situation.trainset.ready \
                    or acceleration_desired - situation.trainset.acceleration <= TRACTIVE_ACCELERATION_MARGIN \
                    or full_power
        Hint.BUFFERS_COMPRESS:
            return absf(_tractive_force(controlling)) > PRESSED_BUFFERS_FORCE \
                    or not situation.order & MaszynaAIDriver.Order.DISCONNECT
        Hint.BRAKING_FORCE_DECREASE:
            return absf(_braking_force(controlling)) <= parameter
        Hint.BRAKING_FORCE_INCREASE:
            var brake:RailVehicleBrake = _brake(vehicle)
            return absf(_braking_force(controlling)) > parameter or brake == null or absf(brake.get_controller_position()
                    - brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_EMERGENCY)) <= HANDLE_TOLERANCE
        # the handle where the driver's own release leaves it: the original checks the driving
        # position (driverhints.cpp:868), which an EP brake released at its EP releasing position
        # (DecBrake(), Driver.cpp:3321-3325) never shows
        Hint.BRAKING_FORCE_SET_ZERO:
            var brake:RailVehicleBrake = _brake(vehicle)
            var released:RailVehicleBrake.HandlePosition = RailVehicleBrake.HANDLE_POSITION_EP_RELEASE \
                    if _released_by_ep(vehicle) else RailVehicleBrake.HANDLE_POSITION_DRIVE
            return situation.trainset.ready and situation.trainset.brake_pressure_max < RELEASED_BRAKE_PRESSURE \
                    and (brake == null or absf(brake.get_controller_position()
                        - brake.get_handle_position(released)) <= HANDLE_TOLERANCE)
        Hint.INDEPENDENT_BRAKE_APPLY:
            var brake:RailVehicleBrake = _brake(vehicle)
            return brake == null or brake.get_local_position_normalized() >= parameter
        Hint.INDEPENDENT_BRAKE_RELEASE:
            var brake:RailVehicleBrake = _brake(vehicle)
            return brake == null or brake.get_local_position_normalized() < RELEASED_LOCAL_BRAKE
        Hint.INDEPENDENT_BRAKE_RELEASE_OTHER_END:
            for car:RID in situation.trainset.vehicles:
                var brake:RailVehicleBrake = _brake(car)
                if not car == vehicle and brake and brake.get_local_position_normalized() >= RELEASED_LOCAL_BRAKE:
                    return false
            return true
        Hint.RELEASER_ON, Hint.RELEASER_OFF:
            var brake:RailVehicleBrake = _brake(vehicle)
            return brake == null or brake.get_releaser_active() == (hint == Hint.RELEASER_ON)
        Hint.HEADCODE_PC1, Hint.HEADCODE_PC2, Hint.HEADCODE_PC5, Hint.HEADCODE_TB1, Hint.LIGHTS_OFF:
            return MaszynaDriverLights.shows(situation, hint)
    # a hint no driver cues has nothing to wait for
    return true


## The vehicle of the trainset a hint's device belongs to
static func device(situation:MaszynaDriverTraction.Situation, which:Device) -> RID:
    match which:
        Device.CONTROLLING:
            return situation.controlling
        Device.PANTOGRAPH_UNIT:
            return situation.trainset.pantograph_unit
    return situation.vehicle


## The cab's master controller - a joint controller where the cab has one in its place (SM42's)
static func master_controller(cabin:RID) -> StringName:
    return (MASTER_CONTROLLER if CabinSystem.has_control(cabin, MASTER_CONTROLLER)
            else MaszynaCabinJointController.CONTROL)


## The line breaker of the vehicle's engine closed; a vehicle without an engine has none
static func _main_switch_enabled(vehicle:RID) -> bool:
    var engine:RailVehicleEngine = _engine(vehicle)
    return engine != null and engine.get_main_switch_enabled()


## The vehicle's active cab (CabActive) - none on a vehicle without a master controller
static func _active_cab(vehicle:RID) -> int:
    var master:RailVehicleMasterController = MaszynaDriverTraction.master_controller(vehicle)
    return master.get_cabin() if master else 0


static func _power24_available(vehicle:RID) -> bool:
    var power_supply:RailVehiclePowerSupply = _power_supply(vehicle)
    return power_supply != null and power_supply.get_power24_available()


## Ft and Fb of a vehicle [N], none without the engine or the brake
static func _tractive_force(vehicle:RID) -> float:
    var engine:RailVehicleEngine = _engine(vehicle)
    return engine.get_tractive_force() if engine else 0.0


static func _braking_force(vehicle:RID) -> float:
    var brake:RailVehicleBrake = _brake(vehicle)
    return brake.get_force() if brake else 0.0


static func _slipping(vehicle:RID) -> bool:
    var wheels:RailVehicleWheels = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_WHEELS) as RailVehicleWheels
    return wheels != null and wheels.get_slipping()


## The train brake is released at its EP releasing position: an EP brake, but an induction motor's,
## which its handle releases at the driving position (DecBrake(), Driver.cpp:3300-3325)
static func _released_by_ep(vehicle:RID) -> bool:
    var brake:RailVehicleBrake = _brake(vehicle)
    var engine:RailVehicleEngine = _engine(vehicle)
    return brake and brake.cntrl_brake_system == RailVehicleBrake.BRAKE_SYSTEM_ELECTRO_PNEUMATIC \
            and not (engine and engine.get_type() == RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR)


static func _engine(vehicle:RID) -> RailVehicleEngine:
    return VehicleServer.vehicle_component_get(vehicle, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine


## The share of the engine's power its control asks for (eimic_real) - the Mover's model of each
## kind of engine has its own
static func _eimic_real(engine:RailVehicleEngine) -> float:
    var series:MaszynaMoverRailVehicleElectricSeriesEngine = engine as MaszynaMoverRailVehicleElectricSeriesEngine
    if series:
        return series.get_eimic_real()
    var induction:MaszynaMoverRailVehicleElectricInductionEngine = engine as MaszynaMoverRailVehicleElectricInductionEngine
    if induction:
        return induction.get_eimic_real()
    var diesel:MaszynaMoverRailVehicleDieselEngine = engine as MaszynaMoverRailVehicleDieselEngine
    if diesel:
        return diesel.get_eimic_real()
    var diesel_electric:MaszynaMoverRailVehicleDieselElectricEngine = engine as MaszynaMoverRailVehicleDieselElectricEngine
    return diesel_electric.get_eimic_real() if diesel_electric else 0.0


static func _diesel(vehicle:RID) -> RailVehicleDieselEngine:
    return _engine(vehicle) as RailVehicleDieselEngine


static func _brake(vehicle:RID) -> RailVehicleBrake:
    return RailVehicleServer.vehicle_component_get(vehicle, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake


static func _power_supply(vehicle:RID) -> RailVehiclePowerSupply:
    return RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_POWER_SUPPLY) as RailVehiclePowerSupply


static func _power_source(vehicle:RID) -> RailVehicleEnginePowerSource:
    return RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE) as RailVehicleEnginePowerSource


static func _switches(vehicle:RID) -> RailVehicleSwitches:
    return RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_SWITCHES) as RailVehicleSwitches


static func _doors(vehicle:RID) -> RailVehicleDoors:
    return VehicleServer.vehicle_component_get(vehicle, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors


static func _security(vehicle:RID) -> RailVehicleSecuritySystem:
    return RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_SECURITY) as RailVehicleSecuritySystem
