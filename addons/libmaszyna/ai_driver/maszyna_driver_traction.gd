@tool
extends RefCounted
class_name MaszynaDriverTraction

## The original driver's tractive force (TController::control_tractive_force(),
## increase_tractive_force(), control_handles(), Driver.cpp:7996-8063, 6438-6482): power added
## while the trainset accelerates less than wanted and runs slower than wanted, taken off when it
## runs too fast or accelerates too much. Every change is a step of the cab's controllers, as a
## player makes it (CabinSystem.act()).
##
## How a step of power is made depends on the engine of the driver's vehicle (IncSpeed(),
## DecSpeed(), SpeedSet(), Driver.cpp:3406-3827, 3846-4010): one subclass per engine type,
## chosen by create(). This base class is the vehicle without an engine of its own to drive.
##
## The doors are closed before adding power by the driver (MaszynaAIDriver). Not ported: the
## no-current sections of the track (fOverhead2, iOverheadZero) - see TODO.md, "Drivers".

const MASTER_CONTROLLER:StringName = MaszynaDriverHints.MASTER_CONTROLLER
const SECOND_CONTROLLER:StringName = MaszynaDriverHints.SECOND_CONTROLLER
const MOTOR_OVERLOAD_RESET:StringName = &"fuse_bt"
## A limit of exactly this [km/h] is driven up to without the margin - under it a train would never
## move off (Driver.cpp:8008-8010)
const CRAWL_VELOCITY:float = 1.0
## Within the distance kept to the next speed, power is added only this much [km/h] under it
## (Driver.cpp:8016)
const NEXT_VELOCITY_MARGIN:float = 1.0
## Going uphill harder than this [m/s2], power is taken off only when the driver wants to slow down
## (Driver.cpp:8036)
const UPHILL_GRAVITY:float = -0.01
## The acceleration over the one wanted that takes power off on the flat (Driver.cpp:8037)
const EXCESS_ACCELERATION:float = 10.05
## With the cruise control on and a target over SPEED_CONTROL_FROM [km/h], power comes off only
## SPEED_CONTROL_MARGIN over it (Driver.cpp:8027)
const SPEED_CONTROL_MARGIN:float = 3.0
const SPEED_CONTROL_FROM:float = 5.0
## The cruise control follows the next speed within this distance [m] at least (Driver.cpp:3605)
const SPEED_CONTROL_PROXIMITY:float = 50.0
## A target speed over this [km/h] is one the cruise control is set to (Driver.cpp:3613)
const SPEED_CONTROL_TARGET_FROM:float = 0.1
## fVoltage: the line's voltage the driver reckons with is the mean of the last one and the one read
## now; under the collector's lowest the driver waits WAIT_MIN plus up to WAIT_SPREAD [s]
## (Driver.cpp:6232-6240)
const VOLTAGE_SMOOTHING:float = 0.5
const LOW_VOLTAGE_WAIT_MIN:float = 2.0
const LOW_VOLTAGE_WAIT_SPREAD:float = 10.0
## Pressing the buffers to uncouple, power is added while the force is under this [N]
## (bufferscompress, driverhints.cpp:584)
const PRESSING_FORCE:float = 50000.0
## The sand used below this share of the high threshold of the motor overload relay
## (control_wheelslip(), Driver.cpp:6209)
const SANDING_CURRENT_SHARE:float = 0.75

## The positions of an EIM controller of a Traxx (EIMCtrlType 1) and of an Elf (2): driving, and
## the neutral one past which it brakes (Driver.cpp:3767-3780, 3801-3818)
const TRAXX_DRIVING_POSITION:int = 6
const TRAXX_NEUTRAL_POSITION:int = 4
const ELF_DRIVING_POSITION:int = 4
const ELF_NEUTRAL_POSITION:int = 2

## A universal controller's position takes power off faster than this (UniCtrlList[].SpeedDown,
## Driver.cpp:4152, 4232)
const UNIVERSAL_DECREASING_SPEED:float = 0.01

## Series mode is kept under this share of the collector's range of voltage, of a heavy goods train
## the lower (Driver.cpp:3456-3461, 6450-6454)
const SERIES_VOLTAGE_SHARE:float = 0.40
const HEAVY_SERIES_VOLTAGE_SHARE:float = 0.35

