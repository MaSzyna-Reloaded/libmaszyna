@tool
extends MaszynaDriverTraction
class_name MaszynaDriverControlCarTraction

## The traction of an EMU's control car (EN57; IncSpeed(), DecSpeed(), SpeedSet(),
## Driver.cpp:3411-3414, 3693-3701, 3851-3913): no engine of its own - the driver only decides to
## drive (moveIncSpeed), and the master controller is set on every update to the position the
## speed wanted takes, one whole group of positions at a time, and held a while after each.

## The speeds wanted [km/h] each further position takes - series, parallel, the three field
## shunts; the first position (przetok) is always taken (Driver.cpp:3880-3900)
const POSITION_VELOCITIES:Array[float] = [0.0, 20.0, 50.0, 80.0, 90.0, 100.0]
## How long the driver holds a position it set [s] (fActionTime = -5.0, Driver.cpp:3863, 3909)
const HOLD_TIME:float = 5.0

## moveIncSpeed: the driver drives
var driving:bool = false


func increase(situation:MaszynaDriverTraction.Situation) -> bool:
    if main_position_count(situation) > 0:
        driving = true
    return false


func decrease(situation:MaszynaDriverTraction.Situation, force:bool = false) -> bool:
    driving = false
    # at a cab's activation it goes to zero at once
    if force and main_position_count(situation) > 0:
        for _step:int in mini(main_powercontroller_position(situation), 2):
            step_main(situation, -1)
    return false


## control_handles() goes by the engine the car drives (mvControlling, Driver.cpp:6440)
func control_handles(situation:MaszynaDriverTraction.Situation) -> void:
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
            situation.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    if engine and engine.get_type() == RailVehicleEngine.ELECTRIC_SERIES_MOTOR:
        control_series_motor_handles(situation)


func set_speed(situation:MaszynaDriverTraction.Situation) -> void:
    if main_position_count(situation) <= 0:
        return
    if not driving:
        # the line off at once, whatever the time
        set_main_controller(situation, 0)
        if action_time >= 0.0:
            action_time = -HOLD_TIME
        return
    if action_time < 0.0 or not situation.trainset.ready:
        return
    # the second position of the reverser, so that an EN57 drives at full field (Driver.cpp:3874)
    if VehicleServer.vehicle_get_controller(situation.vehicle).get_direction() == VehicleController.DIRECTION_FORWARD:
        CabinSystem.act(situation.cabin, MaszynaDriverHints.REVERSER, &"increase")
    var engine:RailVehicleElectricEngine = VehicleServer.vehicle_component_get(
            situation.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleElectricEngine
    var main:int = main_controller_position(situation)
    if main > 0 and not (engine and engine.is_line_contactor_closed()):
        # the line contactors open: to zero, and wait for the camshaft to turn back
        for _step:int in 2:
            step_main(situation, -1)
    # set_speed() runs on a master controller only (main_position_count() above)
    elif not (main == 0 and master_controller(situation.controlling).get_main_actual_position() > 0):
        # from the position it stands at on, every one the speed wanted takes
        for position:int in range(main, POSITION_VELOCITIES.size()):
            if situation.speed.velocity_desired >= POSITION_VELOCITIES[position]:
                step_main(situation, 1)
    if main_controller_position(situation) > 0:
        action_time = -HOLD_TIME
