extends MaszynaStartupTest
class_name MaszynaBrakingTest

## A vehicle of a scenery fixture started and moved off as MaszynaStartupTest does it, driven up to a
## speed by the player's keys and stopped by one brake: the train brake at its full service position
## or at emergency, or the local brake in full. Once its cylinders have filled - to
## APPLIED_PRESSURE_SHARE of the most they get - it has to stand within the distance its data
## allows from there: the brakes' force at full pressure (get_force_at(), BrakeForceR,
## Mover.cpp:4606, what the original's driver works out its braking from, Driver.cpp:2286-2291),
## each no more than its wheels' adhesion, against the mass, the speed falling from the one the
## cylinders filled at. How long they took is logged, not asserted: the Mover's valves fill an
## emergency stop's in 5.1-6.3 s, UIC 540 gives 3-5 s for P - the original's model, no port of it.
## Issue #310: an induction motor's local brake braked nothing, an AI's Vectron ran through a signal
## at stop.
## A test of one vehicle is a script of its own (test_zzz_braking_<vehicle>.gd) calling run_braking().

## Which brake stops it
enum Brake {
    ## the train brake handle at its full service position (HANDLE_POSITION_FULL), the keys of the
    ## brake level
    TRAIN,
    ## the train brake handle at emergency (HANDLE_POSITION_EMERGENCY)
    EMERGENCY,
    ## the local (independent) brake in full
    LOCAL,
}
## The speed it brakes from [km/h] - half the vehicle's own top speed when that is lower
const BRAKING_SPEED:float = 40.0
const BRAKING_SPEED_SHARE:float = 0.5
## Simulated seconds it is given to reach the speed
const ACCELERATION_TIMEOUT:float = 240.0
## The share of a gearbox engine's top revs it takes the next gear at
const FULL_REVS_SHARE:float = 0.95
## Simulated seconds a brake is given to stop it
const STOPPING_TIMEOUT:float = 180.0
## Standing [km/h]
const STOPPED_SPEED:float = 0.1
## Brake level and local brake key presses given at most to reach the position
const MAX_HANDLE_STEPS:int = 20
## The local brake's normalized position taken as full
const FULL_LOCAL_BRAKE:float = 0.99
## The speed step the stopping distance is summed by [km/h]
const DISTANCE_STEP_SPEED:float = 1.0
## The data's distance is an ideal: the force at full pressure all the way, the friction of the
## blocks as the data gives it. What the vehicle may run beyond it
const DISTANCE_MARGIN:float = 1.5
## The brake taken as applied: its cylinders at this share of the most they got on the way to the
## stop - not its force, which rises to the stop with the friction of the blocks (UIC 540 times the
## filling to 95%)
const APPLIED_PRESSURE_SHARE:float = 0.95
## The local brake types a driver applies from the cab - the manual one is the hand wheel
const DRIVER_LOCAL_BRAKES:Array[int] = [
    RailVehicleBrake.LOCAL_BRAKE_TYPE_PNEUMATIC, RailVehicleBrake.LOCAL_BRAKE_TYPE_HYDRAULIC]
## Adhesion of a wheel: the track's friction times (28 + v) / (14 + v), v the wheel's speed [m/s],
## lowered by the slip beyond the micro-slips (TMoverParameters::Adhesive(), Mover.cpp:4757-4763).
## A brake the data gives more force than the rolling wheel's locks its wheels, which slide on at
## the lower one (an SM40 below 13 km/h, no anti-slip)
const ADHESION_SPEED_HIGH:float = 28.0
const ADHESION_SPEED_LOW:float = 14.0
const ADHESION_MICRO_SLIP:float = 0.25
const ADHESION_SLIP_FACTOR:float = 0.25
const ADHESION_SLIP_RATE:float = 0.33
const GRAVITY:float = 9.81
const KMH_PER_MPS:float = 3.6
const METRES_PER_KILOMETRE:float = 1000.0