## The engine type the subclass drives (create())
var engine_type:RailVehicleEngine.EngineType = RailVehicleEngine.NONE
## fActionTime [s]: below zero the driver waits before it adds power; counts up with every update
var action_time:float = 0.0
## fVoltage [V]
var voltage:float = 0.0
## Need_TryAgain: power asked for without current coming - the controllers go back to zero on the
## next update (Driver.cpp:3553-3559, 7984-7993)
var retry:bool = false


## What one decision of the driver works with: the driver's state, its vehicle and cab, the engine
## its controls drive (MaszynaDriverTrainset.controlling) and what the driver made of its
## situation on this update - read afresh for every decision, never kept
class Situation:
    var state:MaszynaAIDriver.AIDriverState
    var traction:MaszynaDriverTraction
    var vehicle:RID
    var cabin:RID
    var controlling:RID
    var order:int
    var speed:MaszynaDriverSpeed
    var trainset:MaszynaDriverTrainset
    var route:MaszynaDriverRoute
    var braking:MaszynaDriverBraking
    ## DirectionalVel() [km/h]
    var directional_speed:float
    ## movePress: it presses the buffers to uncouple
    var pressing:bool
    ## moveConnect under a Connect order: it couples up
    var coupling:bool


## The traction of the driver's vehicle's engine (the switch of IncSpeed(), Driver.cpp:3409)
static func create(type:RailVehicleEngine.EngineType) -> MaszynaDriverTraction:
    var traction:MaszynaDriverTraction
    match type:
        RailVehicleEngine.ELECTRIC_SERIES_MOTOR:
            traction = MaszynaDriverSeriesMotorTraction.new()
        RailVehicleEngine.DIESEL_ELECTRIC:
            traction = MaszynaDriverDieselElectricTraction.new()
        RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR:
            traction = MaszynaDriverInductionMotorTraction.new()
        RailVehicleEngine.DIESEL:
            traction = MaszynaDriverDieselTraction.new()
        RailVehicleEngine.NONE:
            traction = MaszynaDriverControlCarTraction.new()
        _:
            traction = MaszynaDriverTraction.new()
    traction.engine_type = type
    return traction


## The driver waits `seconds` before it adds power (fActionTime < 0)
func hold(seconds:float) -> void:
    action_time = -seconds


## What the driver reads of its engine on every update (UpdateSituation(), Driver.cpp:5984,
## 6232-6240): the time it waits runs on, and the line's voltage
func read(situation:Situation, elapsed:float) -> void:
    action_time += elapsed
    var power_source:RailVehicleEnginePowerSource = RailVehicleServer.vehicle_component_get(
            situation.controlling, RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE) as RailVehicleEnginePowerSource
    # the line's voltage, from the vehicle's own pantographs or over the trainset's line (Driver.cpp:6141)
    voltage = VOLTAGE_SMOOTHING * (voltage + (maxf(power_source.get_collector_trainset_high_voltage(),
            power_source.get_collector_voltage()) if power_source else 0.0))
    if voltage < (power_source.get_collector_min_main_switch_voltage() if power_source else 0.0) \
            and action_time >= MaszynaAIDriver.PREPARE_TIME:
        action_time = -LOW_VOLTAGE_WAIT_MIN - randf() * LOW_VOLTAGE_WAIT_SPREAD


## The relays and the controllers before any power (control_tractive_and_braking_force(),
## control_relays(), control_motor_connectors(), control_wheelslip(), Driver.cpp:7925-7952,
## 7967-7993, 6200-6217); false when the power and the brakes are to be left alone
func prepare(situation:Situation) -> bool:
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
            situation.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    if action_time >= 0.0:
        if situation.trainset.motor_overload_relay_open:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_ZERO_SPEED)
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.TRACTION_MOTOR_OVERLOAD_RESET)
        # an engine without one has no ground relay to reset
        if engine and not engine.get_relay_ground():
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_ZERO_SPEED)
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MAIN_CIRCUIT_GROUND_RESET)
    if retry:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_ZERO_SPEED)
        retry = false
    # after a Radio-Stop the power only comes off, nothing else is touched
    var radio:RailVehicleRadio = VehicleServer.vehicle_component_get(
            situation.vehicle, VehicleComponentType.COMPONENT_RADIO) as RailVehicleRadio
    if radio and radio.get_radio_stop_active() \
            and VehicleServer.vehicle_get_speed(situation.vehicle) > MaszynaDriverTrainset.NO_MOVEMENT_SPEED:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_ZERO_SPEED)
        return false
    var wheels:RailVehicleWheels = VehicleServer.vehicle_component_get(
            situation.controlling, VehicleComponentType.COMPONENT_WHEELS) as RailVehicleWheels
    var slipping:bool = wheels != null and wheels.get_slipping()
    var switches:RailVehicleSwitches = RailVehicleServer.vehicle_component_get(
            situation.controlling, RailVehicleComponentType.COMPONENT_SWITCHES) as RailVehicleSwitches
    var sanding:bool = switches != null and switches.get_sand_active()
    var electric:RailVehicleElectricEngine = engine as RailVehicleElectricEngine
    var high_current:bool = electric != null \
            and absf(electric.get_motor_current()) > SANDING_CURRENT_SHARE * electric.circuit_imax_high
    if slipping or high_current:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.SANDING_ON)
    elif sanding:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.SANDING_OFF)
    # slipping, the controls are left alone - the power off and the brakes eased first
    if slipping:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.TRACTIVE_FORCE_DECREASE, 0.0, decrease.bind(situation, false))
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.BRAKING_FORCE_DECREASE, 0.0, situation.braking.ease.bind(situation))
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.ANTISLIP)
        return false
    return true


