@tool
extends ScenarioEventAction
class_name MaszynaWhoisAction

## The original's `whois` event (whois_event::run_(), Event.cpp:1020-1153): writes what the vehicle
## that queued it is to its memory cells, the fields of `mask` only (TMemCell::UpdateValues(),
## MemCell.cpp:28-50). What is written goes by the mode bits (Event.h:37-39):
## +0 the train's name, the stations left and whether it stops at the next one - only with a driver;
## +8 the vehicle's destination, the way it faces in its trainset and its power;
## +16 its load, the amount and the most it takes; +24 its type, the trainset's brake level and the
## distance to an obstacle; +32 its name; +40 the next stop's name, 0 and whether it stops there.
## The train's place in its timetable is its run's (StationServer): the next station is the run's
## entry, or the one after it once the train has left or passed it (StationIndexInc(),
## Driver.cpp:1154, 1305).

## The mode bits (basic_event::flags, Event.h:37-39)
const MODE_ALT:int = 8
const MODE_LOAD:int = 16
const MODE_NAME:int = 32
## What a driver without a timetable is on (TTrainParameters("none"), Driver.cpp:1907) and where a
## train with no stop left goes (TTrainParameters::NextStop(), mtable.cpp:36-42)
const NO_TRAIN:String = "none"
const END_OF_ROUTE:String = "[End of route]"
## A vehicle sent nowhere (DynObj.cpp:1921)
const NO_DESTINATION:String = "none"
## How far ahead an obstacle is looked for [m] (TController::scan_obstacles(), Driver.cpp:6667),
## within which the distance is measured between the couplers (Driver.cpp:6674-6677), and the
## distance of none (neighbour_data, MOVER.h:1254)
const OBSTACLE_SCAN_RANGE:float = 1000.0
const OBSTACLE_COUPLER_RANGE:float = 100.0
const NO_OBSTACLE:float = 10000.0
## What a vehicle without a driver of its trainset reports for the brake level and the obstacle
## (Event.cpp:1087-1096)
const NO_OWNER:float = -1.0
## DirectionGet(): 1 facing the trainset's head, -1 the other way (DynObj.h:791)
const FACING_HEAD:float = 1.0
const FACING_END:float = -1.0

## The memory cells written
var memories:Array[RID] = []
## The fields written (MaszynaScenarioEventServer.MemoryField)
@export var mask:int = 0
## The mode bits
@export var mode:int = 0


