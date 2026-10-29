@tool
extends RefCounted
class_name MaszynaDriverBraking

## The original driver's brakes (TController::control_braking_force(), IncBrake(), DecBrake(),
## Driver.cpp:3014-3366, 8065-8190), operated through the cab as a player does (CabinSystem.act()).
## One per driver: it keeps the driver's own position of the train brake handle (BrakeCtrlPosition,
## the gbh_* scale, Driver.h:196-200) and the delay before the next adjustment (fBrakeTime); the
## position reaches the vehicle's handle the way its type takes it (SetTimeControllers(),
## Driver.cpp:4047-4097).
##
## The braking table (fBrake_a0/a1) is worked out of the trainset's own brakes as the original does
## (CheckVehicles(), Driver.cpp:2154-2340): the deceleration of the whole train at a quarter and at
## full pressure over its speeds, the threshold braking starts at (fAccThreshold), the reaction of
## its brakes and the brake setting each vehicle gets (G, P, R).
##
## Not ported yet: the electro-pneumatic and the EMU/DMU braking, the time-controlled handles
## (MHZ_K5P, MHZ_6P, M394, H14K1, St113, H1405 - moved every frame in the original), the manual
## brake and the spring brake let off by the crew - see TODO.md, "Drivers".

## BrakeCtrlPosition: lap, charging, running, and the range (gbh_*, Driver.h:196-200)
const POSITION_LAP:float = -2.0
const POSITION_CHARGING:float = -1.0
const POSITION_RUNNING:float = 0.0
const POSITION_MIN:float = -2.0
const POSITION_MAX:float = 6.0
## Uncoupling brakes the train at this position before it presses the buffers (trainbrakeapply,
## driverhints.cpp:810-817)
const POSITION_UNCOUPLING:float = 3.0
## BrakingInitialLevel, BrakingLevelIncrease (Driver.h:449-450): of a goods train 1.25 to start
## (Driver.cpp:2306-2314)
const BRAKING_INITIAL_LEVEL:float = 1.0
const CARGO_INITIAL_LEVEL:float = 1.25
const BRAKING_LEVEL_INCREASE:float = 0.25
## The braking table (BrakeAccTableSize, Driver.h:194; CheckVehicles(), Driver.cpp:2284-2300):
## steps of half the top speed, the brake force at a quarter and at full pressure, the rolling
## resistance per km/h, and the full positions the handle's twelve quarter steps make
const TABLE_SIZE:int = 20
const TABLE_SPEED_SHARE:float = 0.5
const TABLE_LOW_PRESSURE:float = 0.25
const TABLE_FULL_PRESSURE:float = 1.0
const TABLE_ROLLING:float = 0.001
const TABLE_FULL_STEPS:float = 12.0
## fAccThreshold of shunting (Driver.cpp:2266-2276) - an EMU earlier, and one with induction motors
## earlier still; of a train out of the table with the passenger's 4 and the goods' 1 full step
## (Driver.cpp:2315-2338), an EMU's or DMU's at most these
const SHUNT_THRESHOLD:float = -0.2
const EMU_SHUNT_THRESHOLD:float = -0.55
const DMU_SHUNT_THRESHOLD:float = -0.45
const INDUCTION_EMU_EARLIER:float = 0.10
const PASSENGER_THRESHOLD_STEPS:float = 4.0
const GOODS_THRESHOLD_STEPS:float = 1.0
const EMU_THRESHOLD_STEPS:float = 4.0
const EP_THRESHOLD_STEPS:float = 8.0
const DMU_THRESHOLD_STEPS:float = 8.0
const EMU_THRESHOLD_MAX:float = -0.75
const INDUCTION_EMU_THRESHOLD_MAX:float = -0.60
## An EMU's or DMU's threshold moves with its speed (Driver.cpp:6028-6031)
const MULTIPLE_UNIT_SPEED_SHARE:float = 0.015
const MULTIPLE_UNIT_SPEED_MIN:float = 0.5
## fBrakeReaction [s per km/h]: 1 plus this per metre of a passenger's or goods train; an EMU's
## or DMU's 0.25 (Driver.cpp:2325-2337)
const BRAKE_REACTION:float = 1.0
const MULTIPLE_UNIT_REACTION:float = 0.25
const PASSENGER_REACTION_PER_METRE:float = 0.004
const GOODS_REACTION_PER_METRE:float = 0.005
## The train is a passenger one with fewer goods wagons than this and than its others; a goods
## one under these lengths [m] and masses [kg] braked P, then GP, else G; GP puts this many
## wagons behind the engine on G (Driver.cpp:2166-2231)
const PASSENGER_GOODS_LIMIT:int = 4
const GOODS_P_LENGTH:float = 300.0
const GOODS_P_MASS:float = 600000.0
const GOODS_GP_LENGTH:float = 500.0
const GOODS_GP_MASS:float = 1300000.0
const GP_WAGONS_ON_G:int = 5
## IsHeavyCargoTrain (Driver.cpp:2304): past this a0 of the first step and mass per vehicle [kg]
const HEAVY_CARGO_A0:float = 0.4
const HEAVY_CARGO_MASS:float = 50000.0
## BrakeAccFactor() (Driver.cpp:2716-2733): the reaction counts 1.5 times with the handle under this
const FACTOR_RELEASED_POSITION:float = 0.5
const FACTOR_RELEASED_REACTION:float = 1.5
## braking_distance_multiplier() (Driver.cpp:1731-1771): above FAST_TARGET [km/h] none; under
## STOP_TARGET a goods train or one downhill past DOWNHILL_GRAVITY, braking harder than
## MULTIPLIER_A0, up to twice; between, up to MOST times for the slowest
const FAST_TARGET:float = 65.0
const STOP_TARGET:float = 5.0
const DOWNHILL_GRAVITY:float = 0.025
const MULTIPLIER_A0:float = 0.2
const STOP_MULTIPLIER:float = 2.0
const MOST_MULTIPLIER:float = 3.0
const MULTIPLIER_SPAN:float = 60.0
## A DMU's automatic gearbox brakes the last leg to a stop earlier: 1 + half per vehicle, 2-4
## times, easing out towards DMU_EASING_SPEED [km/h]
const DMU_MULTIPLIER_PER_VEHICLE:float = 0.5
const DMU_MULTIPLIER_MIN:float = 2.0
const DMU_MULTIPLIER_MAX:float = 4.0
const DMU_EASING_SPEED:float = 40.0
## Below this BrakeCtrlPosition the handle goes back to running (Driver.cpp:3283)
const RUNNING_RETURN_POSITION:float = 0.74
## A step of the handle that releases (Driver.cpp:3279)
const RELEASE_STEP:float = -0.25
## Harder than this [m/s2] at the last position is an emergency: one more (Driver.cpp:3101)
const EMERGENCY_ACCELERATION:float = -1.5
## The deepest position a second step of braking still goes to (Driver.cpp:3151)
const DEEPEST_DOUBLE_STEP:float = 5.0
## The local brake: its positions (LocalBrakePosNo, hamulce.h:39) and a step of one
const LOCAL_BRAKE:StringName = &"localbrake"
const LOCAL_BRAKE_POSITIONS:float = 10.0
const LOCAL_BRAKE_RELEASED:float = 0.0
const LOCAL_BRAKE_APPLIED:float = 1.0
## Released below this local brake position (independentbrakerelease, driverhints.cpp)
const LOCAL_BRAKE_OFF:float = 0.05
## DecLocalBrakeLevel(2) of the pneumatic release (Driver.cpp:3286)
const LOCAL_RELEASE_STEPS:int = 2
const TRAIN_BRAKE:StringName = &"brakectrl"
## The vehicle's handle counts as at a position this close (is_equal(..., 0.2), Driver.cpp:8182)
const HANDLE_TOLERANCE:float = 0.2
## control_braking_force() (Driver.cpp:8116-8155): braking starts past the gravity by these
## [m/s2]; standing uphill and rolling back faster than ROLLING_BACK_SPEED [km/h] brakes too
const BRAKING_MARGIN:float = 0.1
const SUDDEN_BRAKING_MARGIN:float = 0.5
const RELEASING_MARGIN:float = 0.05
const RELEASING_TABLE_FACTOR:float = 0.51
const RELEASING_EXCESS:float = 0.05
const ROLLING_BACK_GRAVITY:float = -0.05
const ROLLING_BACK_SPEED:float = -0.1
## Flat enough for the independent brake alone at a stop (Driver.cpp:8176)
const FLAT_GRAVITY:float = 0.01
## The brake's reaction the next adjustment waits for [s] (Driver.cpp:8121-8130, 8144-8149)
const BRAKE_DELAY_BASE:float = 3.0
const BRAKE_DELAY_SHARE:float = 0.5
const RELEASE_DELAY_SHARE:float = 3.0
## The goods delay setting (bdelay_G, hamulce.h:49), and which of BrakeDelay[] the original reads
## for applying and releasing past it (P, R) and at it
const DELAY_SETTING_G:int = 1
const DELAY_RELEASE_P:int = 0
const DELAY_APPLY_P:int = 1
const DELAY_RELEASE_G:int = 2
const DELAY_APPLY_G:int = 3
## The releaser (control_releaser(), Driver.cpp:8191-8250): the handles that have one, and the
## pressures [bar] it is pressed at - an empty pipe, a released cylinder, a charged control
## reservoir - and never past an overcharged pipe
const RELEASER_HANDLES:Array[int] = [
    MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_FV4A, MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_MHZ_6P, MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_MHZ_K5P,
    MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_MHZ_K8P, MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_M394,
]
const EMPTY_PIPE_PRESSURE:float = 3.0
const RELEASED_BRAKE_PRESSURE:float = 0.4
const CHARGED_CONTROL_RESERVOIR:float = 4.9
## Pressing the buffers, its own brake is released over this [bar] (Driver.cpp:8226)
const PRESSING_BRAKE_PRESSURE:float = 0.1
const OVERCHARGED_PIPE_PRESSURE:float = 5.2
## The old main pipe lock (lock_old, Mover.cpp:4034-4036): an FV4a handle over an LSt or EStED
## distributor (ss_LSt, BrakeSubsystemDecode()) feeds the pipe no more under this [bar] unless the releaser is
## held - its handle at running fills nothing from the main reservoir
const PIPE_LOCK_PRESSURE:float = 2.75
const PIPE_LOCK_VALVES:Array[int] = [
    MaszynaMoverRailVehicleBrake.BRAKE_VALVE_LST, MaszynaMoverRailVehicleBrake.BRAKE_VALVE_ESTED]