## One decision of the driver about the power (control_tractive_force(), Driver.cpp:7996-8040)
func control(situation:Situation) -> void:
    var velocity:float = situation.directional_speed
    var speed:MaszynaDriverSpeed = situation.speed
    var velocity_desired:float = speed.velocity_desired
    var acceleration_desired:float = speed.acceleration_desired
    var control:RailVehicleSpeedControl = RailVehicleServer.vehicle_component_get(
            situation.vehicle, RailVehicleComponentType.COMPONENT_SPEED_CONTROL) as RailVehicleSpeedControl
    var speed_control:bool = control != null and control.get_active()
    var full_power:bool = speed_control and velocity < control.full_power_velocity
    if acceleration_desired > MaszynaDriverSpeed.NO_ACCELERATION \
            and (situation.trainset.acceleration < acceleration_desired or full_power) and not situation.pressing:
        var margin:float = 0.0 if velocity_desired == CRAWL_VELOCITY else situation.route.velocity_minus
        # within the distance kept to the next speed, not over it
        # increase_tractive_force() (Driver.cpp:8043-8063): not while told to wait or with the train
        # stretched; the spring brake off first
        if velocity < velocity_desired - margin and (speed.proximity_distance > situation.route.max_proximity
                or velocity + NEXT_VELOCITY_MARGIN < speed.velocity_next) \
                and action_time >= 0.0 and not situation.trainset.coupler_stretched:
            var spring_brake:RailVehicleSpringBrake = RailVehicleServer.vehicle_component_get(
                    situation.vehicle, RailVehicleComponentType.COMPONENT_SPRING_BRAKE) as RailVehicleSpringBrake
            if spring_brake and spring_brake.get_active():
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.SPRING_BRAKE_OFF)
            var doors:RailVehicleDoors = VehicleServer.vehicle_component_get(
                    situation.vehicle, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
            if doors and doors.get_departure_signal():
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.DEPARTURE_SIGNAL_OFF)
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.TRACTIVE_FORCE_INCREASE, 0.0,
                    increase.bind(situation))
    if not situation.pressing:
        var speed_margin:float = SPEED_CONTROL_MARGIN if speed_control and velocity_desired > SPEED_CONTROL_FROM else 0.0
        if acceleration_desired <= MaszynaDriverSpeed.NO_ACCELERATION:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_ZERO_SPEED)
        elif velocity > velocity_desired + speed_margin or situation.trainset.coupler_stretched \
                or (acceleration_desired < 0.0 if situation.trainset.gravity_acceleration < UPHILL_GRAVITY
                else situation.trainset.acceleration > acceleration_desired + EXCESS_ACCELERATION):
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.TRACTIVE_FORCE_DECREASE, 0.0,
                    decrease.bind(situation, false))
    control_handles(situation)
    # SpeedSet() is the computer's own (AIControllFlag, Driver.cpp:3848)
    if DriverServer.vehicle_is_control_active(situation.vehicle):
        set_speed(situation)


## bufferscompress's action (driverhints.cpp:582-594): power against its own brakes, to press the
## buffers
func press(situation:Situation) -> void:
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
            situation.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    if absf(engine.get_tractive_force() if engine else 0.0) < PRESSING_FORCE:
        increase(situation)


## IncSpeed(): a step of power; true when the controllers moved
func increase(_situation:Situation) -> bool:
    return false


## DecSpeed(): a step of power off (never braking); true when the controllers moved. `force`: at
## once, as at a cab's activation
func decrease(_situation:Situation, _force:bool = false) -> bool:
    return false


