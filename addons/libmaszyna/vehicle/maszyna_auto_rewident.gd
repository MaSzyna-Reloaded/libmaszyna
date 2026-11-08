extends Node
class_name MaszynaAutoRewident

## Automatic trainset inspection of a vehicle's driver - the original TController::AutoRewident()
## (Driver.cpp:2147-2250), for every vehicle MaszynaVehicleSystem builds: a vehicle placed
## with a driver (headdriver/reardriver) prepares its train once its engine is ready, and again
## after every trainset change, setting the brake delay (G/P/R) of every vehicle for the kind of
## train and releasing their manual and spring brakes.
##
## In the original that happens also with a human driver: the scenery gives the driver the
## Prepare_engine order and then Shunt/Obey_train (OrdersInit), PrepareEngine() completes once the
## engine is ready, and CheckVehicles() calls AutoRewident() in those orders (Driver.cpp:2527-2530) -
## trainset changes call CheckVehicles() as well (Train.cpp:6224, 6246). Vehicles placed without a
## speed start braked with a full manual brake (CheckLocomotiveParameters, Mover.cpp:8946), so
## without this the wagons of such a trainset never get released.

## Main reservoir pipe pressure PrepareEngine() waits for (Driver.cpp, isready)
const READY_FEED_PIPE_PRESSURE:float = 4.5
## AutoRewident()'s limits of a train's length [m] and mass [kg], and the vehicles next to the
## locomotive kept on G (Driver.cpp:2190-2230)
const SHORT_TRAIN_LENGTH:float = 300.0
const SHORT_TRAIN_MASS:float = 600000.0
const MEDIUM_TRAIN_LENGTH:float = 500.0
const MEDIUM_TRAIN_MASS:float = 1300000.0
const VEHICLES_ON_G_NEXT_TO_LOCOMOTIVE:int = 5
## Above this power a vehicle is a locomotive, below it a wagon (Power, Driver.cpp:2156)
const LOCOMOTIVE_POWER:float = 1.0
## The original's setting of a train's brakes (ustaw, Driver.cpp:2166-2186): a passenger train on
## R or P, a goods train on P, GP or G
enum Setting { PASSENGER_R, PASSENGER_P, GOODS_P, GOODS_GP, GOODS_G }

## In the original this is not polled at all: AutoRewident() runs inside CheckVehicles()
## (Driver.cpp:2528) for the driving orders, and CheckVehicles() itself is called on events - an
## order change, PrepareEngine() completing (Driver.cpp:2142), a direction change, a coupling
## change (Driver.cpp:2622).
##
## The only event here is RailVehicleServer's trainset_vehicles_changed. What is left to watch is
## the engine becoming ready, a threshold the original's AI watches in its own update too - one
## timer for every vehicle still waiting for it, stopped as soon as none is.
const CHECK_INTERVAL:float = 0.5

## The vehicles followed, and of them the ones whose engine is still to become ready
var _vehicles:Dictionary[RID, bool] = {}
var _waiting:Array[RID] = []
var _timer:Timer = Timer.new()


func _ready() -> void:
    _timer.wait_time = CHECK_INTERVAL
    _timer.timeout.connect(_check_waiting)
    add_child(_timer)


## Followed only in the tree, where the timer runs: a scenery freed on quitting (its
## NOTIFICATION_PREDELETE, after the autoloads have left) uncouples its vehicles, and their
## neighbours report a trainset change (MaszynaMoverRailVehicleController::release())
func _enter_tree() -> void:
    RailVehicleServer.trainset_vehicles_changed.connect(_on_trainset_vehicles_changed)


func _exit_tree() -> void:
    RailVehicleServer.trainset_vehicles_changed.disconnect(_on_trainset_vehicles_changed)


## The vehicle's trainset is inspected once its engine is ready, and after every trainset change
func vehicle_follow(vehicle:RID) -> void:
    _vehicles[vehicle] = true
    _watch(vehicle)


func vehicle_unfollow(vehicle:RID) -> void:
    _vehicles.erase(vehicle)
    _waiting.erase(vehicle)
    if not _waiting:
        _timer.stop()


## Vehicles joined or left a trainset (coupled, uncoupled), the case the original handles with
## CheckVehicles() (Driver.cpp:2622) - each followed vehicle of it inspects it again once its engine
## reports ready.
func _on_trainset_vehicles_changed(trainset:RID) -> void:
    for vehicle:RID in RailVehicleServer.trainset_get_vehicles(trainset):
        _watch(vehicle)


## The followed vehicle's trainset inspected once its engine reports ready
func _watch(vehicle:RID) -> void:
    if not _vehicles.has(vehicle) or _waiting.has(vehicle):
        return
    _waiting.append(vehicle)
    if _waiting.size() == 1:
        _timer.start()


func _check_waiting() -> void:
    for vehicle:RID in _waiting.duplicate():
        if not VehicleServer.vehicle_is_simulation_ready(vehicle):
            continue
        # a vehicle nobody drives has no driver to inspect its trainset, and the scenery seats its
        # driver before the simulation is ready - so there is nothing left to watch
        if not VehicleServer.vehicle_has_person_role(vehicle, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER):
            _waiting.erase(vehicle)
            continue
        # PrepareEngine() completes once the engine reports ready
        if not _is_engine_ready(vehicle):
            continue
        set_brake_delays(vehicle, _get_trainset(vehicle), false)
        # from here the trainset can only change by coupling, and that arrives as a signal
        _waiting.erase(vehicle)
    if not _waiting:
        _timer.stop()