## The positions of MHZ_K8P and MHZ_EN57 (Driver.cpp:4072-4092)
const K8P_FULL_POSITION:float = 10.0
const K8P_FULL_FROM:float = 4.5
const K8P_STRONG_POSITION:float = 9.0
const K8P_STRONG_FROM:float = 3.70
const K8P_SCALE:float = 0.4
const K8P_OFFSET:float = 0.1
const K8P_STEP:float = 0.15
## A powered vehicle (Power > 1, Driver.cpp:3076-3081)
const POWERED:float = 1.0
## The control reservoir a vehicle charges to (Driver.cpp:3114)
const FULL_CONTROL_RESERVOIR:float = 5.0
const POSITION_CORRECTION_SCALE:float = 2.5
const FV4A_CONTROL_PRESSURE_SHARE:float = 0.2
## deltaAcc of the braking table positions: 4 x per full position (Driver.cpp:3126)
const TABLE_STEPS:float = 4.0

## An EMU's braking (control_braking_force(), Driver.cpp:8081-8092): downhill it starts no later
## than this [m/s2]; within EMU_BRAKING_MARGIN when its brakes can do more than EMU_MARGIN_SHARE of
## what is wanted
const EMU_DOWNHILL_THRESHOLD:float = -0.2
const EMU_BRAKING_MARGIN:float = 0.05
const EMU_MARGIN_SHARE:float = 1.1
## A DMU's handle releases a quarter at a time (Driver.cpp:3275)
const DMU_POSITION_STEP:float = 0.25
## SA134's stronger braking to a stop nearer than DMU_STOP_DISTANCE [m]: half a position more while
## the braking distance is under DMU_EARLY_SHARE of the distance (Driver.cpp:3137-3144)
const DMU_STOP_DISTANCE:float = 200.0
const DMU_EARLY_SHARE:float = 0.8
const DMU_HALF_STEP:float = 0.5
## EP releasing and holding this close are one position: the EP brake works by a switch
## (Driver.cpp:3192, 3326, 3348)
const EP_SWITCHED:float = 0.1
## MED_amax of a vehicle without the blended brake (MOVER.h:1403) - its EIM brakes by the local brake
const NO_MED_DECELERATION:float = 9.81
## AIHintLocalBrakeAccFactor's default (MOVER.h:1796)
const DEFAULT_LOCAL_BRAKE_FACTOR:float = 1.05
## IncBrakeEIM() of an EIM controller 0 (Driver.cpp:3210-3215): the driver's fMedAmax (Driver.h:377),
## the extra limit's factor, and an emergency controller's local brake short of full
const EIM_MAX_DECELERATION:float = 0.8
const EIM_BRAKE_LIMIT_FACTOR:float = 2.2
const EMERGENCY_LOCAL_BRAKE:float = 0.9
## DecBrakeEIM() of a Traxx and an Elf: the positions the braking ends at (Driver.cpp:3383-3394)
const EIM_TRAXX_BRAKE_OFF:int = 2
const EIM_ELF_BRAKE_OFF:int = 3
## The integrated local brake counts as full past this (Driver.cpp:4104-4106)
const INTEGRATED_BRAKE_FULL:float = 0.95
## A time-controlled handle holds within this [bar] of the pressure wanted (Driver.cpp:4057-4060)
const TIME_HANDLE_TOLERANCE:float = 0.05
## The universal brake buttons' flags pressed while charging (ub_HighPressure, ub_Overload,
## hamulce.h) and the position counting as charging (Driver.cpp:4250)
const UNIVERSAL_HIGH_PRESSURE:int = 0x04
const UNIVERSAL_OVERLOAD:int = 0x08
const CHARGING_TOLERANCE:float = 0.5
## The universal brake button's flag that unlocks the main pipe (ub_UnlockPipe, hamulce.h:151),
## pressed while the driver wants more than this [m/s2] (control_main_pipe(), Driver.cpp:8254)
const UNIVERSAL_UNLOCK_PIPE:int = 0x02
const PIPE_UNLOCK_ACCELERATION:float = -0.03

## The driver brakes from this BrakeCtrlPosition on (Driver.cpp:3131, 4188, 4313)
const BRAKING_FROM:float = 0.1

## BrakeCtrlPosition
var position:float = POSITION_RUNNING
## UniversalBrakeButtons' ub_UnlockPipe: the main pipe's lock held open
var _unlocking_pipe:bool = false
## The releaser hinted for the old main pipe lock, standing (_control_releaser())
var _releasing_locked_pipe:bool = false
## fBrakeTime [s]
var delay:float = 0.0
## fAccThreshold [m/s2]: braking starts past it
var acceleration_threshold:float = SHUNT_THRESHOLD
## IsHeavyCargoTrain: a goods train (RailVehicleServer.trainset_get_type()) braking poorly and
## heavy behind its engines
var heavy_cargo:bool = false
## fBrake_a0[0], fBrake_a1[0] - the braking table at the current speed; read by the speed wanted
var table_a0:float = 0.0
var table_a1:float = 0.0
## fBrake_a0[1..], fBrake_a1[1..]
var _a0:PackedFloat64Array = []
var _a1:PackedFloat64Array = []
## fNominalAccThreshold, fBrakeReaction
var _nominal_threshold:float = SHUNT_THRESHOLD
var _reaction:float = BRAKE_REACTION
## The trainset and the order the table was worked out for; a DMU (its gearbox)
var _checked:int = 0
var _dmu:bool = false


func _init() -> void:
    _a0.resize(TABLE_SIZE + 1)
    _a1.resize(TABLE_SIZE + 1)