## Starts `vehicle` of `scenery` as a `kind`, drives it up to the speed and stops it by `brake`
func run_braking(scenery:String, vehicle:String, kind:Kind, brake_kind:Brake,
        pantographs:Pantographs = Pantographs.SWITCHES) -> void:
    await run_startup(scenery, vehicle, kind, pantographs)
    if not is_passing():
        return
    var brake:RailVehicleBrake = _brake(occupied)
    if brake_kind == Brake.LOCAL and not brake.cntrl_local_brake_type in DRIVER_LOCAL_BRAKES:
        pass_test("%s has no local brake a driver applies" % vehicle)
        return
    # an induction motor's local valve takes only the blending, and an MHZ_EN57 handle asks it for
    # the train brake handle's EP braking, not the local lever's (LocalBrakeRatio(), Mover.cpp:998-1010)
    if brake_kind == Brake.LOCAL and (brake as MaszynaMoverRailVehicleBrake).cntrl_brake_handle_type \
            == MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_MHZ_EN57 \
            and _engine(powered) and _engine(powered).get_type() == RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR:
        pass_test("%s brakes by the local lever nothing: its blending takes the train brake handle's EP" % vehicle)
        return
    # an induction motor's local valve takes only the ED/EP blending, not ported yet (the TODO in
    # MaszynaMoverDriveUnit::process(), #350)
    if brake_kind == Brake.LOCAL and _engine(powered) \
            and _engine(powered).get_type() == RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR:
        pending("%s: the local brake of an induction motor waits for the ED/EP blending (#350)" % vehicle)
        return
    # an individual brake (a draisine, EL16) has no brake pipe: its hand wheel is its brake
    if not brake_kind == Brake.LOCAL and brake.cntrl_brake_system == RailVehicleBrake.BRAKE_SYSTEM_INDIVIDUAL:
        pass_test("%s has no train brake" % vehicle)
        return
    var controller:VehicleController = VehicleServer.vehicle_get_controller(occupied)
    var target:float = minf(BRAKING_SPEED, controller.max_velocity * BRAKING_SPEED_SHARE)
    var master:RailVehicleMasterController = _master(powered)
    var position_max:int = int(VehicleServer.vehicle_dump_config(powered).get("master_controller_position_max", 0))
    # a gearbox takes its next gear at full revs (the manual) - an SN61's gears are its master
    # controller's positions
    var gearbox:RailVehicleDieselEngine = null
    if kind == Kind.DIESEL_MECHANICAL and not _sn61():
        gearbox = _engine(powered) as RailVehicleDieselEngine
    var elapsed:int = 0
    while _speed() < target and elapsed < ticks(ACCELERATION_TIMEOUT):
        if gearbox and gearbox.get_rpm() >= gearbox.get_max_rpm() * FULL_REVS_SHARE \
                and master.get_second_position() < master.get_second_position_count():
            await key_tap(&"second_controller_increase")
            await step(ticks(CONTROLLER_NOTCH_SECONDS))
            elapsed += ticks(CONTROLLER_NOTCH_SECONDS) + 1
        elif master.get_main_position() < position_max:
            await key_tap(&"main_controller_increase")
            await step(ticks(CONTROLLER_NOTCH_SECONDS))
            elapsed += ticks(CONTROLLER_NOTCH_SECONDS) + 1
        else:
            await step(1)
            elapsed += 1
    if not await _until("accelerated to %.0f km/h" % target, func() -> bool: return _speed() >= target, 0.0):
        return
    while master.get_main_position() > 0:
        await key_tap(&"main_controller_decrease")

    var speed:float = _speed()
    var start:float = controller.get_total_distance()
    var trainset:Array = RailVehicleServer.vehicle_get_coupled(
            occupied, RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_COUPLER)
    # the local brake acts on its own car
    var braking:Array = trainset if not brake_kind == Brake.LOCAL else [occupied]
    if brake_kind == Brake.TRAIN:
        # a press at a time: held, a handle that moves while the key is (FV4a) runs on to emergency
        var full:float = brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_FULL)
        for _step:int in MAX_HANDLE_STEPS:
            if brake.get_controller_position() >= full:
                break
            await key_tap(&"brake_level_increase")
        if not await _until("brake handle at full service", func() -> bool:
                return brake.get_controller_position() >= full, 0.0):
            return
    elif brake_kind == Brake.EMERGENCY:
        # its own key, as the original's trainbrakeemergency (Num 0) - the brake level keys stop
        # many a handle at full service
        var emergency:float = brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_EMERGENCY)
        if not await _key_taken(&"brake_level_emergency", func() -> bool:
                return brake.get_controller_position() >= emergency):
            return
    else:
        await key_press(&"local_brake_increase")
        var applied:bool = await _until("local brake in full", func() -> bool:
                return brake.get_local_position_normalized() >= FULL_LOCAL_BRAKE)
        await key_release(&"local_brake_increase")
        if not applied:
            return
    # the way to the stop, tick by tick: the distance run, the speed and the cylinders' pressure then
    var distances:PackedFloat64Array = []
    var speeds:PackedFloat64Array = []
    var pressures:PackedFloat64Array = []
    for _tick:int in ticks(STOPPING_TIMEOUT):
        if _speed() < STOPPED_SPEED:
            break
        var pressure:float = 0.0
        for car:RID in braking:
            pressure += _brake(car).get_air_pressure()
        distances.append((controller.get_total_distance() - start) * METRES_PER_KILOMETRE)
        speeds.append(_speed())
        pressures.append(pressure)
        await step(1)
    if not await _until("stopped from %.0f km/h" % speed, func() -> bool: return _speed() < STOPPED_SPEED, 0.0):
        return
    var distance:float = (controller.get_total_distance() - start) * METRES_PER_KILOMETRE
    var applied:int = 0
    var most:float = 0.0
    for pressure:float in pressures:
        most = maxf(most, pressure)
    while applied < pressures.size() - 1 and pressures[applied] < most * APPLIED_PRESSURE_SHARE:
        applied += 1
    var filled:float = applied * TICK
    gut.p("%s: the cylinders at %.0f%% of %.2f bar after %.1f s, at %.1f m, %.1f km/h" % [
            vehicle, APPLIED_PRESSURE_SHARE * 100.0, most, filled, distances[applied], speeds[applied]])
    var ideal:float = _ideal_distance(trainset, braking, speeds[applied])
    var allowed:float = distances[applied] + ideal * DISTANCE_MARGIN
    gut.p("%s stopped from %.1f km/h in %.1f m, the data allows %.1f m (%.1f m braked in full from there)" % [
            vehicle, speed, distance, allowed, ideal])
    assert_lt(distance, allowed, "%s stopped by the %s brake within what its data allows" % [
            vehicle, Brake.keys()[brake_kind].to_lower()])