func _run(_event:RID, activator:RID) -> void:
    # the original dereferences an event queued by nothing (MASZYNA_ORIGINAL_QUIRKS.md)
    if not activator.is_valid():
        return
    var trainset:RID = RailVehicleServer.vehicle_get_trainset(activator)
    var run:RID = StationServer.run_find_by_trainset(trainset) if trainset.is_valid() else RID()
    var timetable:RID = StationServer.run_get_timetable(run) if run.is_valid() else RID()
    var entry_count:int = StationServer.timetable_get_entry_count(timetable) if timetable.is_valid() else 0
    var next_entry:int = 0
    if run.is_valid():
        next_entry = StationServer.run_get_current_entry(run)
        if next_entry < entry_count and StationServer.run_entry_get_actual_departure(run, next_entry) >= 0.0:
            next_entry += 1
    # TTrainParameters::IsStop(): the last one is always a stop (mtable.cpp:67-73)
    var is_stop:float = 1.0 if next_entry >= entry_count or StationServer.timetable_entry_is_stop(timetable, next_entry) else 0.0
    var text:String = ""
    var value1:float = 0.0
    var value2:float = 0.0
    if mode & MODE_NAME and mode & MODE_ALT:
        # the next stop of the train's driver (Event.cpp:1034-1058)
        text = NO_TRAIN
        value2 = 0.0
        if timetable.is_valid():
            text = END_OF_ROUTE if next_entry >= entry_count else StationServer.station_get_name(
                    StationServer.timetable_entry_get_station(timetable, next_entry))
            value2 = is_stop
    elif mode & MODE_NAME:
        text = VehicleServer.vehicle_get_name(activator)
    elif mode & MODE_LOAD and mode & MODE_ALT:
        # its type, the most braked vehicle of the trainset and the obstacle ahead of its head
        # (Event.cpp:1078-1104, TController::CheckVehicles(), Driver.cpp:6034-6081)
        text = RailVehicleServer.vehicle_get_type_name(activator)
        value1 = NO_OWNER
        value2 = NO_OWNER
        # a driver aboard some vehicle of the trainset - the original's Mechanik, else its ctOwner
        var driven:bool = false
        for vehicle:RID in RailVehicleServer.trainset_get_vehicles(trainset) if trainset.is_valid() else []:
            driven = driven or DriverServer.vehicle_get_driver(vehicle).is_valid()
        if driven:
            value1 = 0.0
            for vehicle:RID in RailVehicleServer.trainset_get_vehicles(trainset):
                var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
                        vehicle, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
                if brake:
                    value1 = maxf(value1, maxf(0.0, brake.get_air_pressure()))
            var head:RID = RailVehicleServer.trainset_get_head(trainset)
            var end:RailVehicleController.CouplerEnd = (
                    RailVehicleController.COUPLER_END_REAR
                    if RailVehicleServer.trainset_vehicle_get_direction(head) == TrackServer.DIRECTION_REVERSED
                    else RailVehicleController.COUPLER_END_FRONT)
            var obstacle:RailVehicleNeighbour = RailVehicleServer.vehicle_find_vehicle(head, end, OBSTACLE_SCAN_RANGE)
            value2 = NO_OBSTACLE
            if obstacle:
                value2 = obstacle.get_distance()
                if value2 < OBSTACLE_COUPLER_RANGE:
                    value2 = RailVehicleServer.coupler_get_distance(head, obstacle.get_vehicle_rid())
    elif mode & MODE_LOAD:
        # Event.cpp:1107-1118
        var load:RailVehicleLoad = VehicleServer.vehicle_component_get(
                activator, VehicleComponentType.COMPONENT_LOAD) as RailVehicleLoad
        if load:
            text = load.get_load_name()
            value1 = load.get_load_amount()
            value2 = load.max_load
    elif mode & MODE_ALT:
        # its destination - a station, and a track of it after a `:` -, the way it faces and its
        # power (Event.cpp:1122-1133)
        var station:RID = RailVehicleServer.vehicle_get_destination_station(activator)
        var track:RID = RailVehicleServer.vehicle_get_destination_track(activator)
        text = StationServer.station_get_name(station) if station.is_valid() else NO_DESTINATION
        if track.is_valid():
            text += MaszynaStations.DESTINATION_TRACK_SEPARATOR + TrackServer.track_get_name(track)
        value1 = FACING_END if (trainset.is_valid()
                and RailVehicleServer.trainset_vehicle_get_direction(activator) == TrackServer.DIRECTION_REVERSED) else FACING_HEAD
        var controller:VehicleController = VehicleServer.vehicle_get_controller(activator)
        value2 = controller.power if controller else 0.0
    else:
        # only a vehicle with a driver (Event.cpp:1136-1150)
        if not DriverServer.vehicle_get_driver(activator).is_valid():
            return
        text = StationServer.timetable_get_train_name(timetable) if timetable.is_valid() else NO_TRAIN
        value1 = maxi(entry_count - next_entry - 1, 0)
        value2 = is_stop
    for memory:RID in memories:
        MaszynaScenarioEventServer.memory_set_values(
            memory,
            text if mask & MaszynaScenarioEventServer.MEMORY_FIELD_TEXT else MaszynaScenarioEventServer.memory_get_text(memory),
            value1 if mask & MaszynaScenarioEventServer.MEMORY_FIELD_VALUE1 else MaszynaScenarioEventServer.memory_get_value1(memory),
            value2 if mask & MaszynaScenarioEventServer.MEMORY_FIELD_VALUE2 else MaszynaScenarioEventServer.memory_get_value2(memory),
        )
        # a written text may be a command for a driver (UpdateValues(), MemCell.cpp:47-49)
        if mask & MaszynaScenarioEventServer.MEMORY_FIELD_TEXT:
            MaszynaScenarioEventServer.memory_check_command(memory)