## What the driver makes of its trainset's brakes (CheckVehicles(), Driver.cpp:2154-2340), again
## only when the trainset or the kind of order changed; then the table at the current speed
## (UpdateSituation(), Driver.cpp:6023-6031). `in_control`: it may set the brakes of its own vehicle.
func read_trainset(vehicle:RID, order:int, trainset:MaszynaDriverTrainset, in_control:bool) -> void:
    var controller:RailVehicleController = VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController
    var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle)
    var train_type:int = controller.train_type
    var emu:bool = train_type == MaszynaMoverRailVehicleController.TRAIN_TYPE_EZT
    var dmu:bool = train_type == MaszynaMoverRailVehicleController.TRAIN_TYPE_DMU
    _dmu = dmu
    var induction:bool = _induction(vehicle)
    var velocity_max:float = controller.max_velocity
    var train:bool = order & (MaszynaAIDriver.Order.OBEY_TRAIN | MaszynaAIDriver.Order.BANK)
    var shunt:bool = order & (MaszynaAIDriver.Order.SHUNT | MaszynaAIDriver.Order.LOOSE_SHUNT)
    var checked:int = hash([trainset.vehicles, train, shunt])
    if not checked == _checked:
        _checked = checked
        # what the trainset carries, judged where the original judges its train (AutoRewident(),
        # Driver.cpp:2302-2304); the brake settings below follow it
        RailVehicleServer.trainset_determine_type(vehicle)
        var passenger:bool = _set_brake_delays(vehicle, trainset, in_control)
        _a0.fill(0.0)
        _a1.fill(0.0)
        if shunt:
            _nominal_threshold = EMU_SHUNT_THRESHOLD if emu else (DMU_SHUNT_THRESHOLD if dmu else SHUNT_THRESHOLD)
            if emu and induction:
                _nominal_threshold += INDUCTION_EMU_EARLIER
            acceleration_threshold = _nominal_threshold
        if train and trainset.mass > 0.0 and velocity_max > 0.0:
            _build_table(trainset, velocity_max)
            heavy_cargo = RailVehicleServer.trainset_get_type(vehicle) == RailVehicleServer.TRAINSET_TYPE_CARGO \
                    and _a0[1] > HEAVY_CARGO_A0 and trainset.vehicles.size() - trainset.controlled_engines > 0 \
                    and trainset.mass / trainset.vehicles.size() > HEAVY_CARGO_MASS
            var last:int = TABLE_SIZE
            if emu:
                var steps:float = EP_THRESHOLD_STEPS \
                        if brake and brake.cntrl_brake_system == RailVehicleBrake.BRAKE_SYSTEM_ELECTRO_PNEUMATIC \
                        else EMU_THRESHOLD_STEPS
                _nominal_threshold = maxf(INDUCTION_EMU_THRESHOLD_MAX if induction else EMU_THRESHOLD_MAX,
                        -_a0[last] - steps * _a1[last])
                _reaction = MULTIPLE_UNIT_REACTION
            elif dmu:
                _nominal_threshold = maxf(EMU_THRESHOLD_MAX, -_a0[last] - DMU_THRESHOLD_STEPS * _a1[last])
                _reaction = MULTIPLE_UNIT_REACTION
            elif passenger:
                _nominal_threshold = -_a0[last] - PASSENGER_THRESHOLD_STEPS * _a1[last]
                _reaction = BRAKE_REACTION + trainset.length * PASSENGER_REACTION_PER_METRE
            else:
                _nominal_threshold = -_a0[last] - GOODS_THRESHOLD_STEPS * _a1[last]
                _reaction = BRAKE_REACTION + trainset.length * GOODS_REACTION_PER_METRE
            acceleration_threshold = _nominal_threshold
    # the table at the current speed
    var speed:float = VehicleServer.vehicle_get_speed(vehicle)
    var index:int = clampi(int(TABLE_SIZE * speed / velocity_max) if velocity_max > 0.0 else 1, 1, TABLE_SIZE)
    table_a0 = _a0[index]
    table_a1 = _a1[index]
    if emu or dmu:
        var share:float = clampf(speed * MULTIPLE_UNIT_SPEED_SHARE, MULTIPLE_UNIT_SPEED_MIN, 1.0)
        acceleration_threshold = _nominal_threshold * share - _a0[TABLE_SIZE] * (1.0 - share)


## BrakeAccFactor() (Driver.cpp:2716-2733): braking deeper the faster and the nearer the stop, and
## the more the train lags behind the deceleration wanted
func factor(
    speed:MaszynaDriverSpeed, route:MaszynaDriverRoute, trainset:MaszynaDriverTrainset,
    vehicle_speed:float
) -> float:
    var acceleration:float = speed.acceleration_desired
    if acceleration_threshold == 0.0 or acceleration >= 0.0:
        return 1.0
    if not (speed.proximity_distance > route.min_proximity or vehicle_speed > speed.velocity_desired + route.velocity_plus):
        return 1.0
    var reaction:float = _reaction * (FACTOR_RELEASED_REACTION if position < FACTOR_RELEASED_POSITION else 1.0)
    return 1.0 + reaction * vehicle_speed / (maxf(0.0, speed.proximity_distance) + 1.0) \
            * ((acceleration - trainset.acceleration) / acceleration_threshold)


## braking_distance_multiplier() (Driver.cpp:1731-1771): how much longer than the braking distance
## the train needs to reach `target` [km/h] - the slower the target, the longer; a goods train or
## one downhill braking hard needs up to twice as long to stop
func distance_multiplier(
    target:float, vehicle_speed:float, trainset:MaszynaDriverTrainset,
    trainset_type:RailVehicleServer.TrainsetType
) -> float:
    # wet or snowy rails grip less: everything the longer (1 / Global.FrictionWeatherFactor, Driver.cpp:1733)
    var weather:float = 1.0 / SimulationServer.simulation_get_weather_friction_factor()
    if target > FAST_TARGET:
        return weather
    if target < STOP_TARGET:
        if _dmu and vehicle_speed < DMU_EASING_SPEED and target == 0.0:
            var most:float = clampf(1.0 + trainset.vehicles.size() * DMU_MULTIPLIER_PER_VEHICLE, DMU_MULTIPLIER_MIN, DMU_MULTIPLIER_MAX)
            return lerpf(most, 1.0, vehicle_speed / DMU_EASING_SPEED) * weather
        if _a0[1] > MULTIPLIER_A0 and (trainset_type == RailVehicleServer.TRAINSET_TYPE_CARGO
                or trainset.gravity_acceleration > DOWNHILL_GRAVITY):
            return lerpf(1.0, STOP_MULTIPLIER, clampf((_a0[1] - MULTIPLIER_A0) / MULTIPLIER_A0, 0.0, 1.0)) * weather
        return weather
    return lerpf(MOST_MULTIPLIER, 1.0, (target - STOP_TARGET) / MULTIPLIER_SPAN) * weather