## ZeroSpeed() (Driver.cpp:3683), mastercontrollersetzerospeed's action: the power off altogether
func zero(situation:Situation, force:bool = false) -> void:
    while decrease(situation, force):
        pass


## control_handles() (Driver.cpp:6438-6482): the engine's own care of its controllers
func control_handles(_situation:Situation) -> void:
    pass


## SpeedSet() (Driver.cpp:3846-4010): the regulation made with every decision about the power
func set_speed(_situation:Situation) -> void:
    pass


## The engine's part of CheckTimeControllers() (Driver.cpp:4260-4322), on the driver's update before
## any decision: controllers held by time go back to their holding positions
func check_time_controllers(_situation:Situation) -> void:
    pass


## The engine's part of SetTimeControllers() (Driver.cpp:4047-4258), on the driver's update after
## its decisions: controllers held by time moved to where they work until the next update
func set_time_controllers(_situation:Situation) -> void:
    pass


## SpeedCntrl() (Driver.cpp:4011-4045): the cruise control set to `velocity` [km/h]; 0 turns it off
func set_cruise_control(situation:Situation, velocity:float) -> void:
    var control:RailVehicleSpeedControl = RailVehicleServer.vehicle_component_get(
            situation.controlling, RailVehicleComponentType.COMPONENT_SPEED_CONTROL) as RailVehicleSpeedControl
    if control == null or not control.speed_control_enabled:
        return
    var second:int = second_controller_position(situation)
    var second_max:int = second_position_count(situation)
    if engine_type == RailVehicleEngine.DIESEL:
        if velocity < SPEED_CONTROL_TARGET_FROM:
            set_second_controller(situation, 0)
            velocity = 0.0
        elif second < 1:
            set_second_controller(situation, 1)
        MaszynaDriverHints.send(situation.vehicle, "speed_control_set", velocity)
    elif second_max == 1:
        set_second_controller(situation, 1)
        MaszynaDriverHints.send(situation.vehicle, "speed_control_set", velocity)
    elif second_max > 1 and not control.impulse_lever:
        var velocity_max:float = VehicleServer.vehicle_get_controller(situation.controlling).max_velocity
        set_second_controller(situation, 1 + int(second_max * ((velocity - 1.0) / velocity_max)))
    var driven:RailVehicleSpeedControl = RailVehicleServer.vehicle_component_get(
            situation.vehicle, RailVehicleComponentType.COMPONENT_SPEED_CONTROL) as RailVehicleSpeedControl
    if control.power_step > 0.0 and second_controller_position(situation) > 0 and driven:
        while driven.get_desired_power() < control.max_power:
            MaszynaDriverHints.send(situation.vehicle, "speed_control_power_increase")


## The cruise control's speed on an increase (Driver.cpp:3601-3617): the speed wanted, or the next
## one close to it, set when the unit takes it, else the unit off
func cruise(situation:Situation) -> void:
    var control:RailVehicleSpeedControl = RailVehicleServer.vehicle_component_get(
            situation.controlling, RailVehicleComponentType.COMPONENT_SPEED_CONTROL) as RailVehicleSpeedControl
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
            situation.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    if control == null or not control.speed_control_enabled or not (engine and engine.get_main_switch_enabled()):
        return
    var speed:MaszynaDriverSpeed = situation.speed
    var velocity:float = speed.velocity_desired \
            if speed.proximity_distance > maxf(SPEED_CONTROL_PROXIMITY, situation.route.max_proximity) or situation.coupling \
            else MaszynaDriverSpeed.min_speed(speed.velocity_desired, speed.velocity_next)
    var over:float = velocity - control.min_velocity
    if velocity >= control.min_velocity and over == snappedf(over, control.velocity_step):
        set_cruise_control(situation, velocity)
    elif velocity > SPEED_CONTROL_TARGET_FROM:
        set_cruise_control(situation, 0.0)
        set_second_controller(situation, 0)


## The master controller's position of the engine the controls drive (mvControlling->MainCtrlPos)
## - the cab's controllers act on it (CabinState.CONTROLLED_COMMANDS)
static func main_controller_position(situation:Situation) -> int:
    var master:RailVehicleMasterController = master_controller(situation.controlling)
    return master.get_main_position() if master else 0


## The second controller's position of the engine the controls drive (ScndCtrlPos)
static func second_controller_position(situation:Situation) -> int:
    var master:RailVehicleMasterController = master_controller(situation.controlling)
    return master.get_second_position() if master else 0