## The readiness condition of TController::PrepareEngine() (isready). Quirk: the converter and
## compressor terms are left out - the wrapper doesn't expose whether a vehicle has them.
func _is_engine_ready(vehicle:RID) -> bool:
    var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    var brake_handle_ready:bool = (
            not brake
            or not int(brake.get_controller_position())
                    == int(brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_CUTOFF)))
    return (
            not VehicleServer.vehicle_get_controller(vehicle).get_direction() == VehicleController.DIRECTION_NEUTRAL
            and engine and engine.get_main_switch_enabled()
            and (not brake or brake.get_feed_pipe_pressure() > READY_FEED_PIPE_PRESSURE
                    or is_zero_approx(brake.tank_volume_main))
            and brake_handle_ready)


## Coupled vehicles from the head of the train (in the driving direction, CheckVehicles()) to its tail.
func _get_trainset(vehicle:RID) -> Array[RID]:
    # the cab driven from, as the original numbers it (CabOccupied): the front 1, the rear -1, the
    # machine room or none 0 (Train.cpp:8684)
    var cab:int = 0
    match RailVehicleServer.cabin_get_kind(RailVehicleServer.vehicle_get_driver_cabin(vehicle)):
        RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT:
            cab = 1
        RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR:
            cab = -1
    var driving_sign:int = cab * VehicleServer.vehicle_get_controller(vehicle).get_direction()
    var trainset:Array[RID] = []
    trainset.assign(RailVehicleServer.vehicle_get_coupled(
            vehicle, RailVehicleController.COUPLER_END_FRONT if driving_sign >= 0 else RailVehicleController.COUPLER_END_REAR,
            RailVehicleController.COUPLING_FLAG_COUPLER))
    return trainset


## TController::AutoRewident() (Driver.cpp:2146-2246): the brake setting the train needs - a passenger
## train P or R, a goods train P, GP or G by its length and mass - put on every vehicle of `trainset`,
## from the head of the train in the driving direction, by the crew (`auto_rewident`, the rewident's
## own lever); the driver's own vehicle only while the driver drives it (AIControllFlag), as the
## original leaves a human controlled one alone. A passenger or a goods train by the trainset's type
## (RailVehicleServer.trainset_get_type()). True for a passenger train.
static func set_brake_delays(vehicle:RID, trainset:Array[RID], in_control:bool) -> bool:
    var fast:int = 0
    var goods:int = 0
    var passengers:int = 0
    var length:float = 0.0
    var mass:float = 0.0
    for member:RID in trainset:
        var controller:VehicleController = VehicleServer.vehicle_get_controller(member)
        length += controller.dimensions_length
        mass += controller.get_mass_total()
        if controller.power >= LOCOMOTIVE_POWER:
            continue
        if _takes_delay(member, RailVehicleBrake.BRAKE_DELAY_R):
            fast += 1
        elif _takes_delay(member, RailVehicleBrake.BRAKE_DELAY_G):
            goods += 1
        else:
            passengers += 1
    var passenger:bool = not RailVehicleServer.trainset_get_type(RailVehicleServer.vehicle_get_trainset(vehicle)) \
            == RailVehicleServer.TRAINSET_TYPE_CARGO
    # the original's ustaw (Driver.cpp:2166-2186): a light engine is a passenger train on R
    var setting:Setting
    if not fast + goods + passengers or passenger and not (goods and fast < goods + passengers):
        setting = Setting.PASSENGER_R
    elif passenger:
        setting = Setting.PASSENGER_P
    elif length < SHORT_TRAIN_LENGTH and mass < SHORT_TRAIN_MASS:
        setting = Setting.GOODS_P
    elif length < MEDIUM_TRAIN_LENGTH and mass < MEDIUM_TRAIN_MASS:
        setting = Setting.GOODS_GP
    else:
        setting = Setting.GOODS_G
    var near_locomotive:int = 0
    for member:RID in trainset:
        # the driver's own vehicle only while it drives it, and only a vehicle with a brake to set
        if member == vehicle and not in_control or not RailVehicleServer.vehicle_component_get(
                member, RailVehicleComponentType.COMPONENT_BRAKES):
            continue
        var locomotive:bool = VehicleServer.vehicle_get_controller(member).power > LOCOMOTIVE_POWER
        var brake_delay:int = RailVehicleBrake.BRAKE_DELAY_P
        match setting:
            Setting.PASSENGER_R:
                # R, P without it
                if _takes_delay(member, RailVehicleBrake.BRAKE_DELAY_R):
                    brake_delay = RailVehicleBrake.BRAKE_DELAY_R
            Setting.GOODS_P:
                # the locomotive on G, the rest on P
                if locomotive:
                    brake_delay = RailVehicleBrake.BRAKE_DELAY_G
            Setting.GOODS_GP:
                # the locomotive and the vehicles next to it on G, the rest on P
                near_locomotive = 0 if locomotive else near_locomotive + 1
                if near_locomotive <= VEHICLES_ON_G_NEXT_TO_LOCOMOTIVE:
                    brake_delay = RailVehicleBrake.BRAKE_DELAY_G
            Setting.GOODS_G:
                # everything on G, P without it
                if _takes_delay(member, RailVehicleBrake.BRAKE_DELAY_G):
                    brake_delay = RailVehicleBrake.BRAKE_DELAY_G
        VehicleServer.vehicle_send_command(member, "auto_rewident", brake_delay)
    return setting == Setting.PASSENGER_R or setting == Setting.PASSENGER_P


## Whether the vehicle's brake can be set to `delay` (BrakeDelays); never without a brake
static func _takes_delay(vehicle:RID, delay:int) -> bool:
    var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    return brake and brake.cntrl_brake_delays & delay
