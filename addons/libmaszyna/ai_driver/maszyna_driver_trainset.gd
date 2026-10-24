@tool
extends RefCounted
class_name MaszynaDriverTrainset

## What the original's driver reads of its trainset on every update (TController::UpdateSituation(),
## Driver.cpp:6033-6190): the vehicles from the front the way it drives, whether their brakes let
## it start (Ready, fReady, IsConsistBraked), the pull of the slope along the track (fAccGravity)
## and the acceleration of the whole trainset (AbsAccS). The original sums every vehicle with the
## sign of its orientation; here each vehicle's front is taken along the way the driver drives,
## which is what those signs mean.
##
## Not read yet: the doors, the light, the relays other than the motor overload relay; the
## individual release of an overcharged vehicle (Driver.cpp:6059-6078) - see TODO.md, "Drivers".

## Original engine: MOVER.h:87 g [m/s2]
const GRAVITY:float = 9.81
## A vehicle slower than this [km/h] is starting, and its brake must be released below
## RELEASED_BRAKE_PRESSURE; a moving one only must not brake harder than MOVING_BRAKE_FORCE
## (Driver.cpp:6053-6055)
const STARTING_SPEED:float = 1.0
const RELEASED_BRAKE_PRESSURE:float = 0.4
## [kN]
const MOVING_BRAKE_FORCE:float = 10.0
const NEWTONS_PER_KILONEWTON:float = 1000.0
## The train brake counts as applied below this pipe pressure [bar] (Driver.cpp:6038)
const BRAKED_PIPE_PRESSURE:float = 3.9
const BRAKED_PIPE_MARGIN:float = 0.1
## A train that would roll back uphill starts with its brakes not quite released (Driver.cpp:6168)
const ROLLING_BACK_GRAVITY:float = -0.05
const ROLLING_BACK_BRAKE_PRESSURE:float = 0.8
## A diesel engine counts as started past this share of its idle speed, a vehicle moving faster
## than MOVEMENT_SPEED [km/h] as running (EU07_AI_MOVEMENT, Driver.cpp:6185-6189)
const ENGINE_STARTED_RATIO:float = 0.8
const MOVEMENT_SPEED:float = 1.0
## A vehicle slower than this [km/h] stands (EU07_AI_NOMOVEMENT, Driver.h:25)
const NO_MOVEMENT_SPEED:float = 0.05
## A vehicle with more power than this [kW] is an engine (Power > 1.0, Driver.cpp:2470-2489)
const POWERED:float = 1.0
## ... and one whose line breaker counts in the consist's state (Power > 0.01, Driver.cpp:6055)
const LINE_BREAKER_POWER:float = 0.01