## The master controller of a vehicle, null on one without
static func master_controller(vehicle:RID) -> MaszynaMoverRailVehicleMasterController:
    return RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as MaszynaMoverRailVehicleMasterController


## A step of a controller; true when its `position` (one of the two above) moved
static func step(situation:Situation, control:StringName, action:StringName, position:Callable) -> bool:
    var before:int = position.call(situation)
    CabinSystem.act(situation.cabin, control, action)
    return not position.call(situation) == before


## A step of the master controller up (+1) or down (-1); true when it moved
static func step_main(situation:Situation, direction:int) -> bool:
    return step(situation, MaszynaDriverHints.master_controller(situation.cabin),
            &"increase" if direction > 0 else &"decrease", main_controller_position)


static func step_second(situation:Situation, direction:int) -> bool:
    return step(situation, SECOND_CONTROLLER, &"increase" if direction > 0 else &"decrease",
            second_controller_position)


## The master controller stepped to `position`, as far as it goes; true when it moved
static func set_main_controller(situation:Situation, position:int) -> bool:
    var start:int = main_controller_position(situation)
    var current:int = start
    while not current == position and step_main(situation, signi(position - current)):
        current = main_controller_position(situation)
    return not current == start


## The master controller put at `position` at once, as the original's driver sets an EIM
## controller's MainCtrlPos (Driver.cpp:3226-3239, 3387-3399, 3771-3816, 4284-4290): stepped through
## the cab it would restart the relay time on every update, and an Elf's power would never rise past
## its first step (CheckEIMIC(), Mover.cpp); true when it moved
static func put_main_controller(situation:Situation, position:int) -> bool:
    var before:int = main_controller_position(situation)
    MaszynaDriverHints.send(situation.controlling, "main_controller_set_position", position)
    return not main_controller_position(situation) == before


## The second controller stepped to `position`, as far as it goes (DecScndCtrl(2) to 0); true when
## it moved
static func set_second_controller(situation:Situation, position:int) -> bool:
    var start:int = second_controller_position(situation)
    var current:int = start
    while not current == position and step_second(situation, signi(position - current)):
        current = second_controller_position(situation)
    return not current == start


## MainCtrlPosNo / ScndCtrlPosNo of the engine the controls drive: its controllers' last positions
static func main_position_count(situation:Situation) -> int:
    var master:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
            situation.controlling, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
    return master.main_position_count if master else 0


static func second_position_count(situation:Situation) -> int:
    var master:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
            situation.controlling, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
    return master.second_position_count if master else 0