## The distance the trainset stops in from `speed` with `braking` braked in full from the start [m]:
## their force at full pressure, each no more than its wheels' adhesion, against the trainset's mass,
## summed speed step by speed step
func _ideal_distance(trainset:Array, braking:Array, speed:float) -> float:
    # the track's friction as the vehicle gets it (RailVehicleServer: the track's times the
    # scenery's multiplier and the weather's, DynObj.cpp:3176)
    var track:RID = RailVehicleServer.vehicle_get_track_position(occupied)["track_rid"]
    var friction:float = TrackServer.track_get_friction(track) * SimulationServer.get_friction() \
            * SimulationServer.simulation_get_weather_friction_factor()
    var mass:float = 0.0
    for car:RID in trainset:
        mass += VehicleServer.vehicle_get_controller(car).get_mass_total()
    var stopping:float = 0.0
    var low:float = 0.0
    while low < speed:
        var high:float = minf(low + DISTANCE_STEP_SPEED, speed)
        var middle:float = (low + high) / 2.0
        var force:float = 0.0
        var velocity:float = middle / KMH_PER_MPS
        var rolling:float = friction * (ADHESION_SPEED_HIGH + velocity) / (ADHESION_SPEED_LOW + velocity)
        var slip:float = maxf(0.0, velocity - ADHESION_MICRO_SLIP)
        var sliding:float = friction * ADHESION_SPEED_HIGH / ADHESION_SPEED_LOW \
                * (1.0 - (1.0 - ADHESION_SLIP_FACTOR) * slip / (slip + ADHESION_SLIP_RATE))
        for car:RID in braking:
            var weight:float = VehicleServer.vehicle_get_controller(car).get_mass_total() * GRAVITY
            var brake_force:float = _brake(car).get_force_at(1.0, middle)
            force += brake_force if brake_force <= weight * rolling else weight * sliding
        # no brake force in the data allows no distance at all
        if force <= 0.0:
            return 0.0
        stopping += (middle / KMH_PER_MPS) * ((high - low) / KMH_PER_MPS) * mass / force
        low = high
    return stopping


func _speed() -> float:
    return absf(VehicleServer.vehicle_get_speed(occupied))