## The way the driver drives along its vehicle (+1 or -1, iDirection)
var direction:int = 1
## From the front, the way the driver drives
var vehicles:Array[RID] = []
## fMass [kg], fLength [m]
var mass:float = 0.0
var length:float = 0.0
## The way the front vehicle is driven along itself (+1 or -1)
var front_direction:int = 1
## The way each of `vehicles` is driven along itself (+1 or -1, TDynamicObject::DirectionGet())
var directions:Array[int] = []
## fVelMax - the lowest top speed of the trainset [km/h], -1 for none
var velocity_max:float = -1.0
## Ready - no brake of the trainset holds it back
var ready:bool = false
## fReady - the highest brake cylinder pressure of the trainset [bar]
var brake_pressure_max:float = 0.0
## IsConsistBraked - the train brake is applied
var braked:bool = false
## fAccGravity - the slope's pull along the way the driver drives [m/s2]
var gravity_acceleration:float = 0.0
## AbsAccS - the trainset's acceleration along the way the driver drives [m/s2]
var acceleration:float = 0.0
## mvControlling (FindPowered(), DynObj.cpp:7772): the engine the driver's controls drive - its
## own vehicle when that has power, else the nearest one joined to it by the control line, or by the
## unit's permanent coupling in an EMU or DMU
var controlling:RID = RID()
## mvPantographUnit (FindPantographCarrier(), DynObj.cpp:7798): the vehicle whose pantographs the
## driver raises - first of its own unit, then of the vehicles under its control; none without
var pantograph_unit:RID = RID()
## ControlledEnginesCount (Driver.cpp:2470-2489): the engines driven by the driver's controls
var controlled_engines:int = 0
## IsAnyMotorOverloadRelayOpen (Driver.cpp:6135): the motor overload relay of a vehicle under the
## driver's control tripped
var motor_overload_relay_open:bool = false
## IsAnyLineBreakerOpen, IsAnyConverterOverloadRelayOpen (Driver.cpp:6045-6056): a line breaker of
## a powered vehicle under the driver's control open - tripped by a loss of voltage, or opened by a
## player - and a converter's overload relay tripped
var line_breaker_open:bool = false
var converter_overload_relay_open:bool = false
## IsAnyCompressorEnabled, IsAnyCompressorExplicitlyEnabled (Driver.cpp:6136-6137): a vehicle under
## control has its compressor allowed to run - switched on, or started on its own - and switched on
## by hand
var compressor_enabled:bool = false
var compressor_explicitly_enabled:bool = false
## IsAnyCouplerStretched (Driver.cpp:6087-6090): a coupler pulled past its strength
var coupler_stretched:bool = false
## FmaxC of the driver's vehicle's coupler behind it, the way it drives [N] (Driver.cpp:7784)
var coupler_strength:float = 0.0
## movePushPull - its front and rear are joined by the control line, a lone vehicle too: it turns
## by changing the cab, not by shunting (Driver.cpp:2540-2549)
var push_pull:bool = false


