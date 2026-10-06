extends Control

var _t:float = 0.0

@onready var train = $SM42
@onready var brake = $SM42/Brake
@onready var engine = $SM42/StonkaDieselEngine
@onready var security = $SM42/RailVehicleSecuritySystem
@onready var doors = $SM42/RailVehicleDoors
@onready var battery_progress_bar = $%BatteryProgressBar

@onready var FORWARD = $UI/MoverSwitches/Controller/Direction/Forward
@onready var REVERSE = $UI/MoverSwitches/Controller/Direction/Reverse

const rich_print_loglevel_colors = {
    GameLog.LogLevel.DEBUG: "#777",
    GameLog.LogLevel.ERROR: "red",
    GameLog.LogLevel.WARNING: "orange",
    }

const loglevel_names = {
    GameLog.LogLevel.DEBUG: "DEBUG",
    GameLog.LogLevel.INFO: "INFO",
    GameLog.LogLevel.WARNING: "WARNING",
    GameLog.LogLevel.ERROR: "ERROR",
    }
# Called when the node enters the scene tree for the first time.
func _ready() -> void:
    $%TrainName.text = tr("%s (type: %s)") % [train.vehicle_id, train.type_name]
    GameLog.message_logged.connect(print_log_entry_to_godot_console)


func _colorize_loglevel(loglevel, line):
    var color = rich_print_loglevel_colors.get(loglevel)
    if color:
        return "[color=%s]%s[/color]" % [color, line]
    else:
        return line

func print_log_entry_to_godot_console(logger_id, loglevel, line):
    if not logger_id == "game":
        return
    print_rich(_colorize_loglevel(loglevel, "%s: %s" % [loglevel_names[loglevel], line]))


func print_train_log_entry_to_godot_console(train, loglevel, line):
    print_rich(_colorize_loglevel(loglevel, "LOG: [%s][%s] %s" % [train, loglevel, line]))

func draw_dictionary(dict: Dictionary, target: DebugPanel):
    var lines = []
    for k in dict.keys():
        lines.append("%s=%s" % [k, dict[k]])
    target.text = "\n".join(lines)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
    _t += delta

    if(_t>0.1):
        _t = 0

        var train_state = train.get_state()
        var bv = train_state.get("battery_voltage", 0)

        $%BatteryProgressBar.value = bv
        $%BatteryValue.text = "%.2f V" % [bv]

        var security_state = security.get_state()
        var brake_state = brake.get_state()
        var engine_state = engine.get_state()
        var door_state = doors.get_state()

        draw_dictionary(engine_state, $%DebugEngine)
        draw_dictionary(train_state, $%DebugTrain)
        draw_dictionary(brake_state, $%DebugBrake)
        draw_dictionary(security_state, $%DebugSecurity)
        draw_dictionary(door_state, %DebugDoor)



func _on_brake_level_value_changed(value):
    VehicleServer.vehicle_broadcast_command("brake_level_set", value, null)

func _on_main_decrease_button_up():
    train.send_command("main_controller_decrease")

func _on_main_increase_button_up():
    train.send_command("main_controller_increase")

func _on_reverse_button_up():
    train.send_command("direction_decrease")

func _on_forward_button_up():
    train.send_command("direction_increase")

func _on_sm_42_simulation_initialized():
    print("Mover initialized. Train config: ", $SM42.get_config())
