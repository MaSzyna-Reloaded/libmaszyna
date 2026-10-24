extends RefCounted
class_name MaszynaStation

## The passengers of a station (basic_station::update_load(), station.cpp:25-88): at a stop of its
## timetable a train's passenger cars let a random group off and take one on at the platform; the
## train's dispatch (StationServer) waits for them. Only cars with doors and passengers, or empty
## ones that take passengers, exchange anything.

const PASSENGERS:String = "passengers"
## Up to this share of a car's capacity gets off, and as much gets on (station.cpp:62-66)
const GROUP_SHARE:float = 0.15
## A station exchanges twice the group of a small stop (station.cpp:35)
const STATION_GROUP:float = 2.0
const STOP_GROUP:float = 1.0
## Twice as many get on at the first station (station.cpp:71-74)
const FIRST_STATION_BOARDING:int = 2
## TMoverParameters::DamageFlag dtrain_out, a derailed car (MOVER.h:151)
const DERAILED:int = 128


## The exchange at the station `timetable` stands at, on the `platform` side of the way `trainset`
## drives: its cars told what to let off and take on
static func update_load(
    trainset:MaszynaDriverTrainset, timetable:MaszynaDriverTimetable, platform:RailVehicleLoad.PlatformSide
) -> void:
    var index:int = timetable.station_index
    if index >= timetable.get_entry_count():
        return
    var first_stop:bool = index == 0
    var last_stop:bool = index == timetable.get_entry_count() - 1
    # a small stop and a technical one as the timetable says (station.cpp:31-34)
    var small_stop:bool = StationServer.station_get_type(StationServer.timetable_entry_get_station(
            timetable.timetable, index)) == StationServer.STATION_TYPE_PASSENGER_STOP
    var maintenance:bool = StationServer.timetable_entry_is_technical_stop(timetable.timetable, index)
    var group:float = GROUP_SHARE * (STOP_GROUP if small_stop else STATION_GROUP)
    for vehicle_index:int in trainset.vehicles.size():
        var vehicle:RID = trainset.vehicles[vehicle_index]
        var load:RailVehicleLoad = VehicleServer.vehicle_component_get(vehicle, VehicleComponentType.COMPONENT_LOAD)
        if not load or not VehicleServer.vehicle_component_get(vehicle, VehicleComponentType.COMPONENT_DOORS):
            continue
        var load_name:String = load.get_load_name()
        if not (load_name == PASSENGERS or (not load_name and load.accepted_loads.has(PASSENGERS))):
            continue
        var amount:float = load.get_load_amount()
        var derailed:bool = ((VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController)
                .get_train_damage() & DERAILED) > 0
        var getting_off:int = 0
        if derailed or last_stop:
            getting_off = int(amount)
        elif not (first_stop or maintenance):
            getting_off = int(minf(amount, randf_range(0.0, load.max_load * group)))
        var getting_on:int = 0
        if not (derailed or last_stop or maintenance):
            getting_on = int(randf_range(0.0, load.max_load * group))
            if first_stop:
                getting_on *= FIRST_STATION_BOARDING
        if getting_off == 0 and getting_on == 0:
            continue
        # the platform is on the train's side; a car the other way round has it on its other side
        # (TDynamicObject::LoadExchangeSpeed(), DynObj.cpp:2859-2860)
        var side:RailVehicleLoad.PlatformSide = platform if trainset.directions[vehicle_index] > 0 \
                else opposite_side(platform)
        if getting_off > 0:
            RailVehicleServer.load_remove(vehicle, getting_off, side)
        if getting_on > 0:
            RailVehicleServer.load_add(vehicle, getting_on, side, PASSENGERS)


## The platform seen from a car the other way round
static func opposite_side(side:RailVehicleLoad.PlatformSide) -> RailVehicleLoad.PlatformSide:
    match side:
        RailVehicleLoad.PLATFORM_SIDE_LEFT:
            return RailVehicleLoad.PLATFORM_SIDE_RIGHT
        RailVehicleLoad.PLATFORM_SIDE_RIGHT:
            return RailVehicleLoad.PLATFORM_SIDE_LEFT
    return side
