extends Node


@export var visible:bool = false:
    set(x):
        if Console:
            Console.set_visible(x)
        visible = x


func _ready() -> void:
    Console.control.visible = visible
    Console.add_command("broadcast", self.console_broadcast, ["command", "p1", "p2"], 1, "Broadcast message to all trains")
    Console.add_command("send", self.console_send, ["train", "command", "p1", "p2"], 2, "Send message to a train")
    Console.add_command("trains", self.console_list_trains, 0, 0, "List trains")
    Console.add_command("commands", self.console_list_train_commands, 0, 0, "List available train commands")
    Console.add_command("get", self.console_get_train_state, ["train", "parameter"], 1, "Get train state / parameter")
    Console.add_command("prop", self.console_get_config_value, ["train", "property"], 2, "Get train config property")
    Console.add_command("props", self.console_get_config_properties, ["train"], 1, "List train config properties")
    Console.add_command(
        "cabin", self.console_cabin, ["train", "operation", "control", "value"], 2,
        "Occupied cabin: controls | state | get [control] | increase|decrease|hold|release|toggle|set <control> [value]")

## The console knows a vehicle by its scenery name only, so it goes through the server's name
## registry; a name that is empty, "none" or unknown reaches no vehicle.
func _vehicle(train:String) -> RID:
    var vehicle:RID = VehicleServer.vehicle_get_rid_by_name(train)
    if not vehicle.is_valid():
        console_print_error("No vehicle named \"%s\"" % train)
    return vehicle

func console_get_config_value(train, property):
    var vehicle:RID = _vehicle(train)
    if vehicle.is_valid():
        Console.print_line("%s" % VehicleServer.vehicle_dump_config(vehicle).get(property))

func console_get_config_properties(train):
    var vehicle:RID = _vehicle(train)
    if not vehicle.is_valid():
        return
    var config:Dictionary = VehicleServer.vehicle_dump_config(vehicle)
    var lines = []
    for prop in config:
        lines.append("%s=%s" % [prop, config[prop]])
    Console.print_line("%s" % "\n".join(lines))

func console_broadcast(command, p1=null, p2=null):
    VehicleServer.vehicle_broadcast_command(command, p1, p2)

func console_send(train, command, p1=null, p2=null):
    var vehicle:RID = _vehicle(train)
    if vehicle.is_valid():
        VehicleServer.vehicle_send_command(vehicle, command, p1, p2)

func console_list_trains():
    var names:PackedStringArray = []
    for vehicle:RID in VehicleServer.vehicle_get_rids():
        names.append(VehicleServer.vehicle_get_name(vehicle))
    Console.print_line("%s" % "\n".join(names))

func console_list_train_commands():
    var commands:Dictionary[String, bool] = {}
    for vehicle:RID in VehicleServer.vehicle_get_rids():
        for command:String in VehicleServer.vehicle_get_commands(vehicle):
            commands[command] = true
    var names:Array[String] = []
    names.assign(commands.keys())
    names.sort()
    Console.print_line("%s" % "\n".join(names))

## A command's own error, in the console - the game's log (GameLog) is not printed here
func console_print_error(line:String) -> void:
    Console.print_line("[color=red]%s[/color]" % [line])

func console_cabin(train, operation, control=null, value=null):
    var vehicle:RID = _vehicle(train)
    if not vehicle.is_valid():
        return
    # the cabin the vehicle is driven from
    var cabin:RID = RailVehicleServer.vehicle_get_driver_cabin(vehicle)
    if not cabin.is_valid():
        console_print_error("%s: Nobody drives it, no cabin to operate" % [train])
        return
    if operation == "controls":
        Console.print_line("controls:\n%s\nactions: %s" % [
            "\n".join(CabinSystem.get_controls(cabin)), ", ".join(CabinSystem.ACTIONS)])
    elif operation == "state" or (operation == "get" and not control):
        Console.print_line("%s" % [CabinSystem.get_state(cabin)])
    elif operation == "get":
        Console.print_line("%s" % [CabinSystem.get_control(cabin, control)])
    elif not StringName(operation) in CabinSystem.ACTIONS:
        console_print_error("%s: Unknown cabin operation: %s" % [train, operation])
    elif not control:
        console_print_error("%s: Cabin operation %s needs a control id" % [train, operation])
    else:
        Console.print_line("%s" % [CabinSystem.act(cabin, control, operation, value)])

func console_get_train_state(train, key=null):
    var vehicle:RID = _vehicle(train)
    if not vehicle.is_valid():
        return
    var out = VehicleServer.vehicle_dump_state(vehicle)
    if key:
        out = out.get(key)
    Console.print_line("%s" % [out])