## CheckVehicles() 4. (Driver.cpp:2284-2300): the whole train's deceleration by the brakes at a
## quarter and at full pressure, step by step of half its top speed
func _build_table(trainset:MaszynaDriverTrainset, velocity_max:float) -> void:
    var step:float = velocity_max * TABLE_SPEED_SHARE / TABLE_SIZE
    for other:RID in trainset.vehicles:
        var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(other, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
        if not brake:
            continue
        for index:int in TABLE_SIZE:
            var velocity:float = step * (1 + 2 * index)
            _a0[index + 1] += brake.get_force_at(TABLE_LOW_PRESSURE, velocity)
            _a1[index + 1] += brake.get_force_at(TABLE_FULL_PRESSURE, velocity)
    for index:int in TABLE_SIZE:
        _a1[index + 1] -= _a0[index + 1]
        _a0[index + 1] /= trainset.mass
        _a0[index + 1] += TABLE_ROLLING * step * (1 + 2 * index)
        _a1[index + 1] /= TABLE_FULL_STEPS * trainset.mass


## CheckVehicles() 1.-3. (Driver.cpp:2154-2237): the brake setting the train needs, from its
## wagons - a passenger train P or R, a goods train P, GP or G by its length and mass - put on
## every vehicle by the crew (`auto_rewident`, the rewident's own lever); the driver's vehicle only
## while the driver drives it. A passenger train (ustaw > 16) by the trainset's type
## (RailVehicleServer.trainset_get_type()); a mixed one by the original's count of its wagons.
func _set_brake_delays(vehicle:RID, trainset:MaszynaDriverTrainset, in_control:bool) -> bool:
    var fast:int = 0
    var goods:int = 0
    var passengers:int = 0
    for other:RID in trainset.vehicles:
        if VehicleServer.vehicle_get_controller(other).power >= 1.0:
            continue
        var other_brake:MaszynaMoverRailVehicleBrake = _brake(other)
        var delays:int = other_brake.cntrl_brake_delays if other_brake else 0
        if delays & RailVehicleBrake.BRAKE_DELAY_R:
            fast += 1
        elif delays & RailVehicleBrake.BRAKE_DELAY_G:
            goods += 1
        else:
            passengers += 1
    var type:RailVehicleServer.TrainsetType = RailVehicleServer.trainset_get_type(vehicle)
    var passenger:bool = not type == RailVehicleServer.TRAINSET_TYPE_CARGO
    if type == RailVehicleServer.TRAINSET_TYPE_MIXED:
        passenger = goods < mini(PASSENGER_GOODS_LIMIT, fast + passengers)
    var setting:int = RailVehicleBrake.BRAKE_DELAY_R
    if fast + goods + passengers > 0:
        if passenger:
            setting = RailVehicleBrake.BRAKE_DELAY_P if goods and fast < goods + passengers else RailVehicleBrake.BRAKE_DELAY_R
        elif trainset.length < GOODS_P_LENGTH and trainset.mass < GOODS_P_MASS:
            setting = RailVehicleBrake.BRAKE_DELAY_P
        elif trainset.length < GOODS_GP_LENGTH and trainset.mass < GOODS_GP_MASS:
            # GP, in the original's own numbering the R of a goods train
            setting = RailVehicleBrake.BRAKE_DELAY_R
        else:
            setting = RailVehicleBrake.BRAKE_DELAY_G
    var behind_engine:int = 0
    for other:RID in trainset.vehicles:
        # the driver's own vehicle only while it drives it, and only a vehicle with a brake to set
        var other_brake:MaszynaMoverRailVehicleBrake = _brake(other)
        if other == vehicle and not in_control or not other_brake:
            continue
        var powered:bool = VehicleServer.vehicle_get_controller(other).power > POWERED
        var delays:int = other_brake.cntrl_brake_delays
        var own:int
        if passenger:
            own = RailVehicleBrake.BRAKE_DELAY_R if setting == RailVehicleBrake.BRAKE_DELAY_R and delays & RailVehicleBrake.BRAKE_DELAY_R \
                    else RailVehicleBrake.BRAKE_DELAY_P
        elif setting == RailVehicleBrake.BRAKE_DELAY_P:
            own = RailVehicleBrake.BRAKE_DELAY_G if powered else RailVehicleBrake.BRAKE_DELAY_P
        elif setting == RailVehicleBrake.BRAKE_DELAY_G:
            own = RailVehicleBrake.BRAKE_DELAY_G if delays & RailVehicleBrake.BRAKE_DELAY_G else RailVehicleBrake.BRAKE_DELAY_P
        else:
            if powered:
                behind_engine = 0
                own = RailVehicleBrake.BRAKE_DELAY_G
            else:
                behind_engine += 1
                own = RailVehicleBrake.BRAKE_DELAY_G if behind_engine <= GP_WAGONS_ON_G else RailVehicleBrake.BRAKE_DELAY_P
        MaszynaDriverHints.send(other, "auto_rewident", own)
    return passenger


## One decision of the driver about the brakes, `elapsed` [s] after the last one
## (control_braking_force(), Driver.cpp:8065-8177)
func control(situation:MaszynaDriverTraction.Situation, elapsed:float) -> void:
    delay -= elapsed
    # differs from the original, which takes its level from the handle only while preparing the
    # engine (PrepareEngine(), Driver.cpp:2867-2878): a player's handle moved from cutoff to running
    # after that left the level lapped, and an EN57 told to move off was never told to reduce the
    # braking force (MASZYNA_ORIGINAL_QUIRKS.md) - the player's handle is the level on every decision
    if not DriverServer.vehicle_is_control_active(situation.vehicle):
        take_handle_position(situation.vehicle)
    # the train brake is never left locked (Driver.cpp:7950-7952)
    if position == POSITION_LAP:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.TRAIN_BRAKE_RELEASE, 0.0, release_train_brake)
    # the brakes are not touched while the buffers are pressed to uncouple
    if situation.pressing:
        return
    var vehicle:RID = situation.vehicle
    var cabin:RID = situation.cabin
    var speed:MaszynaDriverSpeed = situation.speed
    var trainset:MaszynaDriverTrainset = situation.trainset
    var acceleration:float = speed.acceleration_desired
    var gravity:float = trainset.gravity_acceleration
    var directional_speed:float = situation.directional_speed
    var disconnecting:bool = situation.order == MaszynaAIDriver.Order.DISCONNECT
    # accelerating, it does not brake - but not while uncoupling
    if acceleration > 0.0 and not disconnecting:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.BRAKING_FORCE_SET_ZERO, 0.0, _release.bind(situation))
    if is_emu(vehicle):
        # an EMU's own braking: the EP brake answers at once (Driver.cpp:8078-8110)
        var threshold:float = acceleration_threshold if gravity < DOWNHILL_GRAVITY \
                else maxf(EMU_DOWNHILL_THRESHOLD, acceleration_threshold)
        var acceleration_max:float = minf(table_a0 + TABLE_FULL_STEPS * table_a1, _med_max_deceleration(vehicle))
        var margin:float = EMU_BRAKING_MARGIN if acceleration_max > EMU_MARGIN_SHARE * acceleration and gravity < DOWNHILL_GRAVITY \
                else 0.0
        if acceleration < threshold and (trainset.acceleration > acceleration + margin or position < POSITION_RUNNING):
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.BRAKING_FORCE_INCREASE, 0.0, _increase.bind(situation, 1.0))
        elif not disconnecting:
            if trainset.acceleration < acceleration - RELEASING_MARGIN:
                if position >= POSITION_RUNNING and speed.velocity_desired > 0.0:
                    MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.BRAKING_FORCE_DECREASE, 0.0, _decrease.bind(
                            situation, factor(speed, situation.route, trainset, absf(directional_speed))))
            else:
                # LapBrake() (Driver.cpp:3344-3353): an EP brake applied by time held where it is
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.BRAKING_FORCE_LAP, 0.0, func() -> void:
                    var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle)
                    if not (brake and brake.get_handle_ep_time_controlled()):
                        return
                    var hold:float = brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_EP_HOLD)
                    if brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_EP_RELEASE) - hold < EP_SWITCHED:
                        MaszynaDriverHints.send(vehicle, "ep_brake", false)
                    else:
                        _set_handle(vehicle, cabin, hold))
    else:
        var brake_factor:float = factor(speed, situation.route, trainset, absf(directional_speed))
        if (acceleration < gravity - BRAKING_MARGIN and trainset.acceleration > acceleration + table_a1) \
                or (gravity < ROLLING_BACK_GRAVITY and directional_speed < ROLLING_BACK_SPEED):
            if delay < 0.0 or acceleration < gravity - SUDDEN_BRAKING_MARGIN or position <= POSITION_RUNNING:
                # with the delay before the next change of the brakes (driverhints.cpp:840-845)
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.BRAKING_FORCE_INCREASE, 0.0, func() -> void:
                    if _increase(situation, brake_factor):
                        delay = (BRAKE_DELAY_BASE + BRAKE_DELAY_SHARE
                                * (_brake_delay(vehicle, DELAY_APPLY_P, DELAY_APPLY_G) - BRAKE_DELAY_BASE)) * BRAKE_DELAY_SHARE)
        if acceleration < gravity - RELEASING_MARGIN \
                and (acceleration - table_a1 * RELEASING_TABLE_FACTOR) - trainset.acceleration > RELEASING_EXCESS \
                and not disconnecting and speed.velocity_desired > 0.0:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.BRAKING_FORCE_DECREASE, 0.0, func() -> void:
                if _decrease(situation, brake_factor):
                    delay = _brake_delay(vehicle, DELAY_RELEASE_P, DELAY_RELEASE_G) / RELEASE_DELAY_SHARE * BRAKE_DELAY_SHARE)
    # at a stop: the locomotive held by its own brake on the flat, the train released
    # (Driver.cpp:8166-8180)
    var standing:bool = VehicleServer.vehicle_get_speed(vehicle) < MaszynaDriverTrainset.NO_MOVEMENT_SPEED
    if standing and (speed.velocity_desired == 0.0 or acceleration <= MaszynaDriverSpeed.NO_ACCELERATION):
        var joining:int = (MaszynaAIDriver.Order.DISCONNECT | MaszynaAIDriver.Order.CONNECT
                | MaszynaAIDriver.Order.CHANGE_DIRECTION)
        if not situation.order & joining and absf(gravity) < FLAT_GRAVITY:
            apply_independent_brake_only(situation)
        # told to turn, the brake left applied in the cab it leaves is let go (Driver.cpp:8171-8175)
        if situation.order & MaszynaAIDriver.Order.CHANGE_DIRECTION:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.INDEPENDENT_BRAKE_RELEASE)
    _control_releaser(situation, acceleration)


## brakingforcedecrease's action as the wheels slip: the brakes eased once (control_wheelslip(),
## Driver.cpp:6205)
func ease(situation:MaszynaDriverTraction.Situation) -> void:
    _decrease(situation, 1.0)


