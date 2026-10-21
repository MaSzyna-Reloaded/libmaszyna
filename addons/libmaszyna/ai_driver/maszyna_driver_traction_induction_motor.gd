@tool
extends MaszynaDriverTraction
class_name MaszynaDriverInductionMotorTraction

## The traction of an electric induction motor (Traxx, Elf, the new EMUs; IncSpeed(), DecSpeed(),
## SetTimeControllers() 4., Driver.cpp:3597-3620, 3714-3716, 4122-4136): the EIM controller to its
## driving position, the cruise control set to the speed wanted; a cruise control with an impulse
## lever of four positions pushed up or down until it holds the speed wanted, in tens.

## The impulse lever: its positions that raise and lower the speed set, how many it has, and the
## tens the speed is set in (Driver.cpp:4127-4135)
const IMPULSE_LEVER_POSITIONS:int = 4
const IMPULSE_LEVER_RAISE:int = 3
const IMPULSE_LEVER_LOWER:int = 1
const IMPULSE_LEVER_MIDDLE:int = 2
## The EIM controllers' holding positions: a Traxx's driving and braking either side of its neutral
## one, an Elf's (Driver.cpp:4283-4292)
const TRAXX_NEUTRAL:int = 3
const TRAXX_DRIVING_HOLD:int = 5
const TRAXX_BRAKING_HOLD:int = 1
const ELF_DRIVING_HOLD:int = 3
const ELF_BRAKING_HOLD:int = 2
const IMPULSE_LEVER_STEP:float = 10.0
## The speed set counts as reached within this [km/h]
const IMPULSE_LEVER_TOLERANCE:float = 0.1


func increase(situation:MaszynaDriverTraction.Situation) -> bool:
    if situation.trainset.motor_overload_relay_open:
        return false
    # the original also goes in shunting mode (ShuntMode), which no induction motor here has
    if not (situation.trainset.ready or situation.pressing):
        return false
    var moved:bool = increase_eim(situation)
    cruise(situation)
    return moved


func decrease(situation:MaszynaDriverTraction.Situation, _force:bool = false) -> bool:
    return decrease_eim(situation)


## CheckTimeControllers() 3.-4. (Driver.cpp:4280-4299): a Traxx's controller to its holding driving
## or braking position, an Elf's by what it asks for, the impulse lever back to its middle
func check_time_controllers(situation:MaszynaDriverTraction.Situation) -> void:
    var main:int = main_controller_position(situation)
    match eim_control_type(situation):
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_1:
            if main > TRAXX_NEUTRAL:
                put_main_controller(situation, TRAXX_DRIVING_HOLD)
            elif main < TRAXX_NEUTRAL:
                put_main_controller(situation, TRAXX_BRAKING_HOLD)
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_2:
            var engine:MaszynaMoverRailVehicleElectricInductionEngine = VehicleServer.vehicle_component_get(
                    situation.vehicle, VehicleComponentType.COMPONENT_ENGINE) as MaszynaMoverRailVehicleElectricInductionEngine
            var asked:float = engine.get_eimic_real() if engine else 0.0
            if asked > 0.0:
                put_main_controller(situation, ELF_DRIVING_HOLD)
            elif asked < 0.0:
                put_main_controller(situation, ELF_BRAKING_HOLD)
    if _impulse_lever(situation):
        set_second_controller(situation, IMPULSE_LEVER_MIDDLE)


func set_time_controllers(situation:MaszynaDriverTraction.Situation) -> void:
    if not _impulse_lever(situation):
        return
    var speed:MaszynaDriverSpeed = situation.speed
    var velocity:float = speed.velocity_desired \
            if speed.proximity_distance > maxf(SPEED_CONTROL_PROXIMITY, situation.route.max_proximity) \
            else MaszynaDriverSpeed.min_speed(speed.velocity_desired, speed.velocity_next)
    velocity = IMPULSE_LEVER_STEP * floorf(velocity / IMPULSE_LEVER_STEP)
    var control:RailVehicleSpeedControl = RailVehicleServer.vehicle_component_get(
            situation.vehicle, RailVehicleComponentType.COMPONENT_SPEED_CONTROL) as RailVehicleSpeedControl
    var set_velocity:float = control.get_set_velocity()
    if set_velocity + IMPULSE_LEVER_TOLERANCE < velocity:
        set_second_controller(situation, IMPULSE_LEVER_RAISE)
    if set_velocity - IMPULSE_LEVER_TOLERANCE > velocity:
        set_second_controller(situation, IMPULSE_LEVER_LOWER)


## A cruise control with an impulse lever of four positions (SpeedCtrlTypeTime)
func _impulse_lever(situation:MaszynaDriverTraction.Situation) -> bool:
    var control:RailVehicleSpeedControl = RailVehicleServer.vehicle_component_get(
            situation.vehicle, RailVehicleComponentType.COMPONENT_SPEED_CONTROL) as RailVehicleSpeedControl
    var master:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
            situation.vehicle, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
    return control != null and control.impulse_lever and master != null \
            and master.second_position_count == IMPULSE_LEVER_POSITIONS
