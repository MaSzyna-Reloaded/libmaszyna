extends RefCounted
class_name MaszynaCabinJointController

## Joint master controller (MMD "jointctrl:", e.g. SM42's nastawnik): moving it forward adjusts
## power, backward past the no-power position applies the local brake. Ported from the cab layer:
## TTrain::OnCommand_mastercontrollerincrease/decrease (Train.cpp:1094-1146) with the joint
## controller branches of OnCommand_independentbrakeincrease/decrease (Train.cpp:1449-1510).
## The handle shows the joint position (Train.cpp:7699-7714), whoever moved the controllers.

## The master controller is the driven vehicle's, the local brake the cab's own (OnCommand_jointcontrollerset)
const TARGET:CabinState.Target = CabinState.Target.CONTROLLED
const CONTROL:StringName = &"jointctrl"
## The local brake's positions, the handle's range below zero (LocalBrakePosNo, hamulce.h:45)
const LOCAL_BRAKE_POSITIONS:int = 10

## The key the handle's widget reads its position by (MmdSemanticCatalog's jointctrl)
const STATE_KEY:String = "controller_joint_position"

var _cabin:RID
var _vehicle_rid:RID
## The local brake is the cab's own vehicle's, taken once (OnCommand_independentbrake*)
var _brake:RailVehicleBrake


func control_ids() -> Array[StringName]:
    return [CONTROL]


func register(vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    _vehicle_rid = vehicle_rid
    _brake = RailVehicleServer.vehicle_component_get(
            vehicle_rid, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    CabinSystem.register_control(cabin, CONTROL, _joint_controller)
    CabinSystem.state_computed_value_register(vehicle_rid, STATE_KEY, _joint_position)
    CabinSystem.vehicle_command_received.connect(_on_vehicle_command_received)
    CabinSystem.vehicle_cabin_built.connect(_on_vehicle_cabin_built)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CONTROL, _joint_controller)
    CabinSystem.state_computed_value_unregister(_vehicle_rid, STATE_KEY)
    CabinSystem.vehicle_command_received.disconnect(_on_vehicle_command_received)
    CabinSystem.vehicle_cabin_built.disconnect(_on_vehicle_cabin_built)


func _joint_controller(state:CabinState, action:StringName, _value:Variant) -> Variant:
    if action == &"increase":
        # Train.cpp:1098 - an applied local brake is released first
        if float(state.vehicle_state_value("brake_local_position_normalized", 0.0)) > 0.0:
            return state.send_vehicle_command("local_brake_decrease")
        return state.send_vehicle_command("main_controller_increase", null, null, TARGET)
    if action == &"decrease":
        # Train.cpp:1138 - below the no-power position the handle applies the local brake
        if state.vehicle_state_value("main_no_power_pos", false, TARGET):
            return state.send_vehicle_command("local_brake_increase")
        return state.send_vehicle_command("main_controller_decrease", null, null, TARGET)
    return null


## The handle stands where the controllers are, whoever moved them - its keys, the local brake's
## handle, the computer driver (CabinSystem.act()), a command sent past the cab (the console, a
## script). Its widget re-reads the position only on the master controller's commands. ggJointCtrl
## takes it every frame (Train.cpp:7699-7714); here it is taken on a command of the shown cab's
## train, as only a command moves the controllers, and no step of the cab is spent on it
func _on_vehicle_command_received(_vehicle_rid:RID, _command:String, _p1:Variant, _p2:Variant) -> void:
    CabinSystem.get_cabin_state(_cabin).set_value(CONTROL, _joint_position())


## A cab interior built anew reads the handle's position itself; the cab's value is brought to it,
## as a command given while it was hidden came to nobody
func _on_vehicle_cabin_built(vehicle_rid:RID, _interior:Cabin3D) -> void:
    if vehicle_rid == _vehicle_rid:
        CabinSystem.get_cabin_state(_cabin).set_value(CONTROL, _joint_position())


## The local brake of the cab's own vehicle, else the master controller of the driven one, as
## ggJointCtrl reads them (Train.cpp:7710-7714) - not the joint position of one vehicle: an EMU's
## control car takes its motor car's controller over only on its next step (DynObj.cpp:3196-3201)
func _joint_position() -> int:
    var local_brake:float = _brake.get_local_position_normalized() if _brake else 0.0
    if local_brake > 0.0:
        return -roundi(local_brake * LOCAL_BRAKE_POSITIONS)
    var master:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
            CabinState.vehicle_of(_vehicle_rid, TARGET),
            RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
    if not master:
        return 0
    if master.coupled_controllers:
        return master.get_main_position() + master.get_second_position()
    return master.get_main_position()