## CheckTimeControllers() 1. (Driver.cpp:4264-4278), on the driver's update before any decision: a
## handle held by time goes back to holding - the pipe changed while it was held in its last update
func check_time_controllers(situation:MaszynaDriverTraction.Situation) -> void:
    var brake:MaszynaMoverRailVehicleBrake = _brake(situation.vehicle)
    if brake == null:
        return
    if brake.cntrl_brake_system == RailVehicleBrake.BRAKE_SYSTEM_ELECTRO_PNEUMATIC and brake.get_handle_ep_time_controlled():
        _set_handle(situation.vehicle, situation.cabin, brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_EP_HOLD))
    elif brake.cntrl_brake_system == RailVehicleBrake.BRAKE_SYSTEM_PNEUMATIC and brake.get_handle_time_controlled():
        _set_handle(situation.vehicle, situation.cabin, brake.get_handle_position(
                RailVehicleBrake.HANDLE_POSITION_FIRST_STEP if position > POSITION_RUNNING
                else RailVehicleBrake.HANDLE_POSITION_DRIVE))
    _hold_universal_controller(situation)


## SetTimeControllers() 1., 3., 6. (Driver.cpp:4047-4102, 4104-4112, 4248-4258), on the driver's
## update after its decisions: its position to the vehicle's handle as the handle's type takes it
## - FV4a as it is, MHZ_K8P and MHZ_EN57 by their table, a handle held by time to where the pipe
## goes towards the position's pressure; the local brake full as the EIM controller's braking; the
## universal brake buttons at charging
func set_time_controllers(situation:MaszynaDriverTraction.Situation) -> void:
    var vehicle:RID = situation.vehicle
    var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle)
    if brake == null:
        return
    if brake.cntrl_brake_system == RailVehicleBrake.BRAKE_SYSTEM_PNEUMATIC:
        if brake.get_handle_time_controlled():
            # a handle held by time (Driver.cpp:4054-4067): braking, to full braking while the pipe
            # is over the pressure of the position wanted, to running under it; else to running,
            # charging or lap
            # DeltaPipePress: the pipe's working range (Mover.cpp:10474)
            var wanted:float = brake.pipe_pressure_max \
                    - position * TABLE_LOW_PRESSURE * (brake.pipe_pressure_max - brake.pipe_pressure_min)
            var difference:float = brake.get_handle_control_pressure() - wanted
            var target:Variant = null
            if position > POSITION_RUNNING and difference > TIME_HANDLE_TOLERANCE:
                target = RailVehicleBrake.HANDLE_POSITION_FULL
            elif position > POSITION_RUNNING and difference < -TIME_HANDLE_TOLERANCE:
                target = RailVehicleBrake.HANDLE_POSITION_DRIVE
            elif position == POSITION_RUNNING:
                target = RailVehicleBrake.HANDLE_POSITION_DRIVE
            elif position == POSITION_CHARGING:
                target = RailVehicleBrake.HANDLE_POSITION_FILLING
            elif position == POSITION_LAP:
                target = RailVehicleBrake.HANDLE_POSITION_CUTOFF
            if not target == null:
                _set_handle(vehicle, situation.cabin, brake.get_handle_position(target))
        _apply_handle(vehicle, situation.cabin, brake)
    var controller:RailVehicleUniversalController = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_UNIVERSAL_CONTROLLER) as RailVehicleUniversalController
    if controller and controller.integrated_local_brake \
            and brake.get_local_position_normalized() > INTEGRATED_BRAKE_FULL:
        while _increase_eim(situation):
            pass
    _hold_universal_controller(situation)
    # the universal buttons: high pressure and overload while charging (Driver.cpp:4248-4258)
    var charging:bool = absf(position - POSITION_CHARGING) < CHARGING_TOLERANCE
    var pressed:int = (UNIVERSAL_HIGH_PRESSURE | UNIVERSAL_OVERLOAD if charging else 0) \
            | (UNIVERSAL_UNLOCK_PIPE if _unlocking_pipe else 0)
    var buttons:Array[int] = [brake.universal_brake_button_1, brake.universal_brake_button_2, brake.universal_brake_button_3]
    # a vehicle whose buttons do nothing (no UBB1-3, most of them) is left alone: the Mover's flag
    # stays 0 and the releaser untouched whatever is pressed (Mover.cpp:3670-3689), and a command
    # renews the vehicle's whole state
    if not (buttons[0] or buttons[1] or buttons[2]):
        return
    for index:int in buttons.size():
        MaszynaDriverHints.send(vehicle, "universal_brake_button", index, bool(pressed & buttons[index]))


## trainbrakeapply's action (driverhints.cpp:810-825): the train brake applied to uncouple; the
## handle takes it on the next control(). The electro-pneumatic brake's own position is not ported
## (TODO.md).
func apply_train_brake() -> void:
    position = POSITION_UNCOUPLING


## trainbrakerelease's action (driverhints.cpp:796-800): the driver's own level to running - the
## handle follows it in the time controllers, never the handle itself
func release_train_brake() -> void:
    position = POSITION_RUNNING


## control_main_pipe() (Driver.cpp:8252-8261): the main pipe unlocked while the driver wants to go,
## let go once it is. Differs from the original, which holds the button with no hint, only to go and
## only once ready: the train brake handle at running cannot release on a locked, empty pipe - a
## player told to stand was sent round "release" and "neutral" for ever (SA132,
## MASZYNA_ORIGINAL_QUIRKS.md) - once the main reservoir has the air to fill it
func control_main_pipe(situation:MaszynaDriverTraction.Situation) -> void:
    var brake:MaszynaMoverRailVehicleBrake = _brake(situation.vehicle)
    if brake == null:
        return
    # only the cab that drives fills the pipe through its handle (IsCabMaster())
    var master:RailVehicleMasterController = MaszynaDriverTraction.master_controller(situation.vehicle)
    if not brake.get_main_pipe_locked():
        _unlocking_pipe = false
    elif master and master.get_cabin_controleable() and (situation.speed.acceleration_desired > PIPE_UNLOCK_ACCELERATION or absf(brake.get_controller_position()
                - brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_DRIVE)) <= MaszynaDriverHints.HANDLE_TOLERANCE) \
            and MaszynaDriverHints.is_done(situation, MaszynaDriverHints.Hint.WAIT_PRESSURE_TOO_LOW, 0.0):
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MAIN_PIPE_UNLOCK, 0.0, unlock_main_pipe)


## mainpipeunlock's action: the universal brake button that unlocks the main pipe held, from the
## next time controllers on, until the pipe is unlocked (control_main_pipe(), Driver.cpp:8255-8257)
func unlock_main_pipe() -> void:
    _unlocking_pipe = true


## The driver's own level taken from the handle where the handle stands at running, lap or charging
## ("sync virtual brake state with the 'real' one", Driver.cpp:2867-2878): the positions compared as
## whole numbers, the first of them that matches wins
func take_handle_position(vehicle:RID) -> void:
    var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle)
    if brake == null or brake.cntrl_brake_handle_type == MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_NO_HANDLE:
        return
    var handle:int = int(brake.get_controller_position())
    if handle == int(brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_DRIVE)):
        position = POSITION_RUNNING
    elif handle == int(brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_CUTOFF)):
        position = POSITION_LAP
    elif handle == int(brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_FILLING)):
        position = POSITION_CHARGING


## independentbrakerelease's action (driverhints.cpp:901-910): the local brake off
func release_local_brake(vehicle:RID, cabin:RID) -> void:
    _set_local_brake(vehicle, cabin, LOCAL_BRAKE_RELEASED)


