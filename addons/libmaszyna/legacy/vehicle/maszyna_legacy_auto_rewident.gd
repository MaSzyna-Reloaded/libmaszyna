extends Node
class_name MaszynaLegacyAutoRewident

## Automatic trainset inspection of a vehicle's driver - the original TController::AutoRewident()
## (Driver.cpp:2147-2250), for every vehicle MaszynaLegacyVehicleSystem builds: a vehicle placed
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

## bdelay_* brake delay flags (hamulce.h:49-51)
const BDELAY_G:int = 1
const BDELAY_P:int = 2
const BDELAY_R:int = 4
## AutoRewident's "passenger train" marker added to the chosen setting
const PASSENGER_TRAIN:int = 16
## Main reservoir pipe pressure PrepareEngine() waits for (Driver.cpp, isready)
const READY_FEED_PIPE_PRESSURE:float = 4.5
## AutoRewident()'s limits of a train's length [m] and mass [kg], and the vehicles next to the
## locomotive kept on G (Driver.cpp:2190-2230)
const SHORT_TRAIN_LENGTH:float = 300.0
const SHORT_TRAIN_MASS:float = 600000.0
const MEDIUM_TRAIN_LENGTH:float = 500.0
const MEDIUM_TRAIN_MASS:float = 1300000.0
const VEHICLES_ON_G_NEXT_TO_LOCOMOTIVE:int = 5
const MIXED_TRAIN_FREIGHT_LIMIT:int = 4

## In the original this is not polled at all: AutoRewident() runs inside CheckVehicles()
## (Driver.cpp:2528) for the driving orders, and CheckVehicles() itself is called on events - an
## order change, PrepareEngine() completing (Driver.cpp:2142), a direction change, a coupling
## change (Driver.cpp:2622).
##
## The only event here is RailVehicleServer's vehicle_trainset_changed. What is left to watch is
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
## neighbours report a trainset change (MoverRailVehicleController::release())
func _enter_tree() -> void:
    RailVehicleServer.vehicle_trainset_changed.connect(_on_vehicle_trainset_changed)


func _exit_tree() -> void:
    RailVehicleServer.vehicle_trainset_changed.disconnect(_on_vehicle_trainset_changed)


## The vehicle's trainset is inspected once its engine is ready, and after every trainset change
func vehicle_follow(vehicle:RID) -> void:
    _vehicles[vehicle] = true
    _on_vehicle_trainset_changed(vehicle)


func vehicle_unfollow(vehicle:RID) -> void:
    _vehicles.erase(vehicle)
    _waiting.erase(vehicle)
    if not _waiting:
        _timer.stop()


## A vehicle joined or left the trainset (VehicleController::couple()/uncouple()), the case the
## original handles with CheckVehicles() (Driver.cpp:2622) - inspect it again once the engine of
## the new trainset reports ready.
func _on_vehicle_trainset_changed(vehicle:RID) -> void:
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
        _rewident(vehicle, _get_trainset(vehicle))
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


## TController::AutoRewident() (Driver.cpp:2147-2246). The driver's own vehicle is left alone, as the
## original does with a human controlled vehicle.
func _rewident(vehicle:RID, trainset:Array[RID]) -> void:
    var express:int = 0
    var freight:int = 0
    var passenger:int = 0
    var length:float = 0.0
    var mass:float = 0.0
    for member:RID in trainset:
        var member_controller:VehicleController = VehicleServer.vehicle_get_controller(member)
        length += member_controller.dimensions_length
        mass += member_controller.get_mass_total()
        if member_controller.power < 1.0:
            var member_brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
                    member, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
            var delays:int = member_brake.cntrl_brake_delays if member_brake else 0
            if delays & BDELAY_R:
                express += 1
            elif delays & BDELAY_G:
                freight += 1
            else:
                passenger += 1

    var setting:int
    if express + freight + passenger == 0:
        setting = PASSENGER_TRAIN + BDELAY_R # light engine
    elif freight < mini(MIXED_TRAIN_FREIGHT_LIMIT, express + passenger):
        setting = PASSENGER_TRAIN + (BDELAY_P if freight and express < freight + passenger else BDELAY_R)
    elif length < SHORT_TRAIN_LENGTH and mass < SHORT_TRAIN_MASS:
        setting = BDELAY_P
    elif length < MEDIUM_TRAIN_LENGTH and mass < MEDIUM_TRAIN_MASS:
        setting = BDELAY_R
    else:
        setting = BDELAY_G

    var near_locomotive:int = 0
    for member:RID in trainset:
        var member_brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
                member, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
        var is_locomotive:bool = VehicleServer.vehicle_get_controller(member).power > 1.0
        var delays:int = member_brake.cntrl_brake_delays if member_brake else 0
        var brake_delay:int = BDELAY_P
        match setting:
            BDELAY_P:
                # freight P - locomotive on G, the rest on P
                brake_delay = BDELAY_G if is_locomotive else BDELAY_P
            BDELAY_G:
                # freight G - everything on G, P without it
                brake_delay = BDELAY_G if delays & BDELAY_G else BDELAY_P
            BDELAY_R:
                # freight GP - locomotive and the vehicles next to it on G, the rest on P
                if is_locomotive:
                    brake_delay = BDELAY_G
                    near_locomotive = 0
                else:
                    near_locomotive += 1
                    brake_delay = BDELAY_G if near_locomotive <= VEHICLES_ON_G_NEXT_TO_LOCOMOTIVE else BDELAY_P
            PASSENGER_TRAIN + BDELAY_R:
                # passenger R - R, P without it
                brake_delay = BDELAY_R if delays & BDELAY_R else BDELAY_P
            PASSENGER_TRAIN + BDELAY_P:
                brake_delay = BDELAY_P
        if not member == vehicle:
            VehicleServer.vehicle_send_command(member, "auto_rewident", brake_delay)