## EIMCtrlType of the engine the controls drive (Cntrl. EIMCtrlType)
static func eim_control_type(situation:Situation) -> MaszynaMoverEngineConfiguration.EimControlType:
    var configuration:MaszynaMoverEngineConfiguration = engine_configuration(VehicleServer.vehicle_component_get(
            situation.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine)
    return configuration.cntrl_eim_control_type if configuration else MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_0


## The Mover's model of the engine (its controls' kind), of each kind of engine its own; null for an
## engine of another backend
static func engine_configuration(engine:RailVehicleEngine) -> MaszynaMoverEngineConfiguration:
    var series:MaszynaMoverRailVehicleElectricSeriesEngine = engine as MaszynaMoverRailVehicleElectricSeriesEngine
    if series:
        return series.engine_configuration
    var induction:MaszynaMoverRailVehicleElectricInductionEngine = engine as MaszynaMoverRailVehicleElectricInductionEngine
    if induction:
        return induction.engine_configuration
    var diesel:MaszynaMoverRailVehicleDieselEngine = engine as MaszynaMoverRailVehicleDieselEngine
    if diesel:
        return diesel.engine_configuration
    var diesel_electric:MaszynaMoverRailVehicleDieselElectricEngine = engine as MaszynaMoverRailVehicleDieselElectricEngine
    return diesel_electric.engine_configuration if diesel_electric else null


## The Mover's model of a diesel engine (its throttle table, clutch and torque converter), of the
## diesel and the diesel-electric engine; null for another engine or backend
static func diesel_configuration(engine:RailVehicleEngine) -> MaszynaMoverDieselConfiguration:
    var diesel:MaszynaMoverRailVehicleDieselEngine = engine as MaszynaMoverRailVehicleDieselEngine
    if diesel:
        return diesel.diesel_configuration
    var diesel_electric:MaszynaMoverRailVehicleDieselElectricEngine = engine as MaszynaMoverRailVehicleDieselElectricEngine
    return diesel_electric.diesel_configuration if diesel_electric else null


## IncSpeedEIM() (Driver.cpp:3761-3789): power by the EIM controller's kind - a step, or straight to
## its driving position (Traxx 6, Elf 4); true when it moved
func increase_eim(situation:Situation) -> bool:
    var main:int = main_controller_position(situation)
    match eim_control_type(situation):
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_0:
            return step_main(situation, 1)
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_1:
            if main < TRAXX_DRIVING_POSITION:
                return put_main_controller(situation, TRAXX_DRIVING_POSITION)
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_2:
            if main < ELF_DRIVING_POSITION:
                return put_main_controller(situation, ELF_DRIVING_POSITION)
    return false


## DecSpeedEIM() (Driver.cpp:3791-3826): power off by the EIM controller's kind - a step, to its
## neutral position, or the cruise control's power down while the driver still wants to go
func decrease_eim(situation:Situation) -> bool:
    var main:int = main_controller_position(situation)
    match eim_control_type(situation):
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_0:
            return step_main(situation, -1)
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_1:
            if main > TRAXX_NEUTRAL_POSITION:
                return put_main_controller(situation, TRAXX_NEUTRAL_POSITION)
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_2:
            var control:RailVehicleSpeedControl = RailVehicleServer.vehicle_component_get(
                    situation.controlling, RailVehicleComponentType.COMPONENT_SPEED_CONTROL) as RailVehicleSpeedControl
            if situation.speed.acceleration_desired > 0.0 and control and control.get_active() \
                    and control.power_step > 0.0 and control.get_desired_power() > control.min_power:
                MaszynaDriverHints.send(situation.vehicle, "speed_control_power_decrease")
            elif main > ELF_NEUTRAL_POSITION:
                return put_main_controller(situation, ELF_NEUTRAL_POSITION)
    return false


## The voltage under which a series motor's controls keep to series mode (Driver.cpp:3456-3461)
func series_voltage(situation:Situation) -> float:
    var power_source:RailVehicleEnginePowerSource = RailVehicleServer.vehicle_component_get(
            situation.controlling, RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE) as RailVehicleEnginePowerSource
    if power_source == null:
        return 0.0
    return lerpf(power_source.get_collector_min_main_switch_voltage(), power_source.get_collector_max_voltage(),
            HEAVY_SERIES_VOLTAGE_SHARE if situation.braking.heavy_cargo else SERIES_VOLTAGE_SHARE)


## control_handles() of a series motor (Driver.cpp:6441-6464) - of the driver's own, or of the one
## an EMU's control car drives
func control_series_motor_handles(situation:Situation) -> void:
    var engine:RailVehicleElectricEngine = VehicleServer.vehicle_component_get(
            situation.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleElectricEngine
    var master:MaszynaMoverRailVehicleMasterController = master_controller(situation.controlling)
    # the line contactors dropped out: back to zero
    if not (engine and engine.is_line_contactor_closed()) and not (master and master.get_main_delayed()) \
            and main_powercontroller_position(situation) > 1:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_ZERO_SPEED)
    # a heavily burdened substation: series mode, to lessen the load
    if voltage <= series_voltage(situation):
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_SERIES_MODE)
    if not situation.trainset.ready and main_powercontroller_position(situation) > 1:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_ZERO_SPEED)


## mastercontrollersetseriesmode's action (driverhints.cpp:503-521): off the parallel positions
func set_series_mode(situation:Situation) -> void:
    var engine:MaszynaMoverRailVehicleElectricSeriesEngine = VehicleServer.vehicle_component_get(
            situation.controlling, VehicleComponentType.COMPONENT_ENGINE) as MaszynaMoverRailVehicleElectricSeriesEngine
    if engine == null:
        return
    var relays:Array = engine.relay_list
    if main_controller_position(situation) >= relays.size() \
            or (relays[main_controller_position(situation)] as MaszynaMoverRelayListItem).branch_count <= 1:
        return
    set_second_controller(situation, 0)
    while (relays[main_controller_position(situation)] as MaszynaMoverRelayListItem).branch_count > 1 \
            and step_main(situation, -1):
        pass


## MainCtrlPowerPos(): the master controller's position past the last without power
static func main_powercontroller_position(situation:Situation) -> int:
    var master:RailVehicleMasterController = master_controller(situation.controlling)
    return main_controller_position(situation) - (master.get_main_no_power_position() if master else 0)