## control_releaser() (Driver.cpp:8191-8250): the releaser held while the driver wants to go and
## the pipe is empty - the handle at the position that unlocks the pipe first, where the vehicle
## has one - or its own brake overcharged, or pressing the buffers - a locomotive whose control
## reservoir stayed fuller than the pipe keeps braking otherwise
func _control_releaser(situation:MaszynaDriverTraction.Situation, acceleration:float) -> void:
    var vehicle:RID = situation.vehicle
    var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle)
    var train_type:MaszynaMoverRailVehicleController.TrainType = (
            VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController).train_type
    if brake == null or not brake.cntrl_brake_system == RailVehicleBrake.BRAKE_SYSTEM_PNEUMATIC \
            or train_type == MaszynaMoverRailVehicleController.TRAIN_TYPE_EZT or train_type == MaszynaMoverRailVehicleController.TRAIN_TYPE_DMU \
            or not brake.cntrl_brake_handle_type in RELEASER_HANDLES:
        return
    # Differs from the original, which hints the releaser only to go: a pipe drained under the old
    # lock stayed empty with the handle at running, a player standing at a stop signal never told
    # why (reports#37). Hinted wherever the handle could fill it - at running or charging, in the
    # cab that drives (IsCabMaster()), the pipe not unlocked by the universal button
    var handle:float = brake.get_controller_position()
    var master:RailVehicleMasterController = MaszynaDriverTraction.master_controller(vehicle)
    if brake.cntrl_brake_handle_type == MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_FV4A \
            and brake.valve_type in PIPE_LOCK_VALVES and brake.get_pipe_pressure() < PIPE_LOCK_PRESSURE \
            and not _unlocking_pipe and master and master.get_cabin_controleable() \
            and (absf(handle - brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_DRIVE)) <= MaszynaDriverHints.HANDLE_TOLERANCE
                or absf(handle - brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_FILLING)) <= MaszynaDriverHints.HANDLE_TOLERANCE):
        _releasing_locked_pipe = true
        if not brake.get_releaser_active():
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_ZERO_SPEED)
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.RELEASER_ON)
        return
    # nothing done standing while told to wait (Driver.cpp:8193) - but the releaser hinted for the
    # lock let go once the pipe is past it: held, it empties the control reservoir (hamulce.cpp:1130)
    if VehicleServer.vehicle_get_speed(vehicle) < MaszynaDriverTrainset.MOVEMENT_SPEED \
            and situation.traction.action_time < 0.0:
        if _releasing_locked_pipe:
            if brake.get_releaser_active():
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.RELEASER_OFF)
            else:
                MaszynaDriverHints.withdraw(situation, MaszynaDriverHints.Hint.RELEASER_ON)
        _releasing_locked_pipe = false
        return
    _releasing_locked_pipe = false
    var pipe:float = brake.get_pipe_pressure()
    var actuate:bool = false
    var handle_unlocked:bool = true
    if acceleration > MaszynaDriverSpeed.NO_ACCELERATION:
        if pipe < EMPTY_PIPE_PRESSURE:
            actuate = true
            var unlock:float = brake.main_pipe_minimum_unblocking_handle_position
            if not unlock == MaszynaDriverHints.PIPE_UNLOCK_NONE:
                # trainbrakesetpipeunlock (driverhints.cpp:779-795): its level down to the position
                MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.TRAIN_BRAKE_SET_PIPE_UNLOCK, 0.0, func() -> void:
                    while position >= unlock and _add_position(-1.0):
                        pass)
                handle_unlocked = position == unlock
        if brake.get_air_pressure() > RELEASED_BRAKE_PRESSURE \
                and brake.get_control_reservoir_pressure() > CHARGED_CONTROL_RESERVOIR:
            actuate = true
        # its own brakes kept released while pressing the buffers (Driver.cpp:8224-8228)
        if situation.pressing and brake.get_air_pressure() > PRESSING_BRAKE_PRESSURE:
            actuate = true
    if pipe > OVERCHARGED_PIPE_PRESSURE:
        actuate = false
    var releasing:bool = brake.get_releaser_active()
    if actuate:
        # some vehicles take the releaser only with the master controller at zero
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.MASTER_CONTROLLER_SET_ZERO_SPEED)
        if not releasing and handle_unlocked:
            MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.RELEASER_ON)
    elif releasing:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.RELEASER_OFF)
    else:
        # Differs from the original, which keeps "Actuate" listed until the releaser is on: an
        # independent brake applied and let off before the start left it asking for the releaser
        # with the cylinders empty (SN61-02, 2026-10-07)
        MaszynaDriverHints.withdraw(situation, MaszynaDriverHints.Hint.RELEASER_ON)


## brakingforcesetzero's action (driverhints.cpp:863-872): DecBrake() until nothing is left to
## release
func _release(situation:MaszynaDriverTraction.Situation) -> void:
    while _decrease(situation, 1.0):
        pass


## IncBrake() (Driver.cpp:3014-3202); true when a control moved
func _increase(situation:MaszynaDriverTraction.Situation, brake_factor:float = 1.0) -> bool:
    var vehicle:RID = situation.vehicle
    var cabin:RID = situation.cabin
    var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle)
    if brake == null:
        return false
    var acceleration:float = situation.speed.acceleration_desired
    match brake.cntrl_brake_system:
        RailVehicleBrake.BRAKE_SYSTEM_INDIVIDUAL:
            if brake.cntrl_local_brake_type == RailVehicleBrake.LOCAL_BRAKE_TYPE_MANUAL:
                return _step_manual_brake(vehicle, 1 + floori(0.5 + absf(acceleration)))
            return _step_local_brake(vehicle, cabin, floori(1.5 + absf(acceleration)))
        RailVehicleBrake.BRAKE_SYSTEM_PNEUMATIC:
            var trainset:MaszynaDriverTrainset = situation.trainset
            var moved:bool = false
            if _is_standalone(situation):
                # hamowanie lokalnym bo luzem jedzie - a trainset of engines alone brakes with its own brake
                if not MaszynaDriverTraction.eim_control_type(situation) == MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_0:
                    moved = _increase_eim(situation)
                else:
                    moved = _step_local_brake(vehicle, situation.cabin, 1 + floori(0.5 + absf(acceleration)))
            elif position + 1.0 == POSITION_MAX:
                if acceleration < EMERGENCY_ACCELERATION:
                    moved = _add_position(1.0)
            else:
                # the wagons whose control reservoir is overcharged need the pipe lower (Driver.cpp:3107-3124)
                var correction:float = 0.0
                for other:RID in trainset.vehicles:
                    var other_brake:MaszynaMoverRailVehicleBrake = _brake(other)
                    var control_reservoir:float = other_brake.get_control_reservoir_pressure() if other_brake else 0.0
                    if not (other_brake and other_brake.is_cut_off()):
                        correction -= (minf(FULL_CONTROL_RESERVOIR, control_reservoir) - FULL_CONTROL_RESERVOIR) \
                                * VehicleServer.vehicle_get_controller(other).get_mass_total()
                correction = correction / trainset.mass * POSITION_CORRECTION_SCALE if trainset.mass > 0.0 else 0.0
                if brake.cntrl_brake_handle_type == MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_FV4A:
                    correction += brake.get_handle_control_pressure() * FV4A_CONTROL_PRESSURE_SHARE
                var excess:float = -acceleration * brake_factor - (table_a0 + TABLE_STEPS * (position - 1.0 - correction) * table_a1)
                if excess > table_a1:
                    if position < BRAKING_FROM:
                        # BrakingInitialLevel (Driver.cpp:2306-2309)
                        moved = _add_position(CARGO_INITIAL_LEVEL if RailVehicleServer.trainset_get_type(vehicle) \
                                == RailVehicleServer.TRAINSET_TYPE_CARGO else BRAKING_INITIAL_LEVEL)
                        # stronger braking to overcome SA134's engine (Driver.cpp:3137-3144)
                        var route:MaszynaDriverRoute = situation.route
                        if _dmu and situation.speed.velocity_next == 0.0 and route.brake_distance < DMU_STOP_DISTANCE:
                            _add_position(DMU_HALF_STEP if route.brake_distance / situation.speed.proximity_distance < DMU_EARLY_SHARE
                                    else 1.0)
                    else:
                        moved = _add_position(BRAKING_LEVEL_INCREASE)
                        if excess > 2.0 * table_a1 and position + BRAKING_LEVEL_INCREASE <= DEEPEST_DOUBLE_STEP:
                            _add_position(BRAKING_LEVEL_INCREASE)
            # braking, the releaser is let go (Driver.cpp:3154-3158)
            if position > POSITION_RUNNING and brake.get_releaser_active():
                MaszynaDriverHints.send(vehicle, "brake_releaser", false)
            return moved
        RailVehicleBrake.BRAKE_SYSTEM_ELECTRO_PNEUMATIC:
            # its highest operation mode; an induction motor's by its EIM controller or its EN57
            # handle, else the handle between EP releasing and EP braking by the share of the
            # deceleration wanted, or, applied by time, held at EP braking (Driver.cpp:3161-3198)
            var mode:int = brake.get_operation_mode()
            while mode << 1 <= brake.cntrl_brake_op_modes:
                MaszynaDriverHints.send(vehicle, "brake_operation_mode_increase")
                var raised:int = brake.get_operation_mode()
                if raised == mode:
                    break
                mode = raised
            var handle:float = brake.get_controller_position()
            if _induction(vehicle):
                if brake.cntrl_brake_handle_type == MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_MHZ_EN57:
                    if handle < brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_FULL):
                        return _set_handle(vehicle, situation.cabin, handle + 1.0)
                    return false
                return _increase_eim(situation)
            var release:float = brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_EP_RELEASE)
            var braking:float = brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_EP_BRAKE)
            if not brake.get_handle_ep_time_controlled():
                return _set_handle(vehicle, situation.cabin, lerpf(release, braking, _ep_share(situation)))
            if not _set_handle(vehicle, situation.cabin, braking):
                return false
            if release - brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_EP_HOLD) < EP_SWITCHED:
                MaszynaDriverHints.send(vehicle, "ep_brake", true)
            return true
    return false


