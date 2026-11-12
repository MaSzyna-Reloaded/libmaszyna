extends Node

## The console commands of MaSzyna (Console.add_command()): what the original's commands do to its
## scenario, here by hand


func _ready() -> void:
    Console.add_command("queueevent", queue_event, ["event", "vehicle"], 1,
            "Queue a scenery event, by a vehicle if one is named")


func _exit_tree() -> void:
    Console.remove_command("queueevent")


## user_command::queueevent (simulation.cpp:293-307): the event of the name queued as the original
## queues it (event_manager::AddToQuery()), by the vehicle of the name if one is given - its scenery
## name, as the original finds a vehicle (FindDynamic())
func queue_event(event_name:String, vehicle_name:String = "") -> void:
    var event:RID = ScenarioEventServer.event_get_rid_by_name(StringName(event_name.to_lower()))
    if not event.is_valid():
        Console.print_line("No event named \"%s\"" % event_name)
        return
    var vehicle:RID = VehicleServer.vehicle_get_rid_by_id(vehicle_name.to_lower()) if vehicle_name else RID()
    if vehicle_name and not vehicle.is_valid():
        Console.print_line("No vehicle named \"%s\"" % vehicle_name)
        return
    MaszynaScenarioEventServer.event_add_to_query(event, vehicle)
