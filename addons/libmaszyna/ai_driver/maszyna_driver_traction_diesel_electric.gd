@tool
extends MaszynaDriverTraction
class_name MaszynaDriverDieselElectricTraction

## The traction of a diesel-electric engine (SM42, ST44...; IncSpeed(), DecSpeed(),
## control_handles(), SetTimeControllers() 5.5, Driver.cpp:3580-3594, 3708-3713, 6465-6477,
## 4217-4246): the master controller up once the line contactors closed, then the second
## controller; power off the second controller first; an engine with the universal controller
## (EIMCtrlType 3) held at the position that keeps, adds or takes power.

## The acceleration off the one wanted a universal controller moves at [m/s2] (Driver.cpp:4236-4237)
const UNIVERSAL_HYSTERESIS:float = 0.05


func increase(situation:MaszynaDriverTraction.Situation) -> bool:
    var engine:RailVehicleDieselElectricEngine = VehicleServer.vehicle_component_get(
            situation.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleDieselElectricEngine
    if engine == null:
        return false
    # not with the overload relay or the pressure switch tripped; past the first position only
    # once the line contactors closed
    if situation.trainset.motor_overload_relay_open or engine.is_pressure_switch_tripped():
        return false
    if not (engine.get_main_no_power_pos() or engine.is_line_contactor_closed()):
        return false
    if not (situation.trainset.ready or situation.pressing):
        return false
    return increase_eim(situation) or step_main(situation, 1) or step_second(situation, 1)


func decrease(situation:MaszynaDriverTraction.Situation, _force:bool = false) -> bool:
    if second_controller_position(situation) > 0:
        return set_second_controller(situation, 0)
    # DecMainCtrl(min(MainCtrlPowerPos(), 2 + MainCtrlPowerPos() / 2)), Driver.cpp:3712
    var power:int = main_powercontroller_position(situation)
    var moved:bool = false
    for _step:int in mini(power, 2 + power / 2):
        moved = step_main(situation, -1) or moved
    return moved


func control_handles(situation:MaszynaDriverTraction.Situation) -> void:
    var engine:RailVehicleDieselElectricEngine = VehicleServer.vehicle_component_get(
            situation.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleDieselElectricEngine
    var master:MaszynaMoverRailVehicleMasterController = master_controller(situation.controlling)
    if not (engine and engine.is_line_contactor_closed()) and not (master and master.get_main_delayed()) \
            and main_powercontroller_position(situation) > 1:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_ZERO_SPEED)
    if not situation.trainset.ready and main_powercontroller_position(situation) > 1:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_ZERO_SPEED)


## The universal controller of a diesel-electric engine (SetTimeControllers() 5.5,
## Driver.cpp:4217-4246): the position that adds, keeps or takes power, as the trainset accelerates
## less or more than wanted
func set_time_controllers(situation:MaszynaDriverTraction.Situation) -> void:
    if not eim_control_type(situation) == MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_3:
        return
    var acceleration:float = situation.speed.acceleration_desired
    var engine:RailVehicleDieselElectricEngine = VehicleServer.vehicle_component_get(
            situation.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleDieselElectricEngine
    if acceleration < 0.0 or not (engine and engine.is_line_contactor_closed()):
        return
    var controller:MaszynaMoverRailVehicleUniversalController = RailVehicleServer.vehicle_component_get(
            situation.controlling, RailVehicleComponentType.COMPONENT_UNIVERSAL_CONTROLLER) as MaszynaMoverRailVehicleUniversalController
    if controller == null:
        return
    var positions:Array = controller.positions
    var increase_position:int = mini(positions.size() - 1,
            main_position_count(situation))
    var keep_position:int = 0
    var decrease_position:int = 0
    for index:int in range(increase_position, -1, -1):
        var item:MaszynaMoverUniversalControllerListItem = positions[index]
        if item.target_value > 0.0:
            continue
        if item.decrease_speed == 0.0:
            keep_position = index
        if item.decrease_speed > UNIVERSAL_DECREASING_SPEED:
            decrease_position = index
            break
    var achieved:float = situation.trainset.acceleration
    set_main_controller(situation,
            increase_position if acceleration > achieved + UNIVERSAL_HYSTERESIS
            else decrease_position if acceleration < achieved - UNIVERSAL_HYSTERESIS
            else keep_position)