## DecBrake() (Driver.cpp:3254-3342); true when a control moved
func _decrease(situation:MaszynaDriverTraction.Situation, brake_factor:float) -> bool:
    var vehicle:RID = situation.vehicle
    var cabin:RID = situation.cabin
    var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle)
    if brake == null:
        return false
    var acceleration:float = situation.speed.acceleration_desired
    match brake.cntrl_brake_system:
        RailVehicleBrake.BRAKE_SYSTEM_INDIVIDUAL:
            var steps:int = 1 + floori(0.5 + absf(acceleration))
            if brake.cntrl_local_brake_type == RailVehicleBrake.LOCAL_BRAKE_TYPE_MANUAL:
                return _step_manual_brake(vehicle, -steps)
            return _step_local_brake(vehicle, cabin, -steps)
        RailVehicleBrake.BRAKE_SYSTEM_PNEUMATIC:
            # without a braking table it always releases
            var shortfall:float = -1.0
            if not table_a0 == 0.0 or not table_a1 == 0.0:
                var step:float = DMU_POSITION_STEP if _dmu else 1.0
                shortfall = -acceleration * brake_factor - (table_a0 + TABLE_STEPS * (position - step) * table_a1)
            var moved:bool = false
            if shortfall < 0.0 and position > POSITION_RUNNING:
                moved = _add_position(RELEASE_STEP)
                if position < RUNNING_RETURN_POSITION:
                    position = POSITION_RUNNING
            if not moved:
                moved = _step_local_brake(vehicle, cabin, -LOCAL_RELEASE_STEPS)
            if not moved:
                moved = _decrease_eim(situation)
            return moved
        RailVehicleBrake.BRAKE_SYSTEM_ELECTRO_PNEUMATIC:
            # an induction motor's by its EN57 handle or its EIM controller, else the handle between
            # EP releasing and braking, or, applied by time, back to EP releasing (Driver.cpp:3301-3335)
            var handle:float = brake.get_controller_position()
            var release:float = brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_EP_RELEASE)
            var moved:bool = false
            if _induction(vehicle):
                if brake.cntrl_brake_handle_type == MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_MHZ_EN57:
                    moved = handle > brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_DRIVE) \
                            and _set_handle(vehicle, cabin, handle - 1.0)
                else:
                    moved = _decrease_eim(situation)
            elif not brake.get_handle_ep_time_controlled():
                moved = _set_handle(vehicle, cabin, lerpf(
                        release, brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_EP_BRAKE), _ep_share(situation)))
            else:
                moved = _set_handle(vehicle, cabin, release)
                # the original switches the EP brake on here too (Driver.cpp:3326-3328)
                if release - brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_EP_HOLD) < EP_SWITCHED:
                    moved = bool(MaszynaDriverHints.send(vehicle, "ep_brake", true)) or moved
            if not moved:
                moved = _step_local_brake(vehicle, cabin, -LOCAL_RELEASE_STEPS)
            return moved
    return false


## The share of the EP brake the deceleration wanted takes (Driver.cpp:3178-3184, 3315-3321)
func _ep_share(situation:MaszynaDriverTraction.Situation) -> float:
    var vehicle:RID = situation.vehicle
    var acceleration_max:float = minf(table_a0 + TABLE_FULL_STEPS * table_a1, _med_max_deceleration(vehicle))
    if acceleration_max == 0.0:
        return 0.0
    return clampf(-situation.speed.acceleration_desired / acceleration_max * _local_brake_factor(vehicle), 0.0, 1.0)


## IncBrakeEIM() (Driver.cpp:3204-3252): braking by the EIM controller's kind - the local brake's
## share of the deceleration wanted, the controller to its braking position, or the local brake a
## step; true when a control moved
func _increase_eim(situation:MaszynaDriverTraction.Situation) -> bool:
    var vehicle:RID = situation.vehicle
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(vehicle, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    var main:int = MaszynaDriverTraction.main_controller_position(situation)
    match MaszynaDriverTraction.eim_control_type(situation):
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_0:
            if _med_max_deceleration(vehicle) == NO_MED_DECELERATION:
                return _step_local_brake(vehicle, situation.cabin, 1)
            var acceleration:float = situation.speed.acceleration_desired
            var configuration:MaszynaMoverEngineConfiguration = MaszynaDriverTraction.engine_configuration(engine)
            var most:float = EMERGENCY_LOCAL_BRAKE if configuration and configuration.cntrl_eim_control_emergency else 1.0
            var limit:float = -EIM_BRAKE_LIMIT_FACTOR * acceleration / EIM_MAX_DECELERATION - 1.0
            var hinted:float = -_local_brake_factor(vehicle) * acceleration / EIM_MAX_DECELERATION
            return _set_local_brake(vehicle, situation.cabin, most * clampf(maxf(limit, hinted), 0.0, 1.0))
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_1:
            if main > 0:
                return MaszynaDriverTraction.put_main_controller(situation, 0)
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_2:
            if main > 1:
                return MaszynaDriverTraction.put_main_controller(situation, 1)
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_3:
            var controller:RailVehicleUniversalController = RailVehicleServer.vehicle_component_get(
                    vehicle, RailVehicleComponentType.COMPONENT_UNIVERSAL_CONTROLLER) as RailVehicleUniversalController
            if controller and controller.integrated_local_brake:
                return main > 0 and MaszynaDriverTraction.put_main_controller(situation, 0)
            return _step_local_brake(vehicle, situation.cabin, 1)
    return false


## DecBrakeEIM() (Driver.cpp:3367-3404): braking off by the EIM controller's kind
func _decrease_eim(situation:MaszynaDriverTraction.Situation) -> bool:
    var vehicle:RID = situation.vehicle
    var main:int = MaszynaDriverTraction.main_controller_position(situation)
    match MaszynaDriverTraction.eim_control_type(situation):
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_0:
            if _med_max_deceleration(vehicle) == NO_MED_DECELERATION:
                return _step_local_brake(vehicle, situation.cabin, -1)
            var acceleration:float = situation.speed.acceleration_desired
            if VehicleServer.vehicle_get_speed(vehicle) <= MaszynaDriverTrainset.NO_MOVEMENT_SPEED:
                acceleration = maxf(0.0, acceleration)
            return _set_local_brake(vehicle, situation.cabin, clampf(
                    -_local_brake_factor(vehicle) * acceleration / _med_max_deceleration(vehicle), 0.0, 1.0))
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_1:
            if main < EIM_TRAXX_BRAKE_OFF:
                return MaszynaDriverTraction.put_main_controller(situation, EIM_TRAXX_BRAKE_OFF)
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_2:
            if main < EIM_ELF_BRAKE_OFF:
                return MaszynaDriverTraction.put_main_controller(situation, EIM_ELF_BRAKE_OFF)
        MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_3:
            var master_controller:RailVehicleMasterController = RailVehicleServer.vehicle_component_get(
                    situation.controlling, RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController
            var neutral:int = master_controller.direction_change_max_position if master_controller else 0
            if main < neutral:
                return MaszynaDriverTraction.put_main_controller(situation, neutral)
    return false


## The driver's position to an FV4a as it is, and to MHZ_K8P and MHZ_EN57 by their table
## (SetTimeControllers() 1., Driver.cpp:4068-4092)
func _apply_handle(vehicle:RID, cabin:RID, brake:MaszynaMoverRailVehicleBrake) -> void:
    match brake.cntrl_brake_handle_type:
        MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_FV4A:
            _set_handle(vehicle, cabin, position)
        MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_MHZ_K8P, MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_MHZ_EN57:
            var handle_position:float
            if position == POSITION_RUNNING:
                handle_position = brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_DRIVE)
            elif position == POSITION_CHARGING:
                handle_position = brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_FILLING)
            elif position == POSITION_LAP:
                handle_position = brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_CUTOFF)
            elif position > K8P_FULL_FROM:
                handle_position = K8P_FULL_POSITION
            elif position > K8P_STRONG_FROM:
                handle_position = K8P_STRONG_POSITION
            else:
                handle_position = roundf((position * K8P_SCALE - K8P_OFFSET) / K8P_STEP)
            _set_handle(vehicle, cabin, handle_position)


