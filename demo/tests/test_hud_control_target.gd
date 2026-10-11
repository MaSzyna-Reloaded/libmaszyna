extends MaszynaGutTest

## A HUD control acts on the vehicle of its own target in the occupied vehicle's unit, as the cab's
## same control does: from an EN57's cab car the main switch reaches the motor car (mvControlled),
## the reverser stays in the cab car (mvOccupied) - Train.cpp OnCommand_linebreaker*,
## OnCommand_reverserincrease.

const CAB_CAR_PATH:String = "res://tests/fixtures/dynamic/pkp/en57-2000_v1/6ba.fiz"
const MOTOR_CAR_PATH:String = "res://tests/fixtures/dynamic/pkp/en57-2000_v1/6bs.fiz"
const SWITCH:PackedScene = preload("res://addons/libmaszyna/debug_hud/switch.tscn")
const BUTTON:PackedScene = preload("res://addons/libmaszyna/debug_hud/button.tscn")

var cab_car:RID
var motor_car:RID
var received:Array[Array] = []


func before_each() -> void:
    received.clear()
    var cab:VehicleController = build_vehicle("TestHudCabCar", FizVehicleBuilder.build_description_at(CAB_CAR_PATH),
            0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    var motor:VehicleController = build_vehicle("TestHudMotorCar", FizVehicleBuilder.build_description_at(MOTOR_CAR_PATH))
    await wait_idle_frames(2)
    cab.couple(motor, RailVehicleController.COUPLER_END_REAR, RailVehicleController.COUPLER_END_FRONT,
            RailVehicleController.COUPLING_FLAG_COUPLER | RailVehicleController.COUPLING_FLAG_PERMANENT
            | RailVehicleController.COUPLING_FLAG_CONTROL)
    cab_car = cab.get_rid()
    motor_car = motor.get_rid()
    VehicleServer.vehicle_command_received.connect(_on_command)


func after_each() -> void:
    VehicleServer.vehicle_command_received.disconnect(_on_command)


func _on_command(vehicle_rid:RID, command:String, _p1:Variant, _p2:Variant) -> void:
    received.append([vehicle_rid, command])


func test_the_main_switch_reaches_the_motor_car() -> void:
    var widget:DebugSwitch = SWITCH.instantiate()
    widget.command = "main_switch"
    widget.target = CabinState.Target.CONTROLLED
    add_child_autofree(widget)
    widget.vehicle = cab_car
    await wait_idle_frames(2)
    (widget.get_node("Switch") as CheckButton).button_pressed = true
    await wait_idle_frames(1)
    assert_has(received, [motor_car, "main_switch"], "the motor car's main switch")


func test_the_reverser_stays_in_the_cab_car() -> void:
    var widget:DebugButton = BUTTON.instantiate()
    widget.command = "direction_increase"
    add_child_autofree(widget)
    widget.vehicle = cab_car
    await wait_idle_frames(2)
    widget.pressed.emit()
    await wait_idle_frames(1)
    assert_has(received, [cab_car, "direction_increase"], "the cab car's reverser")