## Reads the trainset of the vehicle the driver drives, `driver_direction` +1 or -1 along the vehicle
func update(vehicle:RID, driver_direction:int, diesel_driven:bool) -> void:
    direction = driver_direction
    var ahead:RailVehicleController.CouplerEnd = (RailVehicleController.COUPLER_END_FRONT if direction >= 0
            else RailVehicleController.COUPLER_END_REAR)
    vehicles = RailVehicleServer.vehicle_get_coupled(vehicle, ahead, RailVehicleController.COUPLING_FLAG_COUPLER)
    push_pull = false
    if vehicles:
        push_pull = RailVehicleServer.vehicle_get_coupled(
                vehicles[0], RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_CONTROL).has(vehicles[-1])
    # the vehicles its controls reach (FindPowered(), the vehicles under control of UpdateSituation());
    # the original counts the front vehicle twice when it is not the driver's own
    # (Driver.cpp:2470-2489, MASZYNA_ORIGINAL_QUIRKS.md) - here every engine counts once
    controlling = RailVehicleServer.vehicle_find_powered(vehicle)
    pantograph_unit = RailVehicleServer.vehicle_find_pantograph_carrier(vehicle)
    var controlled:Array[RID] = RailVehicleServer.vehicle_get_coupled(
            vehicle, RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_CONTROL)
    controlled_engines = 0
    motor_overload_relay_open = false
    line_breaker_open = false
    converter_overload_relay_open = false
    compressor_enabled = false
    compressor_explicitly_enabled = false
    for other:RID in controlled:
        var power:float = VehicleServer.vehicle_get_controller(other).power
        if power > POWERED:
            controlled_engines += 1
        var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
                other, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
        # the fuse is the electric engines' own, of both kinds; the converter the electric one's
        var electric:RailVehicleElectricEngine = engine as RailVehicleElectricEngine
        var diesel_electric:RailVehicleDieselElectricEngine = engine as RailVehicleDieselElectricEngine
        motor_overload_relay_open = motor_overload_relay_open or (electric != null and electric.get_fuse_active()) \
                or (diesel_electric != null and diesel_electric.get_fuse_active())
        converter_overload_relay_open = converter_overload_relay_open \
                or (electric != null and electric.get_converter_overload())
        # the line breaker is the engine's: a powered car without one - an EMU's pantograph car,
        # PWR=2 - has none to be open (MASZYNA_ORIGINAL_QUIRKS.md, "A pantograph car's line breaker")
        if power > LINE_BREAKER_POWER and engine:
            line_breaker_open = line_breaker_open or not engine.get_main_switch_enabled()
        # an engine's compressor starts on its own (CompressorStart automatic, Mover.cpp:3889-3892);
        # CompressorAllowLocal is not modelled - always on
        var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
                other, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
        if brake and brake.compressor_speed > 0.0:
            compressor_enabled = compressor_enabled or brake.get_compressor_allowed() \
                    or brake.compressor_power == RailVehicleBrake.COMPRESSOR_POWER_ENGINE
            compressor_explicitly_enabled = compressor_explicitly_enabled or brake.get_compressor_allowed()
    var couplers:RailVehicleBuffCoupl = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_BUFFERS) as RailVehicleBuffCoupl
    var behind:RailVehicleController.CouplerEnd = RailVehicleController.opposite_end(ahead)
    coupler_strength = couplers.get_coupler_max_force(behind) if couplers else 0.0
    var driving:Vector3 = -RailVehicleServer.vehicle_get_transform(vehicle).basis.z * direction
    var driven_brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    ready = true
    brake_pressure_max = 0.0
    braked = (driven_brake.get_pipe_pressure() if driven_brake else 0.0) < BRAKED_PIPE_PRESSURE + BRAKED_PIPE_MARGIN
    mass = 0.0
    length = 0.0
    velocity_max = MaszynaDriverSpeed.NO_LIMIT
    var gravity_force:float = 0.0
    var momentum_change:float = 0.0
    var moving:bool = VehicleServer.vehicle_get_speed(vehicle) > NO_MOVEMENT_SPEED
    coupler_stretched = false
    directions.clear()
    for other:RID in vehicles:
        var controller:RailVehicleController = VehicleServer.vehicle_get_controller(other) as RailVehicleController
        var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
                other, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
        coupler_stretched = coupler_stretched or controller.get_coupler_stretched()
        var brake_pressure:float = maxf(0.0, brake.get_air_pressure() if brake else 0.0)
        if ready and brake and (brake.is_holding() or brake.is_braking()
                or (brake_pressure > RELEASED_BRAKE_PRESSURE if controller.get_speed() < STARTING_SPEED
                else brake.get_force() / NEWTONS_PER_KILONEWTON > MOVING_BRAKE_FORCE)):
            ready = false
        brake_pressure_max = maxf(brake_pressure, brake_pressure_max)
        var vehicle_mass:float = controller.get_mass_total()
        mass += vehicle_mass
        length += controller.dimensions_length
        velocity_max = MaszynaDriverSpeed.min_speed(velocity_max, controller.max_velocity)
        # the vehicle's front along the way the driver drives: its slope and its acceleration count
        # with that sign
        var front:Vector3 = -RailVehicleServer.vehicle_get_transform(other).basis.z
        var along:float = signf(front.dot(driving))
        directions.append(-1 if along < 0.0 else 1)
        if other == vehicles[0]:
            front_direction = directions[-1]
        gravity_force -= vehicle_mass * GRAVITY * front.y * along
        momentum_change += vehicle_mass * controller.get_acceleration() * along
    gravity_acceleration = gravity_force / mass if mass > 0.0 else 0.0
    acceleration = (momentum_change / mass if mass > 0.0 else 0.0) if moving else gravity_acceleration
    if not ready and gravity_acceleration < ROLLING_BACK_GRAVITY and brake_pressure_max < ROLLING_BACK_BRAKE_PRESSURE:
        ready = true
    # a diesel is ready once every live engine of the trainset has started
    if not diesel_driven:
        return
    for other:RID in vehicles:
        if not ready:
            return
        var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
                other, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
        var diesel:RailVehicleDieselEngine = engine as RailVehicleDieselEngine
        var idle:float = diesel.get_idle_rpm_count() if diesel else 0.0
        ready = VehicleServer.vehicle_get_speed(other) > MOVEMENT_SPEED \
                or not (engine != null and engine.get_main_switch_enabled()) \
                or (engine != null and engine.get_rpm_count() > ENGINE_STARTED_RATIO * idle)