## A DMU's universal controller not braking (Check/SetTimeControllers() 5.1, Driver.cpp:4188-4190,
## 4303-4321): held at its position that neither adds nor takes power while the share wanted is
## reached, and the train brake handle at the pneumatic position of the controller's position
func _hold_universal_controller(situation:MaszynaDriverTraction.Situation) -> void:
    if position >= BRAKING_FROM or not situation.controlling.is_valid() \
            or not MaszynaDriverTraction.eim_control_type(situation) == MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_3:
        return
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
            situation.controlling, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    var controller:MaszynaMoverRailVehicleUniversalController = RailVehicleServer.vehicle_component_get(
            situation.controlling, RailVehicleComponentType.COMPONENT_UNIVERSAL_CONTROLLER) as MaszynaMoverRailVehicleUniversalController
    if not (engine and engine.get_type() == RailVehicleEngine.DIESEL) or controller == null:
        return
    var positions:Array = controller.positions
    var main:int = MaszynaDriverTraction.main_controller_position(situation)
    if main < positions.size():
        _set_handle(situation.vehicle, situation.cabin, float((positions[main] as MaszynaMoverUniversalControllerListItem).pneumatic_brake_position))


## BrakeLevelAdd() (Driver.cpp:3836): false once it would leave the range
func _add_position(change:float) -> bool:
    position = clampf(position + change, POSITION_MIN, POSITION_MAX)
    return position < POSITION_MAX if change > 0.0 else position > POSITION_CHARGING


## Whether the trainset brakes with the local brake (IncBrake(), Driver.cpp:3036-3087): an ET41 or
## ET42 not coupled to another unit, never a DMU; loose shunting near the speed wanted; otherwise
## only engines coupled to be driven together
func _is_standalone(situation:MaszynaDriverTraction.Situation) -> bool:
    var vehicle:RID = situation.vehicle
    var train_type:MaszynaMoverRailVehicleController.TrainType = (
            VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController).train_type
    var trainset:MaszynaDriverTrainset = situation.trainset
    if train_type == MaszynaMoverRailVehicleController.TRAIN_TYPE_ET41 or train_type == MaszynaMoverRailVehicleController.TRAIN_TYPE_ET42:
        # a unit of two joined for good, with nothing beyond it
        return RailVehicleServer.vehicle_get_coupled(
                vehicle, RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_PERMANENT).size() \
                == trainset.vehicles.size()
    if _dmu:
        return false
    if situation.order & MaszynaAIDriver.Order.LOOSE_SHUNT \
            and VehicleServer.vehicle_get_speed(vehicle) - situation.speed.velocity_desired \
                < situation.route.velocity_plus + situation.route.velocity_minus:
        return true
    var controlled:Array[RID] = RailVehicleServer.vehicle_get_coupled(
            vehicle, RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_CONTROL)
    if not controlled.size() == trainset.vehicles.size():
        return false
    for other:RID in trainset.vehicles:
        if VehicleServer.vehicle_get_controller(other).power <= POWERED:
            return false
    return true


## apply_independent_brake_only() (Driver.cpp:8178-8189): the local brake on if the train brake
## runs, otherwise the train brake to running first; not with a manual brake, nor in shunting mode.
## Nor without a local brake at all - the original lets that through (only ManualBrake is refused)
## and hints a brake an EN57's motor car does not have, a hint never done after
func apply_independent_brake_only(situation:MaszynaDriverTraction.Situation) -> void:
    var vehicle:RID = situation.vehicle
    var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle)
    if brake == null or brake.cntrl_local_brake_type in [RailVehicleBrake.LOCAL_BRAKE_TYPE_MANUAL,
            RailVehicleBrake.LOCAL_BRAKE_TYPE_NONE]:
        return
    var running:float = brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_DRIVE)
    if absf(brake.get_controller_position() - running) <= HANDLE_TOLERANCE:
        # independentbrakeapply (driverhints.cpp:888-899): an emergency EIM controller one short
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.INDEPENDENT_BRAKE_APPLY, 0.0, func() -> void:
            var configuration:MaszynaMoverEngineConfiguration = MaszynaDriverTraction.engine_configuration(
                    VehicleServer.vehicle_component_get(vehicle, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine)
            var most:float = (LOCAL_BRAKE_POSITIONS - 1.0) / LOCAL_BRAKE_POSITIONS \
                    if configuration and configuration.cntrl_eim_control_emergency else LOCAL_BRAKE_APPLIED
            if brake.get_local_position_normalized() < most:
                _set_local_brake(vehicle, situation.cabin, most))
    else:
        MaszynaDriverHints.cue(situation, MaszynaDriverHints.Hint.TRAIN_BRAKE_RELEASE, 0.0, release_train_brake)


## IncLocalBrakeLevel()/DecLocalBrakeLevel() through the knob; true when it moved
func _step_local_brake(vehicle:RID, cabin:RID, steps:int) -> bool:
    var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle)
    var current:float = brake.get_local_position_normalized() if brake else 0.0
    return _set_local_brake(vehicle, cabin, clampf(current + steps / LOCAL_BRAKE_POSITIONS, LOCAL_BRAKE_RELEASED, LOCAL_BRAKE_APPLIED))


## The local brake knob set; true when it moved
func _set_local_brake(vehicle:RID, cabin:RID, value:float) -> bool:
    var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle)
    if (brake.get_local_position_normalized() if brake else 0.0) == value:
        return false
    CabinSystem.act(cabin, LOCAL_BRAKE, &"set", value)
    return true


## IncManualBrakeLevel()/DecManualBrakeLevel(): the hand brake's wheel turned `steps`; true when it
## moved
func _step_manual_brake(vehicle:RID, steps:int) -> bool:
    var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle)
    if brake == null:
        return false
    var before:int = brake.get_manual_position()
    for _step:int in absi(steps):
        MaszynaDriverHints.send(vehicle, "manual_brake_increase" if steps > 0 else "manual_brake_decrease")
    return not brake.get_manual_position() == before


## The train brake handle put at `handle_position` (the vehicle's own scale); true when it moved
func _set_handle(vehicle:RID, cabin:RID, handle_position:float) -> bool:
    var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle)
    if brake == null or brake.get_controller_position() == handle_position:
        return false
    var low:float = brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_MIN)
    var high:float = brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_MAX)
    if high <= low:
        return false
    CabinSystem.act(cabin, TRAIN_BRAKE, &"set", (handle_position - low) / (high - low))
    return true


## The brake's delay [s] at the setting in use: the one read past G, or at G (Driver.cpp:8124, 8146)
func _brake_delay(vehicle:RID, past_g:int, at_g:int) -> float:
    var brake:MaszynaMoverRailVehicleBrake = _brake(vehicle)
    if brake == null:
        return 0.0
    # BDelay1-4, per delay setting
    var delays:PackedFloat64Array = PackedFloat64Array([
        brake.cntrl_brake_delay_1, brake.cntrl_brake_delay_2, brake.cntrl_brake_delay_3, brake.cntrl_brake_delay_4])
    if delays.size() <= maxi(past_g, at_g):
        return 0.0
    var setting:int = brake.get_delay_setting()
    return delays[past_g] if setting > DELAY_SETTING_G else delays[at_g]


## The Mover's brake, whose model (its delay setting, its operation mode) the driver reads
static func _brake(vehicle:RID) -> MaszynaMoverRailVehicleBrake:
    return RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_BRAKES) as MaszynaMoverRailVehicleBrake


## is_emu() (Driver.h:259)
static func is_emu(vehicle:RID) -> bool:
    return (VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController).train_type \
            == MaszynaMoverRailVehicleController.TRAIN_TYPE_EZT


static func _induction(vehicle:RID) -> bool:
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    return engine != null and engine.get_type() == RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR


## MED_amax: the service deceleration of the blended EP and ED brake [m/s2]
static func _med_max_deceleration(vehicle:RID) -> float:
    var brake:MaszynaMoverRailVehicleElectroPneumaticDynamicBrake = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_EP_ED_BRAKE) as MaszynaMoverRailVehicleElectroPneumaticDynamicBrake
    return brake.blending_max_deceleration if brake else NO_MED_DECELERATION


## AIHintLocalBrakeAccFactor
static func _local_brake_factor(vehicle:RID) -> float:
    var hints:RailVehicleAIHints = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_AI_HINTS) as RailVehicleAIHints
    return hints.local_brake_acceleration_factor if hints else DEFAULT_LOCAL_BRAKE_FACTOR
